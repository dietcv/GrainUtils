#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "DistortionUtils.hpp"
#include "OversamplingUtils.hpp"
#include "BufferUtils.hpp"

// ===== BUCHLA 259 WAVEFOLDER =====

class BuchlaFold : public SCUnit {
public:
    BuchlaFold();
    ~BuchlaFold();
    
private:
    void next(int nSamples);
    
    // Constants cached at construction
    const int m_oversampleIndex;
    const int m_osRatio;
    
    // Core processing
    DistortionUtils::BuchlaFold m_folder;

    // Control-rate interpolation
    Utils::ParamInterp m_driveInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osDriveInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
    
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

// ===== SERGE WAVEFOLDER =====
 
class SergeFold : public SCUnit {
public:
    SergeFold();
    ~SergeFold();
 
private:
    void next(int nSamples);
 
    // Constants cached at construction
    const int m_oversampleIndex;
    const int m_osRatio;
 
    // Core processing
    DistortionUtils::SergeFold m_folder;

    // Control-rate interpolation
    Utils::ParamInterp m_driveInterp;

    // Oversampling interpolation
    OversamplingUtils::OSParamInterp m_osDriveInterp;
    
    // Variable-Oversampling
    OversamplingUtils::VariableOversampling m_outputOversampling;
    float* m_outputOSBuffer{nullptr};
 
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