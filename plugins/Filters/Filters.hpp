#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "FilterUtils.hpp"

// ===== DISPERSER =====

class Disperser : public SCUnit {
public:
    Disperser();
    ~Disperser();

private:
    void next(int nSamples);
   
    // Constants
    static constexpr int NUM_ALLPASSES = 8;
    
    // Constants cached at construction
    const float m_sampleRate;
   
    // Core processing
    FilterUtils::AllpassChain<NUM_ALLPASSES> m_allpassChain;
    FilterUtils::DCBlocker m_dcBlocker;
    
    // Feedback state
    float m_feedbackState{0.0f};

    // Control-rate interpolation
    Utils::ParamInterp m_freqInterp;
    Utils::ParamInterp m_resonanceInterp;
    Utils::ParamInterp m_mixInterp;
    Utils::ParamInterp m_feedbackInterp;

    // Audio rate flags
    bool isFreqAudioRate;
    bool isResonanceAudioRate;
    bool isMixAudioRate;
    bool isFeedbackAudioRate;
   
    enum InputParams {
        Input,     
        Freq,    
        Resonance, 
        Mix,        
        Feedback  
    };
   
    enum Outputs {
        Out
    };

};

// ===== MORPHING FILTER =====
 
class MorphSVF : public SCUnit {
public:
    MorphSVF();
    ~MorphSVF();
 
private:
    void next(int nSamples);
 
    // Constants cached at construction
    const float m_sampleRate;
 
    // Core processing
    FilterUtils::MorphingFilter m_morphingFilter;

    // Control-rate interpolation
    Utils::ParamInterp m_freqInterp;
    Utils::ParamInterp m_resonanceInterp;
    Utils::ParamInterp m_shapeInterp;
 
    // Audio rate flags
    bool isFreqAudioRate;
    bool isResonanceAudioRate;
    bool isShapeAudioRate;
 
    enum InputParams {
        Input,
        Freq,
        Resonance,
        Shape
    };
 
    enum Outputs {
        Out
    };
};