#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include "OscUtils.hpp"
#include "VariableOversampling.hpp"

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
    OscUtils::SincTable m_sincTable;
    OscUtils::DualOsc m_dualOsc;
    OscUtils::BufUnit m_bufUnitA;
    OscUtils::BufUnit m_bufUnitB;
    VariableOversampling<4> m_oversamplingA;
    VariableOversampling<4> m_oversamplingB;
    
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
        Oversample      // 0=1x, 1=2x, 2=4x, 3=8x, 4=16x
    };
    
    enum Outputs { 
        OutA, 
        OutB    
    };
};

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
    OscUtils::SincTable m_sincTable;
    OscUtils::BufUnit m_bufUnit;
    VariableOversampling<4> m_oversampling;
    
    enum InputParams {
        BufNum,
        Phase,
        NumCycles,
        CyclePos,
        Oversample      // 0=1x, 1=2x, 2=4x, 3=8x, 4=16x
    };
    
    enum Outputs { 
        Out
    };
};