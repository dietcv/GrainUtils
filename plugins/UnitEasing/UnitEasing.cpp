#include "UnitEasing.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== JCURVE =====

JCurve::JCurve() {

    // Initialize parameter cache
    shapePast = sc_clip(in0(Shape), 0.0f, 1.0f);
    
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
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            slopedShape.consume();
        
        output[i] = Easing::Interp::jCurve(phase, shape, Easing::Cores::quintic);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    shapePast = isShapeAudioRate ? 
        sc_clip(in(Shape)[nSamples - 1], 0.0f, 1.0f) : 
        slopedShape.value;
}

// ===== SCURVE =====

SCurve::SCurve() {

    // Initialize parameter cache
    shapePast = sc_clip(in0(Shape), 0.0f, 1.0f);
    inflectionPast = sc_clip(in0(Inflection), 0.0f, 1.0f);
    
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
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            slopedShape.consume();
            
        float inflection = isInflectionAudioRate ? 
            sc_clip(in(Inflection)[i], 0.0f, 1.0f) : 
            slopedInflection.consume();
        
        output[i] = Easing::Interp::sCurve(phase, shape, inflection, Easing::Cores::quintic);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    shapePast = isShapeAudioRate ? 
        sc_clip(in(Shape)[nSamples - 1], 0.0f, 1.0f) : 
        slopedShape.value;
        
    inflectionPast = isInflectionAudioRate ? 
        sc_clip(in(Inflection)[nSamples - 1], 0.0f, 1.0f) : 
        slopedInflection.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}