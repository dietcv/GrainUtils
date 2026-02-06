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