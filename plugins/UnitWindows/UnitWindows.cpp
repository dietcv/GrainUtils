#include "UnitWindows.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== HANNING WINDOW =====

HanningWindow::HanningWindow() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    mCalcFunc = make_calc_function<HanningWindow, &HanningWindow::next>();
    next(1);
}

void HanningWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            slopedSkew.consume();
        
        output[i] = WindowFunctions::hanningWindow(phase, skew);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
}

// ===== GAUSSIAN WINDOW =====

GaussianWindow::GaussianWindow() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    indexPast = sc_clip(in0(Index), 0.0f, 10.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isIndexAudioRate = isAudioRateIn(Index);
    
    mCalcFunc = make_calc_function<GaussianWindow, &GaussianWindow::next>();
    next(1);
}

void GaussianWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedIndex = makeSlope(sc_clip(in0(Index), 0.0f, 10.0f), indexPast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            slopedSkew.consume();
            
        float index = isIndexAudioRate ? 
            sc_clip(in(Index)[i], 0.0f, 10.0f) : 
            slopedIndex.consume();
        
        output[i] = WindowFunctions::gaussianWindow(phase, skew, index);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
        
    indexPast = isIndexAudioRate ? 
        sc_clip(in(Index)[nSamples - 1], 0.0f, 10.0f) : 
        slopedIndex.value;
}

// ===== TRAPEZOIDAL WINDOW =====

TrapezoidalWindow::TrapezoidalWindow() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    widthPast = sc_clip(in0(Width), 0.0f, 1.0f);
    dutyPast = sc_clip(in0(Duty), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isWidthAudioRate = isAudioRateIn(Width);
    isDutyAudioRate = isAudioRateIn(Duty);
    
    mCalcFunc = make_calc_function<TrapezoidalWindow, &TrapezoidalWindow::next>();
    next(1);
}

void TrapezoidalWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedWidth = makeSlope(sc_clip(in0(Width), 0.0f, 1.0f), widthPast);
    auto slopedDuty = makeSlope(sc_clip(in0(Duty), 0.0f, 1.0f), dutyPast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            slopedSkew.consume();
            
        float width = isWidthAudioRate ? 
            sc_clip(in(Width)[i], 0.0f, 1.0f) : 
            slopedWidth.consume();
            
        float duty = isDutyAudioRate ? 
            sc_clip(in(Duty)[i], 0.0f, 1.0f) : 
            slopedDuty.consume();
        
        output[i] = WindowFunctions::trapezoidalWindow(phase, skew, width, duty);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
        
    widthPast = isWidthAudioRate ? 
        sc_clip(in(Width)[nSamples - 1], 0.0f, 1.0f) : 
        slopedWidth.value;
        
    dutyPast = isDutyAudioRate ? 
        sc_clip(in(Duty)[nSamples - 1], 0.0f, 1.0f) : 
        slopedDuty.value;
}

// ===== TUKEY WINDOW =====

TukeyWindow::TukeyWindow() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    widthPast = sc_clip(in0(Width), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isWidthAudioRate = isAudioRateIn(Width);
    
    mCalcFunc = make_calc_function<TukeyWindow, &TukeyWindow::next>();
    next(1);
}

void TukeyWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedWidth = makeSlope(sc_clip(in0(Width), 0.0f, 1.0f), widthPast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            slopedSkew.consume();
            
        float width = isWidthAudioRate ? 
            sc_clip(in(Width)[i], 0.0f, 1.0f) : 
            slopedWidth.consume();
        
        output[i] = WindowFunctions::tukeyWindow(phase, skew, width);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
        
    widthPast = isWidthAudioRate ? 
        sc_clip(in(Width)[nSamples - 1], 0.0f, 1.0f) : 
        slopedWidth.value;
}

// ===== EXPONENTIAL WINDOW =====

ExponentialWindow::ExponentialWindow() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    shapePast = sc_clip(in0(Shape), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    isShapeAudioRate = isAudioRateIn(Shape);
    
    mCalcFunc = make_calc_function<ExponentialWindow, &ExponentialWindow::next>();
    next(1);
}

void ExponentialWindow::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedShape = makeSlope(sc_clip(in0(Shape), 0.0f, 1.0f), shapePast);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float skew = isSkewAudioRate ? 
            sc_clip(in(Skew)[i], 0.0f, 1.0f) : 
            slopedSkew.consume();
            
        float shape = isShapeAudioRate ? 
            sc_clip(in(Shape)[i], 0.0f, 1.0f) : 
            slopedShape.consume();
        
        output[i] = WindowFunctions::exponentialWindow(phase, skew, shape);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
        
    shapePast = isShapeAudioRate ? 
        sc_clip(in(Shape)[nSamples - 1], 0.0f, 1.0f) : 
        slopedShape.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
}