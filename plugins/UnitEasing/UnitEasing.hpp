#pragma once
#include "SC_PlugIn.hpp"
#include "ShaperUtils.hpp"

// ===== JCURVE =====

class JCurve : public SCUnit {
public:
    JCurve();

private:
    void next(int nSamples);
    
    // Cache for SlopeSignal state
    float shapePast;
    
    // Audio rate flags
    bool isShapeAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Shape
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== SCURVE =====

class SCurve : public SCUnit {
public:
    SCurve();

private:
    void next(int nSamples);
    
    // Cache for SlopeSignal state
    float shapePast, inflectionPast;
    
    // Audio rate flags
    bool isShapeAudioRate;
    bool isInflectionAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Shape,
        Inflection
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};