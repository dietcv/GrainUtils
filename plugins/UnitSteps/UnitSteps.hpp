#pragma once
#include "SC_PlugIn.hpp"
#include "ShaperUtils.hpp"

// ===== UNIT STEP =====

class UnitStep : public SCUnit {
public:
    UnitStep();

private:
    void next(int nSamples);
    
    // Core processing
    UnitSteps::UnitStep m_state;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Interp
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== UNIT WALK =====

class UnitWalk : public SCUnit {
public:
    UnitWalk();

private:
    void next(int nSamples);
    
    // Core processing
    UnitSteps::UnitWalk m_state;
    
    // Cache for SlopeSignal state
    float stepPast;
    
    // Audio rate flags
    bool isStepAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Step,
        Interp
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== UNIT REGISTER =====

class UnitRegister : public SCUnit {
public:
    UnitRegister();

private:
    void next(int nSamples);
    
    // Constants
    static constexpr int NUM_BITS = 8;
    static constexpr int MAX_LENGTH = 16;
    
    // Core processing
    UnitSteps::UnitRegister m_shiftRegister;
    EventUtils::IsTrigger m_resetTrigger;
    
    // Cache for SlopeSignal state
    float chancePast;
    
    // Audio rate flags
    bool isChanceAudioRate;
    bool isLengthAudioRate;
    bool isRotateAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Chance,
        Length,
        Rotate,
        Interp,
        Reset
    };
    
    // Output indices
    enum Outputs {
        Out3Bit,
        Out8Bit
    };
};