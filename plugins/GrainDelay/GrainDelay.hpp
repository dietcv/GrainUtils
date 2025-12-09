#pragma once
#include "SC_PlugIn.hpp"
#include "EventUtils.hpp"
#include "FilterUtils.hpp"
#include <array>

class GrainDelay : public SCUnit {
public:
    GrainDelay();
    ~GrainDelay();
private:
    void next(int nSamples);
   
    // Constants
    static constexpr int NUM_CHANNELS = 16;
    static constexpr float MAX_DELAY_TIME = 2.0f;
   
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_bufSize;
    const float m_bufFrames;
    const int m_bufMask;
   
    // Core trigger system
    EventUtils::SchedulerCycle m_scheduler;
    EventUtils::VoiceAllocator<NUM_CHANNELS> m_allocator;
    EventUtils::IsTrigger m_resetTrigger;
   
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
    FilterUtils::OnePoleDirect m_dampingFilter;
    FilterUtils::OnePoleHz m_dcBlocker;

    // Cache for SlopeSignal state
    float triggerRatePast, overlapPast, delayTimePast, grainRatePast;
    float mixPast, feedbackPast, dampingPast;
    
    // Audio rate flags
    bool isTriggerRateAudioRate;
    bool isOverlapAudioRate;
    bool isDelayTimeAudioRate;
    bool isGrainRateAudioRate;
    bool isMixAudioRate;
    bool isFeedbackAudioRate;
    bool isDampingAudioRate;
   
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