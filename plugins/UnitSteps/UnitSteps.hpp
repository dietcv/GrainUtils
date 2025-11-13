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

// ===== UNIT USR =====

class UnitUSR : public SCUnit {
public:
    UnitUSR();
private:
    void next(int nSamples);
    
    // Constants
    static constexpr int NUM_BITS = 8;
    static constexpr int MAX_LENGTH = 16;
    
    // Core processing
    UnitSteps::UnitUSR m_shiftRegister;
    EventUtils::IsTrigger m_resetTrigger;
    
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