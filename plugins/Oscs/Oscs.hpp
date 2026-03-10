#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "OscUtils.hpp"
#include "OversamplingUtils.hpp"

// ===== SINGLE WAVETABLE OSCILLATOR =====

class SingleOscOS : public SCUnit {
public:
    SingleOscOS();
    ~SingleOscOS();
    
private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    EventUtils::RampToSlope m_rampToSlope;
    OscUtils::BufUnit m_bufUnit;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling<4> m_outputOversampling;
    OversamplingUtils::VariableOversampling<4> m_cyclePosOversampling;
    
    // Stored oversampling state
    int m_oversampleIndex;
    int m_osRatio;
    float* m_outputOSBuffer;
    float* m_osCyclePosBuffer;
    
    // Cache for SlopeSignal state
    float cyclePosPast;
    
    // Audio rate flags
    bool isCyclePosAudioRate;
    
    enum InputParams {
        BufNum,
        Phase,
        NumCycles,
        CyclePos,
        Oversample
    };
    
    enum Outputs { 
        Out
    };
};

// ===== DUAL WAVETABLE OSCILLATOR =====

class DualOscOS : public SCUnit {
public:
    DualOscOS();
    ~DualOscOS();
    
private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    EventUtils::RampToSlope m_rampToSlopeA;
    EventUtils::RampToSlope m_rampToSlopeB;
    OscUtils::DualOsc m_dualOsc;
    OscUtils::BufUnit m_bufUnitA;
    OscUtils::BufUnit m_bufUnitB;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling<4> m_outputOversamplingA;
    OversamplingUtils::VariableOversampling<4> m_outputOversamplingB;
    OversamplingUtils::VariableOversampling<4> m_cyclePosAOversampling;     
    OversamplingUtils::VariableOversampling<4> m_cyclePosBOversampling;   
    OversamplingUtils::VariableOversampling<4> m_pmIndexAOversampling;    
    OversamplingUtils::VariableOversampling<4> m_pmIndexBOversampling;       
    OversamplingUtils::VariableOversampling<4> m_pmFilterRatioAOversampling; 
    OversamplingUtils::VariableOversampling<4> m_pmFilterRatioBOversampling;
    
    // Stored oversampling state
    int m_oversampleIndex;
    int m_osRatio;
    float* m_outputOSBufferA;
    float* m_outputOSBufferB;
    float* m_osCyclePosABuffer;
    float* m_osCyclePosBBuffer;
    float* m_osPMIndexABuffer;
    float* m_osPMIndexBBuffer;
    float* m_osPMFilterRatioABuffer;
    float* m_osPMFilterRatioBBuffer;
        
    // Cache for SlopeSignal state
    float cyclePosAPast, cyclePosBPast;
    float pmIndexAPast, pmIndexBPast, pmFilterRatioAPast, pmFilterRatioBPast;
    
    // Audio rate flags
    bool isCyclePosAAudioRate;
    bool isCyclePosBAAudioRate;
    bool isPMIndexAAudioRate;
    bool isPMIndexBAudioRate;
    bool isPMFilterRatioAAudioRate;
    bool isPMFilterRatioBAudioRate;
    
    enum InputParams {
        // Oscillator A
        BufNumA,
        PhaseA,
        NumCyclesA,
        CyclePosA,
        
        // Oscillator B  
        BufNumB,
        PhaseB,
        NumCyclesB,
        CyclePosB,
        
        // Cross-modulation parameters
        PMIndexA,       
        PMIndexB,       
        PMFilterRatioA, 
        PMFilterRatioB,
        
        // Global parameters
        Oversample
    };
    
    enum Outputs { 
        OutA, 
        OutB    
    };
};

// ===== PULSAR OSCILLATOR =====

class PulsarOS : public SCUnit {
public:
    PulsarOS();
    ~PulsarOS();
    
private:
    void next(int nSamples);
    
    // Constants
    static constexpr int NUM_VOICES = 16;
    static constexpr int MAX_CHANNELS = 2;
    
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_numChannels;

    // Core processing
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    EventUtils::IsTrigger m_trigger;
    std::array<FilterUtils::OnePoleSlope, NUM_VOICES> m_pmFilters;

    // Buffer units
    OscUtils::BufUnit m_oscBufUnit;
    OscUtils::BufUnit m_envBufUnit;
    OscUtils::BufUnit m_modBufUnit;
    
    // Oversampling
    OversamplingUtils::VariableOversampling<4> m_outputOversamplingL;
    OversamplingUtils::VariableOversampling<4> m_outputOversamplingR;
    OversamplingUtils::VariableOversampling<4> m_oscCyclePosOversampling;
    OversamplingUtils::VariableOversampling<4> m_envCyclePosOversampling;
    OversamplingUtils::VariableOversampling<4> m_modCyclePosOversampling;
    int m_oversampleIndex;
    int m_osRatio;
    float* m_outputOSBufferL;
    float* m_outputOSBufferR;
    float* m_osOscCyclePosBuffer;
    float* m_osEnvCyclePosBuffer;
    float* m_osModCyclePosBuffer;
    
    // Grain data structure
    struct GrainData {
        float grainFreq = 0.0f;
        float modFreq = 0.0f;
        float modIndex = 0.0f;
        float pan = 0.0f;
        float amp = 0.0f;
        double sampleCount = 0.0;
    };
    
    // Grain voices
    std::array<GrainData, NUM_VOICES> m_grainData;
    
    // Output processing
    FilterUtils::OnePoleHz m_dcBlockerL;
    FilterUtils::OnePoleHz m_dcBlockerR;
    
    // Cache for SlopeSignal state
    float oscCyclePosPast;
    float envCyclePosPast;
    float modCyclePosPast;
    
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isGrainFreqAudioRate;
    bool isPanAudioRate;
    bool isAmpAudioRate;
    bool isModFreqAudioRate;
    bool isModIndexAudioRate;
    bool isOscCyclePosAudioRate;
    bool isEnvCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    
    enum InputParams {
        NumChannels,

        Trigger,
        TriggerFreq,
        SubSampleOffset,

        GrainFreq,
        ModFreq,
        ModIndex,
        Pan,
        Amp,

        OscBuffer,
        OscNumCycles,
        OscCyclePos,

        EnvBuffer,
        EnvNumCycles,
        EnvCyclePos,

        ModBuffer,
        ModNumCycles,
        ModCyclePos,
        
        Oversample
    };
    
    enum Outputs {
        OutL,
        OutR
    };
};


