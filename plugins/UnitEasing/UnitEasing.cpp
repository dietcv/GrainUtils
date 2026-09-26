#include "UnitEasing.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== JCURVE =====

JCurve::JCurve() {
    
    // Check which inputs are audio-rate
    isShapeAudioRate = isAudioRateIn(Shape);
    
    // Set calc function & compute initial sample
    set_calc_function<JCurve, &JCurve::next>();
}

void JCurve::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_shapeInterp.update(sc_clip(in0(Shape), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            m_shapeInterp.process();
        
        output[i] = Easing::Interp::jCurve(phase, shape, Easing::Cores::quintic);
    }
}

// ===== SCURVE =====

SCurve::SCurve() {
    
    // Check which inputs are audio-rate
    isShapeAudioRate = isAudioRateIn(Shape);
    isInflectionAudioRate = isAudioRateIn(Inflection);
    
    // Set calc function & compute initial sample
    set_calc_function<SCurve, &SCurve::next>();
}

void SCurve::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_shapeInterp.update(sc_clip(in0(Shape), 0.0f, 1.0f), nSamples);
    m_inflectionInterp.update(sc_clip(in0(Inflection), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            m_shapeInterp.process();
            
        float inflection = isInflectionAudioRate ? 
            sc_clip(in(Inflection)[i], 0.0f, 1.0f) : 
            m_inflectionInterp.process();
        
        output[i] = Easing::Interp::sCurve(phase, shape, inflection, Easing::Cores::quintic);
    }
}

void UnitEasing_setup()
{
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}