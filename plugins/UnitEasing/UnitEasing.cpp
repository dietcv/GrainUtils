#include "UnitEasing.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== JCURVE =====

JCurve::JCurve() {
    mCalcFunc = make_calc_function<JCurve, &JCurve::next>();
    next(1);
}

void JCurve::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* shapeIn = in(Shape);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);

        output[i] = InterpFunctions::jCurve(phase, shape, EasingCores::quintic);
    }
}

// ===== SCURVE =====

SCurve::SCurve() {
    mCalcFunc = make_calc_function<SCurve, &SCurve::next>();
    next(1);
}

void SCurve::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* shapeIn = in(Shape);
    const float* inflectionIn = in(Inflection);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);
        float inflection = sc_clip(inflectionIn[i], 0.0f, 1.0f);
        
        output[i] = InterpFunctions::sCurve(phase, shape, inflection, EasingCores::quintic);
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}