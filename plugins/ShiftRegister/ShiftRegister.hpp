#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"

// ===== SHARED CONSTANTS =====
static constexpr int NUM_BITS = 8;
static constexpr int MAX_LENGTH = 16;

// ===== SHIFT REGISTER =====
class ShiftRegister : public SCUnit {
public:
    ShiftRegister();
    ~ShiftRegister();

private:
    void next_aa(int inNumSamples);
    
    // Core processing
    Utils::ShiftRegister m_shiftRegister;
    
    // Constants
    const float m_sampleRate;
    
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