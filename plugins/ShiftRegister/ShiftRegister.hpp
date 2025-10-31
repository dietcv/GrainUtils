#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"

class ShiftRegister : public SCUnit {
public:
    ShiftRegister();
    ~ShiftRegister();

private:
    void next_aa(int inNumSamples);

    // Constants
    static constexpr int NUM_BITS = 8;
    static constexpr int MAX_LENGTH = 16;
    
    // Constants cached at construction
    const float m_sampleRate;

    // Core processing
    Utils::ShiftRegister m_shiftRegister;
    Utils::IsTrigger m_trigger;
    Utils::IsTrigger m_resetTrigger;
    
    // Input parameter indices
    enum Inputs {
        Trigger,
        Chance,
        Length,
        Rotate,
        Reset
    };
   
    // Output indices  
    enum Outputs {
        Out3Bit,      
        Out8Bit,
    };
};