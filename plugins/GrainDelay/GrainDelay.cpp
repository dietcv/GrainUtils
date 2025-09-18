#include "GrainDelay.hpp"
#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== GRAIN DELAY =====

GrainDelay::GrainDelay() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_sampleDur(static_cast<float>(sampleDur())),
    m_bufFrames(MAX_DELAY_TIME * m_sampleRate),
    m_bufSize(static_cast<int>(m_bufFrames)),
    m_allocator(NUM_CHANNELS)
{
    // Initialize graindata
    m_grainData.resize(NUM_CHANNELS);

    // Allocate audio buffer
    m_buffer = (float*)RTAlloc(mWorld, m_bufSize * sizeof(float));

    // Check the result of RTAlloc!
    auto unit = this;
    ClearUnitIfMemFailed(m_buffer);
    
    // Initialize the allocated buffer with zeros
    memset(m_buffer, 0, m_bufSize * sizeof(float));
    
    mCalcFunc = make_calc_function<GrainDelay, &GrainDelay::next_aa>();
    next_aa(1);
    m_scheduler.init();
}

GrainDelay::~GrainDelay() {
    RTFree(mWorld, m_buffer);
}

void GrainDelay::next_aa(int nSamples) {
    
    // Audio-rate parameters
    const float* input = in(Input);
    const float* triggerRateIn = in(TriggerRate);
    const float* overlapIn = in(Overlap);
    const float* delayTimeIn = in(DelayTime);
    const float* grainRateIn = in(GrainRate);
    
    // Control-rate parameters
    float mix = sc_clip(in0(Mix), 0.0f, 1.0f);
    float feedback = sc_clip(in0(Feedback), 0.0f, 0.99f);
    float damping = sc_clip(in0(Damping), 0.0f, 1.0f);
    bool freeze = in0(Freeze) > 0.5f;
    bool reset = in0(Reset) > 0.5f;

    // Output pointers
    float* output = out(Output);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Sample audio-rate parameters per-sample
        float triggerRate = triggerRateIn[i];
        float overlap = sc_clip(overlapIn[i], 0.001f, static_cast<float>(NUM_CHANNELS));
        float delayTime = sc_clip(delayTimeIn[i], m_sampleDur, MAX_DELAY_TIME);
        float grainRate = sc_clip(grainRateIn[i], 0.125f, 4.0f);
        
        // 1. Get event data from scheduler
        auto scheduler = m_scheduler.process(triggerRate, reset, m_sampleRate);
        
        // 2. Process voice allocation with scaled rate
        float rateScaled = scheduler.rate / overlap;
        m_allocator.process(
            scheduler.trigger, 
            rateScaled, 
            scheduler.subSampleOffset,
            m_sampleRate
        );
        
        // 3. Process all grains
        float delayed = 0.0f;
   
        for (int g = 0; g < NUM_CHANNELS; ++g) {

            // Trigger new grain if needed
            if (m_allocator.triggers[g]) {

                // Calculate read position
                float normalizedWritePos = static_cast<float>(m_writePos) / m_bufFrames;
                float normalizedDelay = std::max(m_sampleDur, delayTime * m_sampleRate / m_bufFrames);
                float readPos = sc_wrap(normalizedWritePos - normalizedDelay, 0.0f, 1.0f);
                
                // Store grain data
                m_grainData[g].readPos = readPos;
                m_grainData[g].rate = grainRate;
                m_grainData[g].sampleCount = scheduler.subSampleOffset;
                m_grainData[g].hasTriggered = true;
            }
            
            // Process grain if voice allocator says it's active
            if (m_allocator.isActive[g]) {

                // Increment sample count
                m_grainData[g].sampleCount++;
                
                // Calculate grain position: readPos + (accumulator * grainRate)
                float grainPos = (m_grainData[g].readPos * m_bufFrames) + (m_grainData[g].sampleCount * m_grainData[g].rate);
                
                // Get sample with interpolation
                float grainSample = Utils::peekCubicInterp(
                    m_buffer,
                    m_bufSize, 
                    grainPos
                );
                
                // Apply Hanning window using voice allocator's sub-sample accurate phase
                grainSample *= WindowFunctions::hanningWindow(m_allocator.phases[g], 0.5f);
                delayed += grainSample;
            }
        }

        // 4. Apply amplitude compensation based on overlap
        float effectiveOverlap = std::max(1.0f, overlap);
        float compensationGain = 1.0f / std::sqrt(effectiveOverlap);
        delayed *= compensationGain;
        
        // 5. Apply feedback with damping filter
        float dampedFeedback = m_dampingFilter.processLowpass(delayed, damping);
        dampedFeedback = zapgremlins(dampedFeedback); // Prevent feedback buildup
        
        // 6. DC block input and write to delay buffer (only when not frozen)
        float dcBlockedInput = m_dcBlocker.processHighpass(input[i], 3.0f, m_sampleRate);
        
        if (!freeze) {
            m_buffer[m_writePos] = dcBlockedInput + dampedFeedback * feedback;
            m_writePos++;
            m_writePos = sc_wrap(m_writePos, 0, m_bufSize - 1);
        }
        
        // 7. Output with wet/dry mix
        output[i] = Utils::lerp(input[i], delayed, mix);
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<GrainDelay>(ft, "GrainDelay", false);
}