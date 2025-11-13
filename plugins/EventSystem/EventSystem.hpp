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
   
    // Core processing
    EventUtils::SchedulerCycle m_scheduler;
    EventUtils::IsTrigger m_resetTrigger;
    
    // Constants
    const float m_sampleRate;
   
    // Input parameters
    enum InputParams {
        Rate,           // Trigger rate in Hz
        Reset,          // Reset trigger
    };
   
    enum Outputs {
        Trigger,            // derived trigger
        RateLatched,        // derived and latched frequency in hz
        SubSampleOffset,    // sub-sample offset
        Phase,            
    };
};

// ===== SCHEDULER BURST =====

class SchedulerBurst : public SCUnit {
public:
    SchedulerBurst();
    ~SchedulerBurst();
   
private:
    void next(int nSamples);
   
    // Core processing
    EventUtils::SchedulerBurst m_scheduler;
    EventUtils::IsTrigger m_initTrigger;
   
    // Constants
    const float m_sampleRate;
   
    // Input parameters
    enum InputParams {
        InitTrigger,    // Trigger to start sequence
        Duration,       // Duration of one cycle
        Cycles          // Number of cycles/events to generate
    };
   
    enum Outputs {
        Trigger,            // Event triggers
        RateLatched,        // Event rate in Hz
        SubSampleOffset,    // sub-sample offset
        Phase               // Event phase [0,cycles)
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
    
    // Core processing
    EventUtils::VoiceAllocator<MAX_CHANNELS> m_allocator;
    EventUtils::IsTrigger m_trigger;
    
    // Runtime state
    const float m_sampleRate;
    int m_numChannels;
   
    // Input parameters
    enum InputParams {
        NumChannels,        // Number of output channels (init-rate)
        Trigger,            // Trigger input from EventScheduler
        Rate,               // Rate from EventScheduler / overlap
        SubSampleOffset     // Subsample offset from EventScheduler
    };
   
    // Outputs: numChannels phases and triggers
    // Output indices are calculated dynamically based on numChannels
};

// ===== RAMP INTEGRATOR =====

class RampIntegrator : public SCUnit {
public:
    RampIntegrator();
    ~RampIntegrator();
private:
    void next(int nSamples);
   
    // Core processing
    EventUtils::RampIntegrator m_integrator;
    EventUtils::IsTrigger m_trigger;
   
    // Constants
    const float m_sampleRate;
   
    // Input parameters
    enum InputParams {
        Trigger,            // Trigger input
        Rate,               // Rate in Hz
        SubSampleOffset     // Subsample offset
    };
   
    enum Outputs {
        Phase               // Wrapped phase output [0,1)
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
   
    // Input parameters
    enum InputParams {
        Trigger,            // Trigger input
        SubSampleOffset     // Subsample offset
    };
   
    enum Outputs {
        Count               // Sample count output
    };
};