#include "UnitSteps.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== UNIT STEP =====

UnitStep::UnitStep() :
    m_interp(in0(Interp) > 0.5f)
{
    // Set calc function & compute initial sample
    set_calc_function<UnitStep, &UnitStep::next>();
    
    // Reset state after priming
    m_state.reset();
}

void UnitStep::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        output[i] = m_state.process(phase, m_interp, rgen);
    }
}

// ===== UNIT WALK =====

UnitWalk::UnitWalk() :
    m_interp(in0(Interp) > 0.5f)
{
    // Check which inputs are audio-rate
    isStepAudioRate = isAudioRateIn(Step);
    
    // Set calc function & compute initial sample
    set_calc_function<UnitWalk, &UnitWalk::next>();
    
    // Reset state after priming
    m_state.reset();
}

void UnitWalk::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Output pointer
    float* output = out(Out);
    
    for (int i = 0; i < nSamples; ++i) {
        
        // Wrap phase between 0 and 1
        float phase = sc_frac(phaseIn[i]);
        
        // Get current parameter values (no interpolation - latched per trigger)
        float step = isStepAudioRate ? 
            sc_clip(in(Step)[i], 0.0f, 1.0f) : 
            sc_clip(in0(Step), 0.0f, 1.0f);
        
        output[i] = m_state.process(phase, step, m_interp, rgen);
    }
}

// ===== UNIT REGISTER =====

UnitRegister::UnitRegister() :
    m_interp(in0(Interp) > 0.5f)
{
    // Check which inputs are audio-rate
    isChanceAudioRate = isAudioRateIn(Chance);
    isSizeAudioRate = isAudioRateIn(Size);
    isRotateAudioRate = isAudioRateIn(Rotate);
    isResetAudioRate = isAudioRateIn(Reset);
    
    // Set calc function & compute initial sample
    set_calc_function<UnitRegister, &UnitRegister::next>();
    
    // Reset state after priming
    m_register.reset();
    m_resetTrigger.reset();
}

void UnitRegister::next(int nSamples) {
    RGen& rgen = *mParent->mRGen;
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
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
        
        // Trigger input (audio-rate or control-rate)
        bool reset = isResetAudioRate ? 
            m_resetTrigger.process(in(Reset)[i]) : 
            m_resetTrigger.process(in0(Reset));
        
        // Process shift register
        auto output = m_register.process(
            phase, 
            chance, 
            size, 
            rotation, 
            m_interp,
            reset,  
            rgen
        );
        
        // Write outputs
        out3Bit[i] = output.out3Bit;
        out8Bit[i] = output.out8Bit;
    }
}

void UnitSteps_setup() 
{
    registerUnit<UnitStep>(ft, "UnitStep", false);
    registerUnit<UnitWalk>(ft, "UnitWalk", false);
    registerUnit<UnitRegister>(ft, "UnitRegisterUgen", false);
}