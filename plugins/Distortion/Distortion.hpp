#pragma once
#include "SC_PlugIn.hpp"
#include "DistortionUtils.hpp"
#include "OversamplingUtils.hpp"

// ===== BUCHLA 259 WAVEFOLDER =====

class BuchlaFoldADAA : public SCUnit {
public:
    BuchlaFoldADAA();
    ~BuchlaFoldADAA();
    
private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    DistortionUtils::BuchlaFoldADAA m_folder;
    OversamplingUtils::VariableOversampling<4> m_oversampling;
    
    // Cache for SlopeSignal state
    float drivePast;
    
    // Audio rate flags
    bool isDriveAudioRate;
    
    enum InputParams { 
        Input, 
        Drive,
        Oversample
    };
    
    enum Outputs { 
        Out 
    };
};