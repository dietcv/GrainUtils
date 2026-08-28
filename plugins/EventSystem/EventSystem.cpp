#include "EventSystem.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== SCHEDULER CYCLE =====

SchedulerCycle::SchedulerCycle() : 
    m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isRateAudioRate = isAudioRateIn(Rate);
    isResetAudioRate = isAudioRateIn(Reset);
    
    // Set calc function & compute initial sample
    set_calc_function<SchedulerCycle, &SchedulerCycle::next>();

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
            sc_clip(in(Rate)[i], 0.0f, m_sampleRate * 0.49f) : 
            sc_clip(in0(Rate), 0.0f, m_sampleRate * 0.49f);
        
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

SchedulerBurst::SchedulerBurst() : 
    m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isInitTriggerAudioRate = isAudioRateIn(InitTrigger);
    isDurationAudioRate = isAudioRateIn(Duration);
    isCyclesAudioRate = isAudioRateIn(Cycles);
    
    // Set calc function & compute initial sample
    set_calc_function<SchedulerBurst, &SchedulerBurst::next>();

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
            sc_max(in(Duration)[i], 0.0f) : 
            sc_max(in0(Duration), 0.0f);
        
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

// ===== SCHEDULER BANK =====

SchedulerBank::SchedulerBank() :
    m_sampleRate(static_cast<float>(sampleRate())),
    m_numChannels(sc_clip(static_cast<int>(in0(NumChannels)), 2, MAX_CHANNELS))
{
    // Check which inputs are audio-rate
    isRateAudioRate = isAudioRateIn(Rate);
    isSpreadAudioRate = isAudioRateIn(Spread);
    isCoupleAudioRate = isAudioRateIn(Couple);
    isBiasAudioRate = isAudioRateIn(Bias);
    isResetAudioRate = isAudioRateIn(Reset);

    // Set calc function & compute initial sample
    set_calc_function<SchedulerBank, &SchedulerBank::next>();

    // Reset state after priming
    m_bank.reset();
    m_resetTrigger.reset();
}

SchedulerBank::~SchedulerBank() = default;

void SchedulerBank::next(int nSamples) {
 
    for (int i = 0; i < nSamples; ++i) {
 
        // Get current parameter values (no interpolation - latched per trigger)
        float rate = isRateAudioRate ?
            sc_clip(in(Rate)[i], 0.0f, m_sampleRate * 0.49f) :
            sc_clip(in0(Rate), 0.0f, m_sampleRate * 0.49f);
 
        float spread = isSpreadAudioRate ?
            sc_clip(in(Spread)[i], 0.0f, 1.0f) :
            sc_clip(in0(Spread), 0.0f, 1.0f);
 
        float couple = isCoupleAudioRate ?
            sc_clip(in(Couple)[i], 0.0f, 1.0f) :
            sc_clip(in0(Couple), 0.0f, 1.0f);
 
        float bias = isBiasAudioRate ?
            sc_clip(in(Bias)[i], -1.0f, 1.0f) :
            sc_clip(in0(Bias), -1.0f, 1.0f);
 
        // Trigger input (audio-rate or control-rate)
        bool reset = isResetAudioRate ?
            m_resetTrigger.process(in(Reset)[i]) :
            m_resetTrigger.process(in0(Reset));
 
        // Process scheduler bank
        auto events = m_bank.process(
            m_numChannels,
            rate,
            spread,
            couple,
            bias,
            reset,
            m_sampleRate
        );
 
        // Output triggers, rates, offsets and phases
        for (int ch = 0; ch < m_numChannels; ++ch) {
            out(ch)[i] = events.triggers[ch];
            out(m_numChannels + ch)[i] = events.rates[ch];
            out(2 * m_numChannels + ch)[i] = events.subSampleOffsets[ch];
            out(3 * m_numChannels + ch)[i] = events.phases[ch];
        }
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
    
    // Set calc function & compute initial sample
    set_calc_function<VoiceAllocator, &VoiceAllocator::next>();

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
            sc_clip(in(Rate)[i], 0.0f, m_sampleRate * 0.49f) : 
            sc_clip(in0(Rate), 0.0f, m_sampleRate * 0.49f);
        
        float offset = isSubSampleOffsetAudioRate ? 
            in(SubSampleOffset)[i] : 
            in0(SubSampleOffset);

        // Process voice allocator
        auto voices = m_allocator.process(
            m_numChannels,
            trigger, 
            rate, 
            offset, 
            m_sampleRate
        );

        // Output phases and triggers
        for (int ch = 0; ch < m_numChannels; ++ch) {
            out(ch)[i] = voices.phases[ch];
            out(m_numChannels + ch)[i] = voices.triggers[ch];
        }
    }
}

