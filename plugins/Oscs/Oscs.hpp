#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "ShaperUtils.hpp"
#include "OscUtils.hpp"
#include "OversamplingUtils.hpp"
#include "PluginUtils.hpp"

// ===== SINGLE WAVETABLE OSCILLATOR =====

class SingleOscOS : public SCUnit {
public:
    SingleOscOS();
    ~SingleOscOS();
    
private:
    void next(int nSamples);
    
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    EventUtils::RampToSlope m_rampToSlope;

    // Buffer units
    PluginUtils::BufUnit m_oscBufUnit;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling m_outputOversampling;
    OversamplingUtils::VariableOversampling m_cyclePosOversampling;

    // Stored oversampling state
    float* m_outputOSBuffer{nullptr};
    float* m_cyclePosOSBuffer{nullptr};
    
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
    
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    EventUtils::RampToSlope m_rampToSlopeA;
    EventUtils::RampToSlope m_rampToSlopeB;
    OscUtils::DualOsc m_dualOsc;

    // Buffer units
    PluginUtils::BufUnit m_oscBufUnitA;
    PluginUtils::BufUnit m_oscBufUnitB;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling m_outputOversamplingA;
    OversamplingUtils::VariableOversampling m_outputOversamplingB;
    OversamplingUtils::VariableOversampling m_cyclePosAOversampling;
    OversamplingUtils::VariableOversampling m_cyclePosBOversampling;
    OversamplingUtils::VariableOversampling m_pmIndexAOversampling;
    OversamplingUtils::VariableOversampling m_pmIndexBOversampling;
    OversamplingUtils::VariableOversampling m_pmFilterRatioAOversampling;
    OversamplingUtils::VariableOversampling m_pmFilterRatioBOversampling;
    
    // Stored oversampling state
    float* m_outputOSBufferA{nullptr};
    float* m_outputOSBufferB{nullptr};
    float* m_cyclePosAOSBuffer{nullptr};
    float* m_cyclePosBOSBuffer{nullptr};
    float* m_pmIndexAOSBuffer{nullptr};
    float* m_pmIndexBOSBuffer{nullptr};
    float* m_pmFilterRatioAOSBuffer{nullptr};
    float* m_pmFilterRatioBOSBuffer{nullptr};
        
    // Cache for SlopeSignal state
    float cyclePosAPast;
    float cyclePosBPast;
    float pmIndexAPast;
    float pmIndexBPast;
    float pmFilterRatioAPast;
    float pmFilterRatioBPast;
    
    // Audio rate flags
    bool isCyclePosAAudioRate;
    bool isCyclePosBAAudioRate;
    bool isPMIndexAAudioRate;
    bool isPMIndexBAudioRate;
    bool isPMFilterRatioAAudioRate;
    bool isPMFilterRatioBAudioRate;
    
    enum InputParams {
        BufNumA,
        PhaseA,
        NumCyclesA,
        CyclePosA,
        
        BufNumB,
        PhaseB,
        NumCyclesB,
        CyclePosB,
        
        PMIndexA,       
        PMIndexB,       
        PMFilterRatioA, 
        PMFilterRatioB,
        
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
    
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_oversampleIndex;
    const int m_osRatio;
 
    // Core processing
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    EventUtils::IsTrigger m_trigger;
    std::array<FilterUtils::OnePoleSlope, NUM_VOICES> m_pmFilters;
 
    // Buffer units
    PluginUtils::BufUnit m_oscBufUnit;
    PluginUtils::BufUnit m_envBufUnit;
    PluginUtils::BufUnit m_modBufUnit;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling m_outputOversampling;
    OversamplingUtils::VariableOversampling m_oscCyclePosOversampling;
    OversamplingUtils::VariableOversampling m_envCyclePosOversampling;
    OversamplingUtils::VariableOversampling m_modCyclePosOversampling;

    // Stored oversampling state
    float* m_outputOSBuffer{nullptr};
    float* m_oscCyclePosOSBuffer{nullptr};
    float* m_envCyclePosOSBuffer{nullptr};
    float* m_modCyclePosOSBuffer{nullptr};
    
