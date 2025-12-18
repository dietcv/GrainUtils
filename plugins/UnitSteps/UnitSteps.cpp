#include "UnitSteps.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== UNIT URN =====

UnitUrn::UnitUrn() {
    
    // Check which inputs are audio-rate
    isChanceAudioRate = isAudioRateIn(Chance);
    isSizeAudioRate = isAudioRateIn(Size);
    
    mCalcFunc = make_calc_function<UnitUrn, &UnitUrn::next>();
    next(1);
    
    // Reset state after priming
    m_urn.reset();
    m_resetTrigger.reset();
}

void UnitUrn::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters
    bool reset = m_resetTrigger.process(in0(Reset));
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (no interpolation - latched per trigger)
        float chance = isChanceAudioRate ? 
            sc_clip(in(Chance)[i], 0.0f, 1.0f) : 
            sc_clip(in0(Chance), 0.0f, 1.0f);

        int size = isSizeAudioRate ? 
            sc_clip(static_cast<int>(in(Size)[i]), 2, MAX_DECK_SIZE) : 
            sc_clip(static_cast<int>(in0(Size)), 2, MAX_DECK_SIZE);    
        
        // Process urn
        output[i] = m_urn.process(
            phase,
            chance,
            size,
            reset,
            rgen
        );
    }
}

// ===== UNIT STEP =====

UnitStep::UnitStep() {
    mCalcFunc = make_calc_function<UnitStep, &UnitStep::next>();
    next(1);
    
    // Reset state after priming
    m_state.reset();
}

void UnitStep::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        output[i] = m_state.process(phase, interp, rgen);
    }
}

// ===== UNIT WALK =====

UnitWalk::UnitWalk() {
    
    // Check which inputs are audio-rate
    isStepAudioRate = isAudioRateIn(Step);
    
    mCalcFunc = make_calc_function<UnitWalk, &UnitWalk::next>();
    next(1);
    
    // Reset state after priming
    m_state.reset();
}

void UnitWalk::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (no interpolation - latched per trigger)
        float step = isStepAudioRate ? 
            sc_clip(in(Step)[i], 0.0f, 1.0f) : 
            sc_clip(in0(Step), 0.0f, 1.0f);
        
        output[i] = m_state.process(phase, step, interp, rgen);
    }
}

// ===== UNIT REGISTER =====

UnitRegister::UnitRegister() {
    
    // Check which inputs are audio-rate
    isChanceAudioRate = isAudioRateIn(Chance);
    isSizeAudioRate = isAudioRateIn(Size);
    isRotateAudioRate = isAudioRateIn(Rotate);
    
    mCalcFunc = make_calc_function<UnitRegister, &UnitRegister::next>();
    next(1);
    
    // Reset state after priming
    m_shiftRegister.reset();
    m_resetTrigger.reset();
}

void UnitRegister::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters
    bool interp = in0(Interp) > 0.5f;
    bool reset = m_resetTrigger.process(in0(Reset));
    
    // Output pointers
    float* out3Bit = out(Out3Bit);
    float* out8Bit = out(Out8Bit);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (no interpolation - latched per trigger)
        float chance = isChanceAudioRate ? 
            sc_clip(in(Chance)[i], 0.0f, 1.0f) : 
            sc_clip(in0(Chance), 0.0f, 1.0f);
        
        int size = isSizeAudioRate ? 
            sc_clip(static_cast<int>(in(Size)[i]), 1, MAX_LENGTH) : 
            sc_clip(static_cast<int>(in0(Size)), 1, MAX_LENGTH);
            
        int rotation = isRotateAudioRate ? 
            sc_clip(static_cast<int>(in(Rotate)[i]), -MAX_LENGTH, MAX_LENGTH) : 
            sc_clip(static_cast<int>(in0(Rotate)), -MAX_LENGTH, MAX_LENGTH);
        
        // Process shift register
        auto output = m_shiftRegister.process(
            phase, 
            chance, 
            size, 
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
    registerUnit<UnitUrn>(ft, "UnitUrn", false);
    registerUnit<UnitStep>(ft, "UnitStep", false);
    registerUnit<UnitWalk>(ft, "UnitWalk", false);
    registerUnit<UnitRegister>(ft, "UnitRegisterUgen", false);
}