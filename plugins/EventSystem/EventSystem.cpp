#include "EventSystem.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== SCHEDULER CYCLE =====

SchedulerCycle::SchedulerCycle() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<SchedulerCycle, &SchedulerCycle::next_aa>();
    next_aa(1);
    m_scheduler.init();
}

SchedulerCycle::~SchedulerCycle() = default;

void SchedulerCycle::next_aa(int nSamples) {  
    // Audio-rate parameters
    const float* rateIn = in(Rate);
    
    // Control-rate parameters
    bool resetTrigger = in0(Reset) > 0.5f;
   
    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {
        // Process event scheduler
        auto event = m_scheduler.process(
            rateIn[i],
            resetTrigger,
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
    mCalcFunc = make_calc_function<SchedulerBurst, &SchedulerBurst::next_aa>();
    next_aa(1);
    m_scheduler.init();
}

SchedulerBurst::~SchedulerBurst() = default;

void SchedulerBurst::next_aa(int nSamples) {    
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
        // Process event scheduler
        auto event = m_scheduler.process(
            initTriggerIn[i] > 0.5f,
            durationIn[i],
            cyclesIn[i],
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
    // Get number of channels and initialize
    m_numChannels = sc_clip(static_cast<int>(in0(NumChannels)), 1, 64);
    m_allocator = Utils::VoiceAllocator(m_numChannels);
    m_allocator.reset();
   
    mCalcFunc = make_calc_function<VoiceAllocator, &VoiceAllocator::next_aa>();
    next_aa(1);
}

VoiceAllocator::~VoiceAllocator() = default;

void VoiceAllocator::next_aa(int nSamples) {
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* rateIn = in(Rate);
    const float* offsetIn = in(SubSampleOffset);
   
    for (int i = 0; i < nSamples; ++i) {
        // Process voice allocator
        m_allocator.process(
            triggerIn[i] > 0.5f,
            rateIn[i],
            offsetIn[i],
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
    mCalcFunc = make_calc_function<RampIntegrator, &RampIntegrator::next_aa>();
    next_aa(1);
}

RampIntegrator::~RampIntegrator() = default;

void RampIntegrator::next_aa(int nSamples) { 
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* rateIn = in(Rate);
    const float* offsetIn = in(SubSampleOffset);
   
    // Output pointer
    float* phaseOut = out(Phase);
   
    for (int i = 0; i < nSamples; ++i) {
        // Process integrator
        phaseOut[i] = m_integrator.process(
            triggerIn[i] > 0.5f,
            rateIn[i],
            offsetIn[i],
            m_sampleRate
        );
    }
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator::RampAccumulator()
{
    mCalcFunc = make_calc_function<RampAccumulator, &RampAccumulator::next_aa>();
    next_aa(1);
}

RampAccumulator::~RampAccumulator() = default;

void RampAccumulator::next_aa(int nSamples) { 
    // Audio-rate parameters
    const float* triggerIn = in(Trigger);
    const float* offsetIn = in(SubSampleOffset);
   
    // Output pointer
    float* countOut = out(Count);
   
    for (int i = 0; i < nSamples; ++i) {
        // Process accumulator
        countOut[i] = m_accumulator.process(
            triggerIn[i] > 0.5f,
            offsetIn[i]
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