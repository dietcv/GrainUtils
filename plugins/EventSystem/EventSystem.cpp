#include "EventSystem.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== SCHEDULER CYCLE =====

SchedulerCycle::SchedulerCycle() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isRateAudioRate = isAudioRateIn(Rate);
    isResetAudioRate = isAudioRateIn(Reset);
    
    mCalcFunc = make_calc_function<SchedulerCycle, &SchedulerCycle::next>();
    next(1);

    // Reset state after priming
    m_scheduler.reset();
    m_resetTrigger.reset();
}

SchedulerCycle::~SchedulerCycle() = default;

void SchedulerCycle::next(int nSamples) {  

    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get current parameter values (no interpolation - latched per trigger)
        float rate = isRateAudioRate ? 
            sc_clip(in(Rate)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
            sc_clip(in0(Rate), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
        
        // Trigger input (audio-rate or control-rate)
        bool reset = isResetAudioRate ? 
            m_resetTrigger.process(in(Reset)[i]) : 
            m_resetTrigger.process(in0(Reset));

        // Process event scheduler
        auto event = m_scheduler.process(
            rate, 
            reset, 
            m_sampleRate
        );
        
        // Output values
        triggerOut[i] = event.trigger;
        phaseOut[i] = event.phase;
        rateOut[i] = event.rate;
        offsetOut[i] = event.subSampleOffset;
    }
}

// ===== SCHEDULER BURST =====

SchedulerBurst::SchedulerBurst() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isInitTriggerAudioRate = isAudioRateIn(InitTrigger);
    isDurationAudioRate = isAudioRateIn(Duration);
    isCyclesAudioRate = isAudioRateIn(Cycles);
    
    mCalcFunc = make_calc_function<SchedulerBurst, &SchedulerBurst::next>();
    next(1);

    // Reset state after priming
    m_scheduler.reset();
    m_initTrigger.reset();
}

SchedulerBurst::~SchedulerBurst() = default;

void SchedulerBurst::next(int nSamples) {   

    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Trigger input (audio-rate or control-rate)
        bool initTrigger = isInitTriggerAudioRate ? 
            m_initTrigger.process(in(InitTrigger)[i]) : 
            m_initTrigger.process(in0(InitTrigger));
        
        // Get current parameter values (no interpolation - latched per trigger)
        float duration = isDurationAudioRate ? 
            sc_max(in(Duration)[i], 0.001f) : 
            sc_max(in0(Duration), 0.001f);
        
        int cycles = isCyclesAudioRate ? 
            sc_max(static_cast<int>(in(Cycles)[i]), 1) : 
            sc_max(static_cast<int>(in0(Cycles)), 1);
        
        // Process event scheduler
        auto event = m_scheduler.process(
            initTrigger,
            duration,
            cycles,
            m_sampleRate
        );
        
        // Output values
        triggerOut[i] = event.trigger;
        phaseOut[i] = event.phase;
        rateOut[i] = event.rate;
        offsetOut[i] = event.subSampleOffset;
    }
}

// ===== VOICE ALLOCATOR =====

VoiceAllocator::VoiceAllocator() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_numChannels(sc_clip(static_cast<int>(in0(NumChannels)), 1, MAX_CHANNELS))
{
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isRateAudioRate = isAudioRateIn(Rate);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
       
    mCalcFunc = make_calc_function<VoiceAllocator, &VoiceAllocator::next>();
    next(1);

    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}

VoiceAllocator::~VoiceAllocator() = default;

void VoiceAllocator::next(int nSamples) {
    for (int i = 0; i < nSamples; ++i) {
        
        // Trigger input (audio-rate or control-rate)
        bool trigger = isTriggerAudioRate ? 
            m_trigger.process(in(Trigger)[i]) : 
            m_trigger.process(in0(Trigger));
        
        // Get current parameter values (no interpolation - latched per trigger)
        float rate = isRateAudioRate ? 
            sc_clip(in(Rate)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
            sc_clip(in0(Rate), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
        
        float offset = isSubSampleOffsetAudioRate ? 
            in(SubSampleOffset)[i] : 
            in0(SubSampleOffset);

        // Process voice allocator
        m_allocator.process(
            trigger, 
            rate, 
            offset, 
            m_sampleRate
        );
       
        // Output phases and triggers
        for (int ch = 0; ch < m_numChannels; ++ch) {
            out(ch)[i] = m_allocator.phases[ch];
            out(m_numChannels + ch)[i] = m_allocator.triggers[ch];
        }
    }
}

// ===== RAMP INTEGRATOR =====

RampIntegrator::RampIntegrator() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    ratePast = in0(Rate);
    
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isRateAudioRate = isAudioRateIn(Rate);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    
    mCalcFunc = make_calc_function<RampIntegrator, &RampIntegrator::next>();
    next(1);

    // Reset state after priming
    m_integrator.reset();
    m_trigger.reset();
}

RampIntegrator::~RampIntegrator() = default;

void RampIntegrator::next(int nSamples) {

    // Control-rate parameters with smooth interpolation
    auto slopedRate = makeSlope(sc_clip(in0(Rate), m_sampleRate * -0.49f, m_sampleRate * 0.49f), ratePast);
    
    // Output pointer
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {

        // Trigger input (audio-rate or control-rate)
        bool trigger = isTriggerAudioRate ? 
            m_trigger.process(in(Trigger)[i]) : 
            m_trigger.process(in0(Trigger));
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float rate = isRateAudioRate ? 
            sc_clip(in(Rate)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
            slopedRate.consume();
        
        // Get current parameter values (no interpolation - latched per trigger)
        float offset = isSubSampleOffsetAudioRate ? 
            in(SubSampleOffset)[i] : 
            in0(SubSampleOffset);

        // Process integrator
        phaseOut[i] = m_integrator.process(
            trigger, 
            rate, 
            offset, 
            m_sampleRate
        );
    }
    
    // Update parameter cache
    ratePast = isRateAudioRate ? in(Rate)[nSamples - 1] : slopedRate.value;
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator::RampAccumulator()
{
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    
    mCalcFunc = make_calc_function<RampAccumulator, &RampAccumulator::next>();
    next(1);
    
    // Reset state after priming
    m_accumulator.reset();
    m_trigger.reset();
}

RampAccumulator::~RampAccumulator() = default;

void RampAccumulator::next(int nSamples) {

    // Output pointer
    float* countOut = out(Count);
   
    for (int i = 0; i < nSamples; ++i) {

        // Trigger input (audio-rate or control-rate)
        bool trigger = isTriggerAudioRate ? 
            m_trigger.process(in(Trigger)[i]) : 
            m_trigger.process(in0(Trigger));
        
        // Get current parameter values (no interpolation - latched per trigger)
        float offset = isSubSampleOffsetAudioRate ? 
            in(SubSampleOffset)[i] : 
            in0(SubSampleOffset);

        // Process accumulator
        countOut[i] = m_accumulator.process(
            trigger, 
            offset
        );
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<SchedulerCycle>(ft, "SchedulerCycleUGen", false);
    registerUnit<SchedulerBurst>(ft, "SchedulerBurstUGen", false);
    registerUnit<VoiceAllocator>(ft, "VoiceAllocatorUGen", false);
    registerUnit<RampIntegrator>(ft, "RampIntegrator", false);
    registerUnit<RampAccumulator>(ft, "RampAccumulator", false);
}