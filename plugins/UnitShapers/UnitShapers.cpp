#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== UNIT TRIANGLE =====

UnitTriangle::UnitTriangle() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    mCalcFunc = make_calc_function<UnitTriangle, &UnitTriangle::next>();
    next(1);
}

void UnitTriangle::next(int nSamples) {
    
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
        
        output[i] = UnitShapers::triangle(phase, skew);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
}

// ===== UNIT KINK =====

UnitKink::UnitKink() {

    // Initialize parameter cache
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    mCalcFunc = make_calc_function<UnitKink, &UnitKink::next>();
    next(1);
}

void UnitKink::next(int nSamples) {
    
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
        
        output[i] = UnitShapers::kink(phase, skew);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    skewPast = isSkewAudioRate ? 
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : 
        slopedSkew.value;
}

// ===== UNIT CUBIC =====

UnitCubic::UnitCubic() {

    // Initialize parameter cache
    indexPast = sc_clip(in0(Index), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isIndexAudioRate = isAudioRateIn(Index);
    
    mCalcFunc = make_calc_function<UnitCubic, &UnitCubic::next>();
    next(1);
}

void UnitCubic::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedIndex = makeSlope(sc_clip(in0(Index), 0.0f, 1.0f), indexPast);
    
    // Output pointer
    float* output = out(Out);
   
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float index = isIndexAudioRate ? 
            sc_clip(in(Index)[i], 0.0f, 1.0f) : 
            slopedIndex.consume();
        
        output[i] = UnitShapers::cubic(phase, index);
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    indexPast = isIndexAudioRate ? 
        sc_clip(in(Index)[nSamples - 1], 0.0f, 1.0f) : 
        slopedIndex.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);
}