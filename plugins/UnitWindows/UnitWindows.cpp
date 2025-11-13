#include "UnitWindows.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== HANNING WINDOW =====

HanningWindow::HanningWindow() {
    mCalcFunc = make_calc_function<HanningWindow, &HanningWindow::next>();
    next(1);
}

void HanningWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::hanningWindow(phase, skew);
    }
}

// ===== GAUSSIAN WINDOW =====

GaussianWindow::GaussianWindow() {
    mCalcFunc = make_calc_function<GaussianWindow, &GaussianWindow::next>();
    next(1);
}

void GaussianWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);
    const float* indexIn = in(Index);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float index = indexIn[i];

        output[i] = WindowFunctions::gaussianWindow(phase, skew, index);
    }
}

// ===== TRAPEZOIDAL WINDOW =====

TrapezoidalWindow::TrapezoidalWindow() {
    mCalcFunc = make_calc_function<TrapezoidalWindow, &TrapezoidalWindow::next>();
    next(1);
}

void TrapezoidalWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);
    const float* widthIn = in(Width);
    const float* dutyIn = in(Duty);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float width = sc_clip(widthIn[i], 0.0f, 1.0f);
        float duty = sc_clip(dutyIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::trapezoidalWindow(phase, skew, width, duty);
    }
}

// ===== TUKEY WINDOW =====

TukeyWindow::TukeyWindow() {
    mCalcFunc = make_calc_function<TukeyWindow, &TukeyWindow::next>();
    next(1);
}

void TukeyWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);
    const float* widthIn = in(Width);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float width = sc_clip(widthIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::tukeyWindow(phase, skew, width);
    }
}

// ===== EXPONENTIAL WINDOW =====

ExponentialWindow::ExponentialWindow() {
    mCalcFunc = make_calc_function<ExponentialWindow, &ExponentialWindow::next>();
    next(1);
}

void ExponentialWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);
    const float* shapeIn = in(Shape);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::exponentialWindow(phase, skew, shape);
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
}