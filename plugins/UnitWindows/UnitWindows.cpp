#include "UnitWindows.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== HANNING WINDOW =====

HanningWindow::HanningWindow() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    // Set calc function & compute initial sample
    set_calc_function<HanningWindow, &HanningWindow::next>();
}

void HanningWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_skewInterp.update(sc_clip(in0(Skew), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            m_skewInterp.process();
        
        output[i] = WindowFunctions::hanningWindow(phase, skew);
    }
}

// ===== GAUSSIAN WINDOW =====

GaussianWindow::GaussianWindow() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isIndexAudioRate = isAudioRateIn(Index);
    
    // Set calc function & compute initial sample
    set_calc_function<GaussianWindow, &GaussianWindow::next>();
}

void GaussianWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_skewInterp.update(sc_clip(in0(Skew), 0.0f, 1.0f), nSamples);
    m_indexInterp.update(sc_clip(in0(Index), 0.0f, 10.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            m_skewInterp.process();
            
        float index = isIndexAudioRate ? 
            sc_clip(in(Index)[i], 0.0f, 10.0f) : 
            m_indexInterp.process();
        
        output[i] = WindowFunctions::gaussianWindow(phase, skew, index);
    }
}

// ===== TRAPEZOIDAL WINDOW =====

TrapezoidalWindow::TrapezoidalWindow() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isWidthAudioRate = isAudioRateIn(Width);
    isDutyAudioRate = isAudioRateIn(Duty);
    
    // Set calc function & compute initial sample
    set_calc_function<TrapezoidalWindow, &TrapezoidalWindow::next>();
}

void TrapezoidalWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_skewInterp.update(sc_clip(in0(Skew), 0.0f, 1.0f), nSamples);
    m_widthInterp.update(sc_clip(in0(Width), 0.0f, 1.0f), nSamples);
    m_dutyInterp.update(sc_clip(in0(Duty), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            m_skewInterp.process();
            
        float width = isWidthAudioRate ? 
            sc_clip(in(Width)[i], 0.0f, 1.0f) : 
            m_widthInterp.process();
            
        float duty = isDutyAudioRate ? 
            sc_clip(in(Duty)[i], 0.0f, 1.0f) : 
            m_dutyInterp.process();
        
        output[i] = WindowFunctions::trapezoidalWindow(phase, skew, width, duty);
    }
}

// ===== TUKEY WINDOW =====

TukeyWindow::TukeyWindow() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isWidthAudioRate = isAudioRateIn(Width);
    
    // Set calc function & compute initial sample
    set_calc_function<TukeyWindow, &TukeyWindow::next>();
}

void TukeyWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_skewInterp.update(sc_clip(in0(Skew), 0.0f, 1.0f), nSamples);
    m_widthInterp.update(sc_clip(in0(Width), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            m_skewInterp.process();
            
        float width = isWidthAudioRate ? 
            sc_clip(in(Width)[i], 0.0f, 1.0f) : 
            m_widthInterp.process();
        
        output[i] = WindowFunctions::tukeyWindow(phase, skew, width);
    }
}

// ===== EXPONENTIAL WINDOW =====

ExponentialWindow::ExponentialWindow() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isShapeAudioRate = isAudioRateIn(Shape);
    
    // Set calc function & compute initial sample
    set_calc_function<ExponentialWindow, &ExponentialWindow::next>();
}

void ExponentialWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_skewInterp.update(sc_clip(in0(Skew), 0.0f, 1.0f), nSamples);
    m_shapeInterp.update(sc_clip(in0(Shape), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            m_skewInterp.process();
            
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            m_shapeInterp.process();
        
        output[i] = WindowFunctions::exponentialWindow(phase, skew, shape);
    }
}

void UnitWindows_setup()
{
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
}