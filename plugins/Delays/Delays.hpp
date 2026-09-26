#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "FilterUtils.hpp"
#include "BufferUtils.hpp"
#include <array>

class GrainDelay : public SCUnit {
public:
    GrainDelay();
    ~GrainDelay();
 
private:
    void next(int nSamples);
    
    // Constants
    static constexpr int NUM_VOICES = 16;
    static constexpr float MAX_DELAY_TIME = 2.0f;
    
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_bufSize;
    const float m_bufFrames;
    const int m_bufMask;
    
    // Core trigger system
    EventUtils::IsTrigger m_resetTrigger;
    EventUtils::SchedulerCycle m_scheduler;
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    FilterUtils::DampingFilter m_dampingFilter;
    FilterUtils::DCBlocker m_dcBlocker;

    // Control-rate interpolation
    Utils::ParamInterp m_delayTimeInterp;
    Utils::ParamInterp m_mixInterp;
    Utils::ParamInterp m_feedbackInterp;
    Utils::ParamInterp m_dampingInterp;

    // Audio buffer and processing
    float *m_buffer{nullptr};
    int m_writePos = 0;
    
    // Grain data structure
    struct GrainData {
        float readPos = 0.0f;
        float rate = 1.0f;
        float sampleCount = 0.0f;
    };
    std::array<GrainData, NUM_VOICES> m_grainData;
    
    // Audio rate flags
    bool isTriggerRateAudioRate;
    bool isOverlapAudioRate;
    bool isDelayTimeAudioRate;
    bool isGrainRateAudioRate;
    bool isMixAudioRate;
    bool isFeedbackAudioRate;
    bool isDampingAudioRate;
    bool isFreezeAudioRate;
    bool isResetAudioRate;
    
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