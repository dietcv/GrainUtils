#pragma once
#include "SC_PlugIn.hpp"
#include "StepUtils.hpp"

// ===== UNIT URN =====
class UnitUrn : public SCUnit {
public:
    UnitUrn();
    
private:
    void next(int nSamples);
    
    // Constants
    static constexpr int MAX_DECK_SIZE = 32;
    
    // Core processing
    UnitSteps::UnitUrn<MAX_DECK_SIZE> m_urn;
    EventUtils::IsTrigger m_resetTrigger;
    
    // Audio rate flags
    bool isChanceAudioRate;
    bool isSizeAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Chance,
        Size,
        Reset
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

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
    
    // Audio rate flags
    bool isChanceAudioRate;
    bool isSizeAudioRate;
    bool isRotateAudioRate;
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Chance,
        Size,
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