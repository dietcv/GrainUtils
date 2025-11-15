#include "UnitEasing.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== JCURVE =====

JCurve::JCurve() {

    // Initialize parameter cache
    shapePast = in0(Shape);
    
    // Check which inputs are audio-rate
    isShapeAudioRate = isAudioRateIn(Shape);
    
    mCalcFunc = make_calc_function<JCurve, &JCurve::next>();
    next(1);
}

void JCurve::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedShape = makeSlope(sc_clip(in0(Shape), 0.0f, 1.0f), shapePast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : slopedShape.consume();
        
        output[i] = InterpFunctions::jCurve(phase, shape, EasingCores::quintic);
    }
    
    // Update parameter cache
    shapePast = isShapeAudioRate ? in(Shape)[nSamples - 1] : slopedShape.value;
}

// ===== SCURVE =====

SCurve::SCurve() {

    // Initialize parameter cache
    shapePast = in0(Shape);
    inflectionPast = in0(Inflection);
    
    // Check which inputs are audio-rate
    isShapeAudioRate = isAudioRateIn(Shape);
    isInflectionAudioRate = isAudioRateIn(Inflection);
    
    mCalcFunc = make_calc_function<SCurve, &SCurve::next>();
    next(1);
}

void SCurve::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedShape = makeSlope(sc_clip(in0(Shape), 0.0f, 1.0f), shapePast);
    auto slopedInflection = makeSlope(sc_clip(in0(Inflection), 0.0f, 1.0f), inflectionPast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : slopedShape.consume();
            
        float inflection = isInflectionAudioRate ? 
            sc_clip(in(Inflection)[i], 0.0f, 1.0f) : slopedInflection.consume();
        
        output[i] = InterpFunctions::sCurve(phase, shape, inflection, EasingCores::quintic);
    }
    
    // Update parameter cache
    shapePast = isShapeAudioRate ? in(Shape)[nSamples - 1] : slopedShape.value;
    inflectionPast = isInflectionAudioRate ? in(Inflection)[nSamples - 1] : slopedInflection.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}