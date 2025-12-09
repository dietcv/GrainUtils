#include "GrainDelay.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

GrainDelay::GrainDelay() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_sampleDur(static_cast<float>(sampleDur())),
    m_bufSize(NEXTPOWEROFTWO(static_cast<int>(MAX_DELAY_TIME * sampleRate()))),
    m_bufFrames(static_cast<float>(m_bufSize)),
    m_bufMask(m_bufSize - 1)
{
    // Initialize parameter cache
    triggerRatePast = sc_clip(in0(TriggerRate), 0.1f, 500.0f);
    overlapPast = sc_clip(in0(Overlap), 0.001f, static_cast<float>(NUM_CHANNELS));
    delayTimePast = sc_clip(in0(DelayTime), m_sampleDur, MAX_DELAY_TIME);
    grainRatePast = sc_clip(in0(GrainRate), 0.125f, 4.0f);
    mixPast = sc_clip(in0(Mix), 0.0f, 1.0f);
    feedbackPast = sc_clip(in0(Feedback), 0.0f, 0.99f);
    dampingPast = sc_clip(in0(Damping), 0.0f, 1.0f);

    // Check which inputs are audio-rate
    isTriggerRateAudioRate = isAudioRateIn(TriggerRate);
    isOverlapAudioRate = isAudioRateIn(Overlap);
    isDelayTimeAudioRate = isAudioRateIn(DelayTime);
    isGrainRateAudioRate = isAudioRateIn(GrainRate);
    isMixAudioRate = isAudioRateIn(Mix);
    isFeedbackAudioRate = isAudioRateIn(Feedback);
    isDampingAudioRate = isAudioRateIn(Damping);

    // Allocate audio buffer
    m_buffer = (float*)RTAlloc(mWorld, m_bufSize * sizeof(float));

    // Check the result of RTAlloc!
    auto unit = this;
    ClearUnitIfMemFailed(m_buffer);   
    
    // Initialize the allocated buffer with zeros
    memset(m_buffer, 0, m_bufSize * sizeof(float));
    
    mCalcFunc = make_calc_function<GrainDelay, &GrainDelay::next>();
    next(1);

    // Reset state after priming
    m_scheduler.reset();
    m_resetTrigger.reset();
}

GrainDelay::~GrainDelay() {
    RTFree(mWorld, m_buffer);
}

void GrainDelay::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
    
    // Control-rate parameters with smooth interpolation
    auto slopedTriggerRate = makeSlope(sc_clip(in0(TriggerRate), 0.1f, 500.0f), triggerRatePast);
    auto slopedOverlap = makeSlope(sc_clip(in0(Overlap), 0.001f, static_cast<float>(NUM_CHANNELS)), overlapPast);
    auto slopedDelayTime = makeSlope(sc_clip(in0(DelayTime), m_sampleDur, MAX_DELAY_TIME), delayTimePast);
    auto slopedGrainRate = makeSlope(sc_clip(in0(GrainRate), 0.125f, 4.0f), grainRatePast);
    auto slopedMix = makeSlope(sc_clip(in0(Mix), 0.0f, 1.0f), mixPast);
    auto slopedFeedback = makeSlope(sc_clip(in0(Feedback), 0.0f, 0.99f), feedbackPast);
    auto slopedDamping = makeSlope(sc_clip(in0(Damping), 0.0f, 1.0f), dampingPast);
    
    bool freeze = in0(Freeze) > 0.5f;
    bool reset = m_resetTrigger.process(in0(Reset));

    // Output pointers
    float* output = out(Output);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float triggerRate = isTriggerRateAudioRate ? 
            sc_clip(in(TriggerRate)[i], 0.1f, 500.0f) : 
            slopedTriggerRate.consume();
            
        float overlap = isOverlapAudioRate ? 
            sc_clip(in(Overlap)[i], 0.001f, static_cast<float>(NUM_CHANNELS)) : 
            slopedOverlap.consume();
            
        float delayTime = isDelayTimeAudioRate ? 
            sc_clip(in(DelayTime)[i], m_sampleDur, MAX_DELAY_TIME) : 
            slopedDelayTime.consume();
            
        float grainRate = isGrainRateAudioRate ? 
            sc_clip(in(GrainRate)[i], 0.125f, 4.0f) : 
            slopedGrainRate.consume();
        
        float mix = isMixAudioRate ? 
            sc_clip(in(Mix)[i], 0.0f, 1.0f) : 
            slopedMix.consume();
            
        float feedback = isFeedbackAudioRate ? 
            sc_clip(in(Feedback)[i], 0.0f, 0.99f) : 
            slopedFeedback.consume();
            
        float damping = isDampingAudioRate ? 
            sc_clip(in(Damping)[i], 0.0f, 1.0f) : 
            slopedDamping.consume();
        
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
                float normalizedDelay = sc_max(m_sampleDur, delayTime * m_sampleRate / m_bufFrames);
                float readPos = sc_frac(normalizedWritePos - normalizedDelay);
                
                // Store grain data
                m_grainData[g].readPos = readPos;
                m_grainData[g].rate = grainRate;
                m_grainData[g].sampleCount = scheduler.subSampleOffset;
                m_grainData[g].hasTriggered = true;
            }
            
            // Process grain if voice allocator says it's active
            if (m_allocator.isActive[g]) {
                
                // Calculate grain position: readPos + (accumulator * grainRate)
                float grainPos = (m_grainData[g].readPos * m_bufFrames) + (m_grainData[g].sampleCount * m_grainData[g].rate);
                
                // Get sample with interpolation
                float grainSample = Utils::peekCubicInterp(
                    m_buffer, 
                    grainPos,
                    m_bufMask
                );
                
                // Apply Hanning window using voice allocator's sub-sample accurate phase
                grainSample *= sc_hanwindow(m_allocator.phases[g]);
                delayed += grainSample;

                // Increment sample count
                m_grainData[g].sampleCount++;
            }
        }

        // 4. Apply amplitude compensation based on overlap
        float effectiveOverlap = sc_max(1.0f, overlap);
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
            m_writePos = m_writePos & m_bufMask;
        }
        
        // 7. Output with wet/dry mix
        output[i] = lininterp(mix, input[i], delayed);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    triggerRatePast = isTriggerRateAudioRate ? 
        sc_clip(in(TriggerRate)[nSamples - 1], 0.1f, 500.0f) : 
        slopedTriggerRate.value;
        
    overlapPast = isOverlapAudioRate ? 
        sc_clip(in(Overlap)[nSamples - 1], 0.001f, static_cast<float>(NUM_CHANNELS)) : 
        slopedOverlap.value;
        
    delayTimePast = isDelayTimeAudioRate ? 
        sc_clip(in(DelayTime)[nSamples - 1], m_sampleDur, MAX_DELAY_TIME) : 
        slopedDelayTime.value;
        
    grainRatePast = isGrainRateAudioRate ? 
        sc_clip(in(GrainRate)[nSamples - 1], 0.125f, 4.0f) : 
        slopedGrainRate.value;

    mixPast = isMixAudioRate ? 
        sc_clip(in(Mix)[nSamples - 1], 0.0f, 1.0f) : 
        slopedMix.value;
        
    feedbackPast = isFeedbackAudioRate ? 
        sc_clip(in(Feedback)[nSamples - 1], 0.0f, 0.99f) : 
        slopedFeedback.value;
        
    dampingPast = isDampingAudioRate ? 
        sc_clip(in(Damping)[nSamples - 1], 0.0f, 1.0f) : 
        slopedDamping.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<GrainDelay>(ft, "GrainDelay", false);
}