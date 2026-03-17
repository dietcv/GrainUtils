#pragma once
#include "SC_PlugIn.hpp"
#include "FilterUtils.hpp"

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
    FilterUtils::AllpassCascade<NUM_ALLPASSES> disperser;
    FilterUtils::OnePoleHz m_dcBlocker;
    
    // Feedback state
    float m_feedbackState{0.0f};
    
    // Cache for SlopeSignal state
    float freqPast, resonancePast, mixPast, feedbackPast;

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