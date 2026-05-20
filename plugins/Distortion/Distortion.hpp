#pragma once
#include "SC_PlugIn.hpp"
#include "DistortionUtils.hpp"
#include "OversamplingUtils.hpp"

// ===== BUCHLA 259 WAVEFOLDER =====

class BuchlaFold : public SCUnit {
public:
    BuchlaFold();
    ~BuchlaFold();
    
private:
    void next(int nSamples);
    
    // Constants
    const float m_sampleRate;
    
    // Core processing
    DistortionUtils::BuchlaFold m_folder;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling<4> m_outputOversampling;
    OversamplingUtils::VariableOversampling<4> m_driveOversampling;
    
    // Stored oversampling state
    int m_oversampleIndex;
    int m_osRatio;
    float* m_outputOSBuffer;
    float* m_osDriveBuffer;
    
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