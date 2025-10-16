#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== UNIT SHAPERS =====

UnitTriangle::UnitTriangle() {
    mCalcFunc = make_calc_function<UnitTriangle, &UnitTriangle::next>();
    next(1);
}

void UnitTriangle::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::triangle(phase, skew);
    }
}

UnitKink::UnitKink() {
    mCalcFunc = make_calc_function<UnitKink, &UnitKink::next>();
    next(1);
}

void UnitKink::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::kink(phase, skew);
    }
}

UnitCubic::UnitCubic() {
    mCalcFunc = make_calc_function<UnitCubic, &UnitCubic::next>();
    next(1);
}

void UnitCubic::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* indexIn = in(1);

    // Output pointers
    float* output = out(0);
   
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float index = sc_clip(indexIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::cubic(phase, index);
    }
}

UnitRand::UnitRand() {
    mCalcFunc = make_calc_function<UnitRand, &UnitRand::next>();
    next(1);

    // Reset state after priming
    m_state.reset();
}

void UnitRand::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate parameters
    const float* phaseIn = in(0);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);

        output[i] = m_state.process(phase, rgen);
    }
}

UnitWalk::UnitWalk() {
    mCalcFunc = make_calc_function<UnitWalk, &UnitWalk::next>();
    next(1);

    // Reset state after priming
    m_state.reset();
}

void UnitWalk::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* stepIn = in(1);
    
    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float step = sc_clip(stepIn[i], 0.0f, 1.0f);

        output[i] = m_state.process(phase, step, rgen);
    }
}

// ===== WINDOW FUNCTIONS =====

HanningWindow::HanningWindow() {
    mCalcFunc = make_calc_function<HanningWindow, &HanningWindow::next>();
    next(1);
}

void HanningWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::hanningWindow(phase, skew);
    }
}

GaussianWindow::GaussianWindow() {
    mCalcFunc = make_calc_function<GaussianWindow, &GaussianWindow::next>();
    next(1);
}

void GaussianWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);
    const float* indexIn = in(2);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float index = indexIn[i];

        output[i] = WindowFunctions::gaussianWindow(phase, skew, index);
    }
}

TrapezoidalWindow::TrapezoidalWindow() {
    mCalcFunc = make_calc_function<TrapezoidalWindow, &TrapezoidalWindow::next>();
    next(1);
}

void TrapezoidalWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);
    const float* widthIn = in(2);
    const float* dutyIn = in(3);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float width = sc_clip(widthIn[i], 0.0f, 1.0f);
        float duty = sc_clip(dutyIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::trapezoidalWindow(phase, skew, width, duty);
    }
}

TukeyWindow::TukeyWindow() {
    mCalcFunc = make_calc_function<TukeyWindow, &TukeyWindow::next>();
    next(1);
}

void TukeyWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);
    const float* widthIn = in(2);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float width = sc_clip(widthIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::tukeyWindow(phase, skew, width);
    }
}

ExponentialWindow::ExponentialWindow() {
    mCalcFunc = make_calc_function<ExponentialWindow, &ExponentialWindow::next>();
    next(1);
}

void ExponentialWindow::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* skewIn = in(1);
    const float* shapeIn = in(2);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_wrap(phaseIn[i], 0.0f, 1.0f);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);

        output[i] = WindowFunctions::exponentialWindow(phase, skew, shape);
    }
}

// ===== INTERP FUNCTIONS =====

JCurve::JCurve() {
    mCalcFunc = make_calc_function<JCurve, &JCurve::next>();
    next(1);
}

void JCurve::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* shapeIn = in(1);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_clip(phaseIn[i], 0.0f, 1.0f);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);

        output[i] = InterpFunctions::jCurve(phase, shape, EasingCores::quintic);
    }
}

SCurve::SCurve() {
    mCalcFunc = make_calc_function<SCurve, &SCurve::next>();
    next(1);
}

void SCurve::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(0);
    const float* shapeIn = in(1);
    const float* inflectionIn = in(2);

    // Output pointers
    float* output = out(0);
    
    for (int i = 0; i < nSamples; ++i) {

        // Get audio-rate parameters per-sample
        float phase = sc_clip(phaseIn[i], 0.0f, 1.0f);
        float shape = sc_clip(shapeIn[i], 0.0f, 1.0f);
        float inflection = sc_clip(inflectionIn[i], 0.0f, 1.0f);
        
        output[i] = InterpFunctions::sCurve(phase, shape, inflection, EasingCores::quintic);
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    // Unit Shapers
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);
    registerUnit<UnitRand>(ft, "UnitRand", false);
    registerUnit<UnitWalk>(ft, "UnitWalk", false);

    // Window Functions
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
    
    // Easing Functions
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}