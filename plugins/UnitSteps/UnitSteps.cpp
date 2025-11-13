#include "UnitSteps.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== UNIT STEP =====

UnitStep::UnitStep() {
    mCalcFunc = make_calc_function<UnitStep, &UnitStep::next>();
    next(1);
    
    // Reset state after priming
    m_state.reset();
}

void UnitStep::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    
    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        
        output[i] = m_state.process(phase, interp, rgen);
    }
}

// ===== UNIT WALK =====

UnitWalk::UnitWalk() {
    mCalcFunc = make_calc_function<UnitWalk, &UnitWalk::next>();
    next(1);
    
    // Reset state after priming
    m_state.reset();
}

void UnitWalk::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* stepIn = in(Step);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    
    // Output pointers
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float step = sc_clip(stepIn[i], 0.0f, 1.0f);
        
        output[i] = m_state.process(phase, step, interp, rgen);
    }
}

// ===== UNIT USR =====

UnitUSR::UnitUSR() {
    mCalcFunc = make_calc_function<UnitUSR, &UnitUSR::next>();
    next(1);
    
    // Reset state after priming
    m_shiftRegister.reset();
    m_resetTrigger.reset();
}

void UnitUSR::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate parameters
    const float* phaseIn = in(Phase);
    const float* chanceIn = in(Chance);
    const float* lengthIn = in(Length);
    const float* rotateIn = in(Rotate);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    bool reset = m_resetTrigger.process(in0(Reset));
    
    // Output pointers
    float* out3Bit = out(Out3Bit);
    float* out8Bit = out(Out8Bit);
    
    for (int i = 0; i < nSamples; ++i) {
        // Get audio-rate parameters per-sample
        float phase = sc_frac(phaseIn[i]);
        float chance = sc_clip(chanceIn[i], 0.0f, 1.0f);
        int length = sc_clip(static_cast<int>(lengthIn[i]), 1, MAX_LENGTH);
        int rotation = sc_clip(static_cast<int>(rotateIn[i]), -MAX_LENGTH, MAX_LENGTH);
        
        // Process shift register
        auto output = m_shiftRegister.process(
            phase, 
            chance, 
            length, 
            rotation, 
            interp,
            reset,  
            rgen
        );
        
        // Write outputs
        out3Bit[i] = output.out3Bit;
        out8Bit[i] = output.out8Bit;
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<UnitStep>(ft, "UnitStep", false);
    registerUnit<UnitWalk>(ft, "UnitWalk", false);
    registerUnit<UnitUSR>(ft, "UnitUSRUgen", false);
}