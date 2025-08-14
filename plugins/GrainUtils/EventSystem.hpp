#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"

// ===== EVENT SCHEDULER =====

class EventScheduler : public SCUnit {
public:
    EventScheduler();
    ~EventScheduler();

private:
    void next_aa(int nSamples);
    void reset();
   
    // Core processing
    Utils::EventScheduler m_scheduler;

    // Constants
    const float m_sampleRate;
   
    // Input parameters
    enum InputParams {
        TriggerRate,    // Trigger rate in Hz
        Reset,          // Reset trigger
       
        NumInputParams
    };
   
    enum Outputs {
        Trigger,            // derived trigger
        Rate,               // derived and latched frequency in hz
        SubSampleOffset,    // sub-sample offset
       
        NumOutputParams
    };
};

// ===== VOICE ALLOCATOR =====

class VoiceAllocator : public SCUnit {
public:
    VoiceAllocator();
    ~VoiceAllocator();

private:
    void next_aa(int nSamples);
    void reset();
   
    // Core processing
    Utils::VoiceAllocator m_allocator;

    // Constants
    const float m_sampleRate;
    int m_numChannels;
   
    // Input parameters
    enum InputParams {
        NumChannels,    // Number of output channels (init-rate)
        Trigger,        // Trigger input from EventScheduler
        Rate,           // Rate from EventScheduler / overlap
        SubSampleOffset,// Subsample offset from EventScheduler
       
        NumInputParams
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
    void next_aa(int nSamples);
    void reset();
   
    // Core processing
    Utils::RampIntegrator m_integrator;
    
    // Constants
    const float m_sampleRate;
   
    // Input parameters
    enum InputParams {
        Trigger,            // Trigger input
        Rate,               // Rate in Hz
        SubSampleOffset,    // Subsample offset
       
        NumInputParams
    };
   
    enum Outputs {
        Phase,          // Wrapped phase output [0,1)
       
        NumOutputParams
    };
};