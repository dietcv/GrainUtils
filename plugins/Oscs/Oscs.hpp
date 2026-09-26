#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "ShaperUtils.hpp"
#include "OscUtils.hpp"
#include "OversamplingUtils.hpp"
#include "BufferUtils.hpp"

// ===== SINGLE WAVETABLE OSCILLATOR =====

class SingleOscOS : public SCUnit {
public:
    SingleOscOS();
    ~SingleOscOS();
    
private:
    void next(int nSamples);
    
    // Constants cached at construction
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    EventUtils::RampToSlope m_rampToSlope;

    // Wavetables
    BufferUtils::Wavetable m_oscWavetable;

    // Control-rate interpolation
    Utils::ParamInterp m_cyclePosInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osCyclePosInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
    
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

    // Wavetables
    BufferUtils::Wavetable m_oscWavetableA;
    BufferUtils::Wavetable m_oscWavetableB;

    // Control-rate interpolation
    Utils::ParamInterp m_cyclePosAInterp;
    Utils::ParamInterp m_cyclePosBInterp;
    Utils::ParamInterp m_xmIndexAInterp;
    Utils::ParamInterp m_xmIndexBInterp;
    Utils::ParamInterp m_xmFltRatioAInterp;
    Utils::ParamInterp m_xmFltRatioBInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osCyclePosAInterp;
    OversamplingUtils::OSParamInterp m_osCyclePosBInterp;
    OversamplingUtils::OSParamInterp m_osXmIndexAInterp;
    OversamplingUtils::OSParamInterp m_osXmIndexBInterp;
    OversamplingUtils::OSParamInterp m_osXmFltRatioAInterp;
    OversamplingUtils::OSParamInterp m_osXmFltRatioBInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversamplingA;
    OversamplingUtils::VariableOversampling m_outputOversamplingB;
    float* m_outputOSBufferA{nullptr};
    float* m_outputOSBufferB{nullptr};
    
    // Audio rate flags
    bool isCyclePosAAudioRate;
    bool isCyclePosBAAudioRate;
    bool isXmIndexAAudioRate;
    bool isXmIndexBAudioRate;
    bool isXmFltRatioAAudioRate;
    bool isXmFltRatioBAudioRate;
    
    enum InputParams {
        BufNumA,
        PhaseA,
        NumCyclesA,
        CyclePosA,
        
        BufNumB,
        PhaseB,
        NumCyclesB,
        CyclePosB,
        
        XmIndexA,       
        XmIndexB,       
        XmFltRatioA, 
        XmFltRatioB,
        
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
    static constexpr int NUM_VOICES = 8;
    
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
 
    // Core processing
    EventUtils::IsTrigger m_trigger;
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    FilterUtils::DCBlocker m_dcBlocker;

    // Per-voice phase modulation state
    std::array<OscUtils::PMOsc, NUM_VOICES> m_pmOscs;
 
    // Wavetables
    BufferUtils::Wavetable m_oscWavetable;
    BufferUtils::Wavetable m_envWavetable;
    BufferUtils::Wavetable m_modWavetable;

    // Control-rate interpolation
    Utils::ParamInterp m_oscCyclePosInterp;
    Utils::ParamInterp m_envCyclePosInterp;
    Utils::ParamInterp m_modCyclePosInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osOscCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osEnvCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osModCyclePosInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
    
    // Grain data structure
    struct GrainData {
        float oscFreq = 0.0f;
        float modFreq = 0.0f;
        float pmIndex = 0.0f;
        double sampleCount = 0.0;
    };
    std::array<GrainData, NUM_VOICES> m_grainData;

    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isOscFreqAudioRate;
    bool isModFreqAudioRate;
    bool isPmIndexAudioRate;
    bool isOscCyclePosAudioRate;
    bool isEnvCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    
    enum InputParams {
        Trigger,
        TriggerFreq,
        SubSampleOffset,
 
        OscFreq,
        ModFreq,
        PmIndex,
 
        OscBufNum,
        OscNumCycles,
        OscCyclePos,
 
        EnvBufNum,
        EnvNumCycles,
        EnvCyclePos,
 
        ModBufNum,
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
    static constexpr int NUM_VOICES = 8;
 
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
 
    // Core processing
    EventUtils::IsTrigger m_trigger;
    EventUtils::VoiceAllocator<NUM_VOICES> m_allocator;
    FilterUtils::DCBlocker m_dcBlocker;
 
    // Per-voice cross-modulation state
    std::array<OscUtils::DualOsc, NUM_VOICES> m_dualOscs;
 
    // Wavetables
    BufferUtils::Wavetable m_oscWavetable;
    BufferUtils::Wavetable m_modWavetable;

    // Control-rate interpolation
    Utils::ParamInterp m_oscCyclePosInterp;
    Utils::ParamInterp m_modCyclePosInterp;
    Utils::ParamInterp m_envSkewInterp;
    Utils::ParamInterp m_envIndexInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osOscCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osModCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osEnvSkewInterp;
    OversamplingUtils::OSParamInterp m_osEnvIndexInterp;
 
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
 
    // Grain data structure
    struct GrainData {
        float oscFreq = 0.0f;
        float modFreq = 0.0f;
        float oscXmIndex = 0.0f;
        float modXmIndex = 0.0f;
        float oscXmFltRatio = 1.0f;
        float modXmFltRatio = 1.0f;
        float oscWarp = 0.5f;
        float modWarp = 0.5f;
        double sampleCount = 0.0;
    };
    std::array<GrainData, NUM_VOICES> m_grainData;
 
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isOscFreqAudioRate;
    bool isModFreqAudioRate;
    bool isOscXmIndexAudioRate;
    bool isModXmIndexAudioRate;
    bool isOscXmFltRatioAudioRate;
    bool isModXmFltRatioAudioRate;
    bool isOscWarpAudioRate;
    bool isModWarpAudioRate;
    bool isOscCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    bool isEnvSkewAudioRate;
    bool isEnvIndexAudioRate;
 
    enum InputParams {
        Trigger,
        TriggerFreq,
        SubSampleOffset,
 
        OscFreq,
        ModFreq,

        OscXmIndex,
        ModXmIndex,
        OscXmFltRatio,
        ModXmFltRatio,
        
        OscWarp,
        ModWarp,
 
        OscBufNum,
        OscNumCycles,
        OscCyclePos,
 
        ModBufNum,
        ModNumCycles,
        ModCyclePos,
 
        EnvSkew,
        EnvIndex,
 
        Oversample
    };
 
    enum Outputs {
        Out
    };
};