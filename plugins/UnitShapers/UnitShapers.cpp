#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== UNIT TRIANGLE =====

UnitTriangle::UnitTriangle() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    // Set calc function & compute initial sample
    set_calc_function<UnitTriangle, &UnitTriangle::next>();
}

void UnitTriangle::next(int nSamples) {
    
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
        
        output[i] = UnitShapers::triangle(phase, skew);
    }
}

// ===== UNIT KINK =====

UnitKink::UnitKink() {
    
    // Check which inputs are audio-rate
    isSkewAudioRate = isAudioRateIn(Skew);
    
    // Set calc function & compute initial sample
    set_calc_function<UnitKink, &UnitKink::next>();
}

void UnitKink::next(int nSamples) {
    
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
        
        output[i] = UnitShapers::kink(phase, skew);
    }
}

// ===== UNIT CUBIC =====

UnitCubic::UnitCubic() {
    
    // Check which inputs are audio-rate
    isIndexAudioRate = isAudioRateIn(Index);
    
    // Set calc function & compute initial sample
    set_calc_function<UnitCubic, &UnitCubic::next>();
}

void UnitCubic::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_indexInterp.update(sc_clip(in0(Index), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
   
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float index = isIndexAudioRate ? 
            sc_clip(in(Index)[i], 0.0f, 1.0f) : 
            m_indexInterp.process();
        
        output[i] = UnitShapers::cubic(phase, index);
    }
}

void UnitShapers_setup()
{
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);
}