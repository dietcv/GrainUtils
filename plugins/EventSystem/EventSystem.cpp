#include "EventSystem.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== SCHEDULER CYCLE =====

SchedulerCycle::SchedulerCycle() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<SchedulerCycle, &SchedulerCycle::next>();
    next(1);

    // Reset state after priming
    m_scheduler.reset();
    m_resetTrigger.reset();
}

SchedulerCycle::~SchedulerCycle() = default;

void SchedulerCycle::next(int nSamples) {  
    // Audio-rate parameters
    const float* rateIn = in(Rate);
    const float* resetIn = in(Reset);
    
    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float rate = rateIn[i];
        bool reset = m_resetTrigger.process(resetIn[i]);

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
    mCalcFunc = make_calc_function<SchedulerBurst, &SchedulerBurst::next>();
    next(1);

    // Reset state after priming
    m_scheduler.reset();
    m_initTrigger.reset();
}

SchedulerBurst::~SchedulerBurst() = default;

void SchedulerBurst::next(int nSamples) {    
    // Audio-rate parameters
    const float* initTriggerIn = in(InitTrigger);
    const float* durationIn = in(Duration);
    const float* cyclesIn = in(Cycles);
    
    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        bool initTrigger = m_initTrigger.process(initTriggerIn[i]);
        float duration = durationIn[i];
        float cycles = cyclesIn[i];

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

VoiceAllocator::VoiceAllocator() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Get number of channels
    m_numChannels = sc_clip(static_cast<int>(in0(NumChannels)), 1, MAX_CHANNELS);
       
    mCalcFunc = make_calc_function<VoiceAllocator, &VoiceAllocator::next>();
    next(1);

    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}

VoiceAllocator::~VoiceAllocator() = default;

void VoiceAllocator::next(int nSamples) {
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* rateIn = in(Rate);
    const float* offsetIn = in(SubSampleOffset);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        bool trigger = m_trigger.process(triggerIn[i]);
        float rate = rateIn[i];
        float offset = offsetIn[i];

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
    mCalcFunc = make_calc_function<RampIntegrator, &RampIntegrator::next>();
    next(1);

    // Reset state after priming
    m_trigger.reset();
}

RampIntegrator::~RampIntegrator() = default;

void RampIntegrator::next(int nSamples) { 
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* rateIn = in(Rate);
    const float* offsetIn = in(SubSampleOffset);
   
    // Output pointer
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        bool trigger = m_trigger.process(triggerIn[i]);
        float rate = rateIn[i];
        float offset = offsetIn[i];

        // Process integrator
        phaseOut[i] = m_integrator.process(
            trigger,
            rate,
            offset,
            m_sampleRate
        );
    }
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator::RampAccumulator()
{
    mCalcFunc = make_calc_function<RampAccumulator, &RampAccumulator::next>();
    next(1);
    
    // Reset state after priming
    m_trigger.reset();
}

RampAccumulator::~RampAccumulator() = default;

void RampAccumulator::next(int nSamples) { 
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* offsetIn = in(SubSampleOffset);
   
    // Output pointer
    float* countOut = out(Count);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        bool trigger = m_trigger.process(triggerIn[i]);
        float offset = offsetIn[i];

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