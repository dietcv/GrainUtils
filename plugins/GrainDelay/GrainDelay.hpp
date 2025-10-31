#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include <array>

class GrainDelay : public SCUnit {
public:
    GrainDelay();
    ~GrainDelay();
private:
    void next_aa(int nSamples);
   
    // Constants
    static constexpr int NUM_CHANNELS = 32;
    static constexpr float MAX_DELAY_TIME = 5.0f;
   
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_bufSize;
    const float m_bufFrames;
    const int m_bufMask;
   
    // Core trigger system
    Utils::SchedulerCycle m_scheduler;
    Utils::VoiceAllocator<NUM_CHANNELS> m_allocator;
    Utils::IsTrigger m_resetTrigger;
   
    // Audio buffer and processing
    float *m_buffer;
    int m_writePos = 0;
   
    // Grain data structure
    struct GrainData {
        float readPos = 0.0f;
        float rate = 1.0f;
        float sampleCount = 0.0f;
        bool hasTriggered = false;
    };
   
    // Grain voices
    std::array<GrainData, NUM_CHANNELS> m_grainData;
   
    // Feedback processing filters
    Utils::OnePole m_dampingFilter;
    Utils::OnePoleHz m_dcBlocker;
   
    // Input parameters for audio processing
    enum InputParams {
        Input,
        TriggerRate,
        Overlap,
        DelayTime,
        GrainRate,
        Mix,
        Feedback,
        Damping,
        Freeze,
        Reset
    };
   
    enum Outputs {
        Output        
    };
};