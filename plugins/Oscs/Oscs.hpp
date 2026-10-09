#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "ShaperUtils.hpp"
#include "OscUtils.hpp"
#include "OversamplingUtils.hpp"
#include "BufferUtils.hpp"

// ===== WAVETABLE OSCILLATOR WITH CROSS-PHASE MODULATION =====

class OscXPM : public SCUnit {
public:
    OscXPM();
    ~OscXPM();
    
private:
    void next(int nSamples);
    
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    EventUtils::RampToSlope m_oscRampToSlope;
    EventUtils::RampToSlope m_modRampToSlope;
    OscUtils::WavetableOscXPM m_xpmOsc;

    // Wavetables
    BufferUtils::Wavetable m_oscWavetable;
    BufferUtils::Wavetable m_modWavetable;

    // Control-rate interpolation
    Utils::ParamInterp m_oscPmIndexInterp;
    Utils::ParamInterp m_modPmIndexInterp;
    Utils::ParamInterp m_oscPmDampingInterp;
    Utils::ParamInterp m_modPmDampingInterp;
    Utils::ParamInterp m_oscCyclePosInterp;
    Utils::ParamInterp m_modCyclePosInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osOscPmIndexInterp;
    OversamplingUtils::OSParamInterp m_osModPmIndexInterp;
    OversamplingUtils::OSParamInterp m_osOscPmDampingInterp;
    OversamplingUtils::OSParamInterp m_osModPmDampingInterp;
    OversamplingUtils::OSParamInterp m_osOscCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osModCyclePosInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
    
    // Audio rate flags
    bool isOscPmIndexAudioRate;
    bool isModPmIndexAudioRate;
    bool isOscPmDampingAudioRate;
    bool isModPmDampingAudioRate;
    bool isOscCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    
    enum InputParams {
        OscPhase,
        ModPhase,

        OscPmIndex,       
        ModPmIndex,       
        OscPmDamping, 
        ModPmDamping,

        OscBufNum,
        OscNumCycles,
        OscCyclePos,
        
        ModBufNum,
        ModNumCycles,
        ModCyclePos,
    
        Oversample
    };
    
    enum Outputs { 
        Out
    };
};

// ===== PULSAR OSCILLATOR WITH CROSS-PHASE MODULATION =====
 
class PulsarXPM : public SCUnit {
public:
    PulsarXPM();
    ~PulsarXPM();
 
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
 
    // Per-voice cross-phase modulation state
    std::array<OscUtils::WavetableOscXPM, NUM_VOICES> m_xpmOscs;
 
    // Wavetables
    BufferUtils::Wavetable m_oscWavetable;
    BufferUtils::Wavetable m_modWavetable;
    BufferUtils::Wavetable m_envWavetable;

    // Control-rate interpolation
    Utils::ParamInterp m_oscPmIndexInterp;
    Utils::ParamInterp m_modPmIndexInterp;
    Utils::ParamInterp m_oscPmDampingInterp;
    Utils::ParamInterp m_modPmDampingInterp;
    Utils::ParamInterp m_oscCyclePosInterp;
    Utils::ParamInterp m_modCyclePosInterp;
    Utils::ParamInterp m_envCyclePosInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osOscPmIndexInterp;
    OversamplingUtils::OSParamInterp m_osModPmIndexInterp;
    OversamplingUtils::OSParamInterp m_osOscPmDampingInterp;
    OversamplingUtils::OSParamInterp m_osModPmDampingInterp;
    OversamplingUtils::OSParamInterp m_osOscCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osModCyclePosInterp;
    OversamplingUtils::OSParamInterp m_osEnvCyclePosInterp;
 
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
 
    // Grain data structure
    struct GrainData {
        float oscFreq = 0.0f;
        float modFreq = 0.0f;
        double sampleCount = 0.0;
    };
    std::array<GrainData, NUM_VOICES> m_grainData;
 
    // Audio rate flags
    bool isTriggerAudioRate;
    bool isTriggerFreqAudioRate;
    bool isSubSampleOffsetAudioRate;
    bool isOscFreqAudioRate;
    bool isModFreqAudioRate;
    bool isOscPmIndexAudioRate;
    bool isModPmIndexAudioRate;
    bool isOscPmDampingAudioRate;
    bool isModPmDampingAudioRate;
    bool isOscCyclePosAudioRate;
    bool isModCyclePosAudioRate;
    bool isEnvCyclePosAudioRate;
 
    enum InputParams {
        Trigger,
        TriggerFreq,
        SubSampleOffset,
 
        OscFreq,
        ModFreq,

        OscPmIndex,
        ModPmIndex,
        OscPmDamping,
        ModPmDamping,
 
        OscBufNum,
        OscNumCycles,
        OscCyclePos,
 
        ModBufNum,
        ModNumCycles,
        ModCyclePos,
 
        EnvBufNum,
        EnvNumCycles,
        EnvCyclePos,
 
        Oversample
    };
 
    enum Outputs {
        Out
    };
};