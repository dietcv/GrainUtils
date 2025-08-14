#include "EventSystem.hpp"

// ===== EVENT SCHEDULER =====

EventScheduler::EventScheduler() : m_sampleRate(static_cast<float>(sampleRate()))
{
    m_scheduler.reset();
    mCalcFunc = make_calc_function<EventScheduler, &EventScheduler::next_aa>();
    next_aa(1);
}

EventScheduler::~EventScheduler() = default;

void EventScheduler::next_aa(int nSamples) {
    // Input parameters
    const float* triggerRateIn = in(TriggerRate);
    const bool resetTrigger = in0(Reset) > 0.5f;
   
    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(Rate);
    float* offsetOut = out(SubSampleOffset);
   
    for (int i = 0; i < nSamples; ++i) {
        // Process event scheduler
        auto result = m_scheduler.process(
            triggerRateIn[i],
            resetTrigger,
            m_sampleRate
        );
        // Output values
        triggerOut[i] = result.trigger;
        rateOut[i] = result.rate;
        offsetOut[i] = result.subSampleOffset;
    }
}

void EventScheduler::reset() {
    m_scheduler.reset();
}

// ===== VOICE ALLOCATOR =====

VoiceAllocator::VoiceAllocator() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Get number of channels and initialize
    m_numChannels = sc_clip(static_cast<int>(in0(NumChannels)), 1, 32);
    m_allocator = Utils::VoiceAllocator(m_numChannels);
    m_allocator.reset();
   
    mCalcFunc = make_calc_function<VoiceAllocator, &VoiceAllocator::next_aa>();
    next_aa(1);
}

VoiceAllocator::~VoiceAllocator() = default;

void VoiceAllocator::next_aa(int nSamples) {
    // Input parameters
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
            out(ch)[i] = m_allocator.outputPhases[ch];
            out(m_numChannels + ch)[i] = m_allocator.outputTriggers[ch];
        }
    }
}

void VoiceAllocator::reset() {
    m_allocator.reset();
}

// ===== RAMP INTEGRATOR =====

RampIntegrator::RampIntegrator() : m_sampleRate(static_cast<float>(sampleRate()))
{
    m_integrator.reset();
    mCalcFunc = make_calc_function<RampIntegrator, &RampIntegrator::next_aa>();
    next_aa(1);
}

RampIntegrator::~RampIntegrator() = default;

void RampIntegrator::next_aa(int nSamples) {
    // Input parameters
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

void RampIntegrator::reset() {
    m_integrator.reset();
}