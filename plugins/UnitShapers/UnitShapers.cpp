#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== UNIT TRIANGLE =====

UnitTriangle::UnitTriangle() {
    mCalcFunc = make_calc_function<UnitTriangle, &UnitTriangle::next>();
    next(1);
}

void UnitTriangle::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::triangle(phase, skew);
    }
}

// ===== UNIT KINK =====

UnitKink::UnitKink() {
    mCalcFunc = make_calc_function<UnitKink, &UnitKink::next>();
    next(1);
}

void UnitKink::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* skewIn = in(Skew);

    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float skew = sc_clip(skewIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::kink(phase, skew);
    }
}

// ===== UNIT CUBIC =====

UnitCubic::UnitCubic() {
    mCalcFunc = make_calc_function<UnitCubic, &UnitCubic::next>();
    next(1);
}

void UnitCubic::next(int nSamples) {
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* indexIn = in(Index);

    // Output pointers
    float* output = out(Out);
   
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float index = sc_clip(indexIn[i], 0.0f, 1.0f);

        output[i] = UnitShapers::cubic(phase, index);
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);
}