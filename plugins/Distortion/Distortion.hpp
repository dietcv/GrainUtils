#pragma once
#include "SC_PlugIn.hpp"
#include "DistortionUtils.hpp"
#include "OversamplingUtils.hpp"
#include "PluginUtils.hpp"

// ===== BUCHLA 259 WAVEFOLDER =====

class BuchlaFold : public SCUnit {
public:
    BuchlaFold();
    ~BuchlaFold();
    
private:
    void next(int nSamples);
    
    // Constants cached at construction
    const float m_sampleRate;
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    DistortionUtils::BuchlaFold m_folder;
    
    // Oversampling objects
    OversamplingUtils::VariableOversampling m_outputOversampling;
    OversamplingUtils::VariableOversampling m_driveOversampling;
    
    // Stored oversampling state
    float* m_outputOSBuffer{nullptr};
    float* m_driveOSBuffer{nullptr};
    
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