#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"

// ===== SCHEDULER CYCLE =====

class SchedulerCycle : public SCUnit {
public:
    SchedulerCycle();
    ~SchedulerCycle();

private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    EventUtils::SchedulerCycle m_scheduler;
    EventUtils::IsTrigger m_resetTrigger;
    
    // Audio rate flags
    bool isRateAudioRate;
    bool isResetAudioRate;
    
    enum InputParams {
        Rate,
        Reset
    };
    
    enum Outputs {
        Trigger,
        RateLatched,
        SubSampleOffset,
        Phase
    };
};

// ===== SCHEDULER BURST =====

class SchedulerBurst : public SCUnit {
public:
    SchedulerBurst();
    ~SchedulerBurst();

private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    EventUtils::SchedulerBurst m_scheduler;
    EventUtils::IsTrigger m_initTrigger;
    
    // Audio rate flags
    bool isInitTriggerAudioRate;
    bool isDurationAudioRate;
    bool isCyclesAudioRate;
    
    enum InputParams {
        InitTrigger,
        Duration,
        Cycles
    };
    
    enum Outputs {
        Trigger,
        RateLatched,
        SubSampleOffset,
        Phase
    };
};

// ===== VOICE ALLOCATOR =====

class VoiceAllocator : public SCUnit {
public:
    VoiceAllocator();
    ~VoiceAllocator();

private:
    void next(int nSamples);
   
    // Constants
    static constexpr int MAX_CHANNELS = 64;
    const float m_sampleRate;
    const int m_numChannels;
    
    // Core processing
    EventUtils::VoiceAllocator<MAX_CHANNELS> m_allocator;
    EventUtils::IsTrigger m_trigger;
    
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isRateAudioRate;
    bool isSubSampleOffsetAudioRate;
   
    enum InputParams {
        NumChannels,
        Trigger,
        Rate,
        SubSampleOffset
    };
   
    // Outputs: numChannels phases and triggers
    // Output indices are calculated dynamically based on m_numChannels
};

// ===== RAMP INTEGRATOR =====

class RampIntegrator : public SCUnit {
public:
    RampIntegrator();
    ~RampIntegrator();

private:
    void next(int nSamples);
   
    // Constants
    const float m_sampleRate;
   
    // Core processing
    EventUtils::RampIntegrator m_integrator;
    EventUtils::IsTrigger m_trigger;
    
    // Cache for SlopeSignal state
    float ratePast;
    
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isRateAudioRate;
    bool isSubSampleOffsetAudioRate;
   
    enum InputParams {
        Trigger,
        Rate,
        SubSampleOffset
    };
   
    enum Outputs {
        Phase
    };
};

// ===== RAMP ACCUMULATOR =====

class RampAccumulator : public SCUnit {
public:
    RampAccumulator();
    ~RampAccumulator();

private:
    void next(int nSamples);
   
    // Core processing
    EventUtils::RampAccumulator m_accumulator;
    EventUtils::IsTrigger m_trigger;
    
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isSubSampleOffsetAudioRate;
   
    enum InputParams {
        Trigger,
        SubSampleOffset
    };
   
    enum Outputs {
        Count
    };
};