// ===== RAMP INTEGRATOR =====

RampIntegrator::RampIntegrator() : 
    m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    ratePast = sc_clip(in0(Rate), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
    
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isRateAudioRate = isAudioRateIn(Rate);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    
    // Set calc function & compute initial sample
    set_calc_function<RampIntegrator, &RampIntegrator::next>();

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
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    ratePast = isRateAudioRate ? 
        sc_clip(in(Rate)[nSamples - 1], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
        slopedRate.value;
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator::RampAccumulator()
{
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    
    // Set calc function & compute initial sample
    set_calc_function<RampAccumulator, &RampAccumulator::next>();
    
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

// ===== RAMP DIVIDER =====

RampDivider::RampDivider() :
    m_mode(sc_clip(static_cast<int>(in0(Mode)), 0, 2))
{
    // Initialize parameter cache
    ratioPast = in0(Ratio);
    
    // Check which inputs are audio-rate
    isRatioAudioRate = isAudioRateIn(Ratio);
    isResetAudioRate = isAudioRateIn(Reset);
    
    // Set calc function & compute initial sample
    set_calc_function<RampDivider, &RampDivider::next>();
    
    // Reset state after priming
    m_simpleDivider.reset();
    m_gridDivider.reset();
    m_offsetDivider.reset();
    m_resetTrigger.reset();
}

RampDivider::~RampDivider() = default;

void RampDivider::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedRatio = makeSlope(in0(Ratio), ratioPast);
    
    // Output pointer
    float* phaseOut = out(PhaseOut);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        double phase = sc_frac(static_cast<double>(phaseIn[i]));
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float ratio = isRatioAudioRate ? 
            in(Ratio)[i] : 
            slopedRatio.consume();
        
        // Trigger input (audio-rate or control-rate)
        bool reset = isResetAudioRate ? 
            m_resetTrigger.process(in(Reset)[i]) : 
            m_resetTrigger.process(in0(Reset));
        
        // Process divider based on mode
        switch (m_mode) {
            case 0: phaseOut[i] = m_simpleDivider.process(phase, ratio, reset); break;
            case 1: phaseOut[i] = m_gridDivider.process(phase, ratio, reset);   break;
            case 2: phaseOut[i] = m_offsetDivider.process(phase, ratio, reset); break;
        }
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    ratioPast = isRatioAudioRate ? 
        in(Ratio)[nSamples - 1] : 
        slopedRatio.value;
}

void EventSystem_setup() 
{
    registerUnit<SchedulerCycle>(ft, "SchedulerCycleUGen", false);
    registerUnit<SchedulerBurst>(ft, "SchedulerBurstUGen", false);
    registerUnit<SchedulerBank>(ft, "SchedulerBankUGen", false);
    registerUnit<VoiceAllocator>(ft, "VoiceAllocatorUGen", false);
    registerUnit<RampIntegrator>(ft, "RampIntegrator", false);
    registerUnit<RampAccumulator>(ft, "RampAccumulator", false);
    registerUnit<RampDivider>(ft, "RampDivider", false);
}