    // Grain data structure
    struct GrainData {
        float oscFreq = 0.0f;
        float modFreq = 0.0f;
        float modIndex = 0.0f;
        double sampleCount = 0.0;
    };
    
    // Grain voices
    std::array<GrainData, NUM_VOICES> m_grainData;
    
    // Output processing
    FilterUtils::OnePoleHz m_dcBlocker;
    
    // Cache for SlopeSignal state
    float oscCyclePosPast;
    float envCyclePosPast;
    float modCyclePosPast;
    
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isOscFreqAudioRate;
    bool isModFreqAudioRate;
    bool isModIndexAudioRate;
    bool isOscCyclePosAudioRate;
    bool isEnvCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    
    enum InputParams {
        Trigger,
        TriggerFreq,
        SubSampleOffset,
 
        OscFreq,
        ModFreq,
        ModIndex,
 
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
        Out
    };
};

// ===== DUAL PULSAR OSCILLATOR =====
 
class DualPulsarOS : public SCUnit {
public:
    DualPulsarOS();
    ~DualPulsarOS();
 
private:
    void next(int nSamples);
 
    // Constants
    static constexpr int NUM_VOICES = 16;
 
    // Constants cached at construction
    const float m_sampleRate;
    const float m_sampleDur;
    const int m_oversampleIndex;
    const int m_osRatio;
 
    // Core processing
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    EventUtils::IsTrigger m_trigger;
 
    // Per-voice cross-modulation state
    std::array<OscUtils::DualOscScaled, NUM_VOICES> m_dualOscs;
 
    // Buffer units
    PluginUtils::BufUnit m_oscBufUnit;
    PluginUtils::BufUnit m_modBufUnit;
 
    // Oversampling objects
    OversamplingUtils::VariableOversampling m_outputOversampling;
    OversamplingUtils::VariableOversampling m_oscCyclePosOversampling;
    OversamplingUtils::VariableOversampling m_modCyclePosOversampling;
    OversamplingUtils::VariableOversampling m_skewOversampling;
    OversamplingUtils::VariableOversampling m_indexOversampling;

    // Stored oversampling state
    float* m_outputOSBuffer{nullptr};
    float* m_oscCyclePosOSBuffer{nullptr};
    float* m_modCyclePosOSBuffer{nullptr};
    float* m_skewOSBuffer{nullptr};
    float* m_indexOSBuffer{nullptr};
 
    // Grain data structure
    struct GrainData {
        float oscFreq = 0.0f;
        float modFreq = 0.0f;
        float pmIndexOsc = 0.0f;
        float pmIndexMod = 0.0f;
        float pmFilterRatioOsc = 1.0f;
        float pmFilterRatioMod = 1.0f;
        float warpOsc = 0.5f;
        float warpMod = 0.5f;
        double sampleCount = 0.0;
    };
    std::array<GrainData, NUM_VOICES> m_grainData;
 
    // Output processing
    FilterUtils::OnePoleHz m_dcBlocker;
 
    // Cache for SlopeSignal state
    float oscCyclePosPast;
    float modCyclePosPast;
    float skewPast;
    float indexPast;
 
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isOscFreqAudioRate;
    bool isModFreqAudioRate;
    bool isPmIndexOscAudioRate;
    bool isPmIndexModAudioRate;
    bool isPmFilterRatioOscAudioRate;
    bool isPmFilterRatioModAudioRate;
    bool isWarpOscAudioRate;
    bool isWarpModAudioRate;
    bool isOscCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    bool isSkewAudioRate;
    bool isIndexAudioRate;
 
    enum InputParams {
        Trigger,
        TriggerFreq,
        SubSampleOffset,
 
        OscFreq,
        ModFreq,
        PmIndexOsc,
        PmIndexMod,
        PmFilterRatioOsc,
        PmFilterRatioMod,
        WarpOsc,
        WarpMod,
 
        OscBuffer,
        OscNumCycles,
        OscCyclePos,
 
        ModBuffer,
        ModNumCycles,
        ModCyclePos,
 
        Skew,
        Index,
 
        Oversample
    };
 
    enum Outputs {
        Out
    };
};


