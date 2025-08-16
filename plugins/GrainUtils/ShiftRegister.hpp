#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"

// ===== SHARED CONSTANTS =====
static constexpr int NUM_BITS = 8;
static constexpr int MAX_LENGTH = 16;
static constexpr int MAX_DATA_VALUE = (1 << NUM_BITS) - 1;

// ===== SHIFT REGISTER =====
class ShiftRegister : public SCUnit {
public:
    ShiftRegister();
    ~ShiftRegister();

private:
    void next_aa(int inNumSamples);
    
    // Core processing
    Utils::ShiftRegister m_shiftRegister;
    Utils::EventScheduler m_scheduler;
    
    // Constants
    const float m_sampleRate;
    
    // Input parameter indices
    enum Inputs {
        Freq,
        Chance,
        Length,
        Rotate,   
        Feedback,
        FeedbackSource,
        Seed,   
        Reset
    };
   
    // Output indices  
    enum Outputs {
        Out3Bit,      
        Out8Bit,   
        OutRamp
    };
};