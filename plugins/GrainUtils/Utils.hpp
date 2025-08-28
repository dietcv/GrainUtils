#pragma once
#include "SC_PlugIn.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace Utils {

// ===== BIT MANIPULATION UTILITIES =====

inline float randomFloat(RGen& rgen) {
    return rgen.frand();
}

inline int rotateBits(int value, int rotation, int length) {
    // Use wrap instead of % to handle negative rotation amount
    int normalizedRotation = sc_wrap(rotation, 0, length - 1);
    int complementRotation = length - normalizedRotation;
        
    // Calculate bit ranges for the given length
    int maxValueForLength = static_cast<int>(std::pow(2, length));
    double leftShiftMultiplier = std::pow(2, normalizedRotation);
    double rightShiftDivisor = std::pow(2, -complementRotation);
        
    // Perform the bit shifts
    double leftShifted = value * leftShiftMultiplier;
    double rightShifted = value * rightShiftDivisor;
        
    // Extract the relevant parts
    int leftPart = static_cast<int>(leftShifted) % maxValueForLength;
    int rightPart = static_cast<int>(std::floor(rightShifted));
        
    // Combine both parts to get the rotated result
    return leftPart + rightPart;
}

// Extract top numBits and apply LSB weighting (bit5*1 + bit6*2 + bit7*4)
inline float getMSBBits(int value, int numBits, int totalBits) {
    int startBit = totalBits - numBits;  // Calculate start bit for MSB
    int result = 0;
        
    for (int i = 0; i < numBits; i++) {
        int bitIndex = startBit + i;
            
        // Extract the bit using power/modulus
        int divisor = static_cast<int>(std::pow(2, bitIndex));
        int bit = (value / divisor) % 2;
            
        // Apply LSB-first weighting
        int weight = static_cast<int>(std::pow(2, i));
        result += bit * weight;
    }
        
    // Normalize to 0-1 range
    int maxValue = static_cast<int>(std::pow(2, numBits)) - 1;
    return static_cast<float>(result) / static_cast<float>(maxValue);
}

// Extract bottom numBits and apply MSB weighting (bit0*128 + bit1*64 + ... + bit7*1)
inline float getLSBBits(int value, int numBits, int totalBits) {
    int result = 0;
        
    for (int i = 0; i < numBits; i++) {
        int bitIndex = i;  // Start from bit 0
            
        // Extract the bit using power/modulus
        int divisor = static_cast<int>(std::pow(2, bitIndex));
        int bit = (value / divisor) % 2;
            
        // Apply MSB-first weighting
        int weight = static_cast<int>(std::pow(2, numBits - 1 - i));
        result += bit * weight;
    }
        
    // Normalize to 0-1 range
    int maxValue = static_cast<int>(std::pow(2, numBits)) - 1;
    return static_cast<float>(result) / static_cast<float>(maxValue);
}

// ===== SHIFT REGISTER =====

struct ShiftRegister {
    int m_register{0};
    float m_out3Bit{0.0f};          // unipolar output [0, 1]
    float m_out8Bit{0.0f};          // unipolar output [0, 1]
    bool m_initialized{false};      // No output until first trigger
    
    void process(bool trigger, float chance, int length, int rotation, RGen& rgen) {

        if (trigger) {
            if (!m_initialized) {
                // output the initial register state for the first ramp cycle
                m_register = 0;
                m_out3Bit = getMSBBits(m_register, 3, 8);
                m_out8Bit = 1.0f - getLSBBits(m_register, 8, 8);
                m_initialized = true;
            } else {
                // Rotate shift register
                int rotated = rotateBits(m_register, rotation, length);

                // Extract LSB for feedback
                int extractedBit = rotated % 2; // Get LSB
                int withoutLSB = rotated - extractedBit; // Remove LSB

                // XOR with random value
                bool feedbackBit = (randomFloat(rgen) < chance);
                int newBit = extractedBit ^ static_cast<int>(feedbackBit);

                // Update Shift Register
                m_register = withoutLSB + newBit;

                // Calculate normalized Shift Register output
                m_out3Bit = getMSBBits(m_register, 3, 8);
                m_out8Bit = 1.0f - getLSBBits(m_register, 8, 8);
            }
        }
    }

    void reset() {
        m_register = 0;
        m_out3Bit = 0.0f;
        m_out8Bit = 0.0f;
        m_initialized = false;
    }
};

// ===== TRIGGER AND TIMING UTILITIES =====

struct RampToSlope {
    double m_lastPhase{0.0};
   
    double process(double currentPhase) {
        // Calculate ramp slope
        double delta = currentPhase - m_lastPhase;

        // Wrap delta to recenter between -0.5 and 0.5, for corrected slope during wrap
        if (delta > 0.5)
            delta -= 1.0;
        else if (delta < -0.5)
            delta += 1.0;    
    
        // Update state for next sample
        m_lastPhase = currentPhase;

        return delta;
    }
   
    void reset() {
        m_lastPhase = 0.0;
    }
};

struct RampToTrig {
    double m_lastPhase{0.0};
    bool m_lastWrap{false};
    
    bool process(double currentPhase) {
        // Detect wrap using current vs last phase
        double delta = currentPhase - m_lastPhase;
        double sum = currentPhase + m_lastPhase;
        bool currentWrap = (sum != 0.0) && (std::abs(delta / sum) > 0.5);
        
        // Edge detection - only trigger on rising edge of wrap
        bool trigger = currentWrap && !m_lastWrap;
        
        // Update state for next sample
        m_lastPhase = currentPhase;
        m_lastWrap = currentWrap;
        
        return trigger;
    }
    
    void reset() {
        m_lastPhase = 0.0;
        m_lastWrap = false;
    }
};

// ===== SCHEDULER CYCLE =====

struct SchedulerCycle {
    // Core timing components
    RampToTrig trigDetect;
   
    // Ramp state
    double m_phase{0.0};        // Current ramp position [0,1)
    double m_slope{0.0};        // Current slope (rate/sampleRate)
    bool m_wrapNext{false};     // Flag: will wrap on next sample
   
    struct Output {
        bool trigger = false;
        float phase = 0.0f;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
   
    Output process(float triggerRate, bool resetTrigger, float sampleRate) {
        Output output;
       
        // Handle reset
        if (resetTrigger) {
            reset();
            return output; // Early exit on reset
        }
     
        // Initialize slope
        if (m_slope <= 0.0) {
            m_slope = triggerRate / sampleRate;
        }
       
        // 1. Handle wrap from previous sample
        if (m_wrapNext) {
            m_phase -= 1.0;                         // Wrap the phase
            m_slope = triggerRate / sampleRate;     // Latch new slope for next period
            m_wrapNext = false;
        }
       
        // 2. Detect trigger
        bool trigger = trigDetect.process(m_phase);
       
        // 3. Calculate subsample offset when trigger occurs
        double subSampleOffset = 0.0;
        if (trigger && m_slope != 0.0) {
            subSampleOffset = m_phase / m_slope;
        }
       
        // 4. Prepare output
        output.trigger = trigger;
        output.phase = static_cast<float>(m_phase);
        output.rate = static_cast<float>(m_slope * sampleRate);
        output.subSampleOffset = static_cast<float>(subSampleOffset);
       
        // 5. Increment phase
        m_phase += m_slope;        
       
        // 6. Check for wrap
        if (m_phase >= 1.0) {
            m_wrapNext = true;
        }
       
        return output;
    }
   
    void reset() {
        m_phase = 0.0;
        m_slope = 0.0;
        m_wrapNext = false;
        trigDetect.reset();
    }

    void init() {
        m_phase = 0.0;
        m_slope = 0.0;
        m_wrapNext = false;
        // Prime trigger detector to fire on next sample
        trigDetect.m_lastPhase = 1.0; // As if coming from wrap
        trigDetect.m_lastWrap = false; // Ready to detect next wrap
    }
};

// ===== VOICE ALLOCATOR =====

struct VoiceAllocator {
    std::vector<double> channelPhases;
    std::vector<double> channelSlopes;
    std::vector<bool> isActive;
    std::vector<bool> justTriggered;
    std::vector<float> outputPhases;
    std::vector<bool> outputTriggers;
    int numChannels;
    
    explicit VoiceAllocator(int channels = 16) : numChannels(channels) {
        channelPhases.resize(numChannels, 0.0f);
        channelSlopes.resize(numChannels, 0.0f);
        isActive.resize(numChannels, false);
        justTriggered.resize(numChannels, false);
        outputPhases.resize(numChannels, 0.0f);
        outputTriggers.resize(numChannels, false);
    }
    
    void process(bool trigger, float rate, float subSampleOffset, float sampleRate) {

        // Clear triggers
        std::fill(justTriggered.begin(), justTriggered.end(), false);
        
        // Handle trigger - find first available channel
        if (trigger) {
            for (int ch = 0; ch < numChannels; ++ch) {
                if (!isActive[ch]) {
                    // Found available channel - trigger voice
                    justTriggered[ch] = true;
                    channelSlopes[ch] = rate / sampleRate;
                    channelPhases[ch] = channelSlopes[ch] * subSampleOffset;
                    isActive[ch] = true;
                    break;
                }
            }
        }
        
        // Process all channels
        for (int ch = 0; ch < numChannels; ++ch) {
            if (!isActive[ch]) {
                outputPhases[ch] = 0.0f;
                outputTriggers[ch] = false;
                continue;
            }
            
            // Don't increment on trigger sample
            if (!justTriggered[ch]) {
                channelPhases[ch] += channelSlopes[ch];
            }
            
            // Check if phase completed
            if (channelPhases[ch] >= 1.0f) {
                isActive[ch] = false;
                outputPhases[ch] = 0.0f;
                outputTriggers[ch] = false;
            } else {
                outputPhases[ch] = static_cast<float>(channelPhases[ch]);
                outputTriggers[ch] = justTriggered[ch];
            }
        }
    }
    
    void reset() {
        std::fill(channelPhases.begin(), channelPhases.end(), 0.0f);
        std::fill(channelSlopes.begin(), channelSlopes.end(), 0.0f);
        std::fill(isActive.begin(), isActive.end(), false);
        std::fill(justTriggered.begin(), justTriggered.end(), false);
        std::fill(outputPhases.begin(), outputPhases.end(), 0.0f);
        std::fill(outputTriggers.begin(), outputTriggers.end(), false);
    }
};

// ===== RAMP INTEGRATOR =====

struct RampIntegrator {
    double m_phase{0.0};
    bool m_hasTriggered{false};
    
    float process(bool trigger, float rate, float subSampleOffset, float sampleRate) {

        // 1. Calculate slope from rate
        double slope = rate / sampleRate;
        
        // 2. Handle trigger - reset phase with subsample offset
        if (trigger) {
            m_phase = slope * subSampleOffset;
            m_hasTriggered = true;
        }
        
        // 3. Output current phase
        float output = 0.0f;
        if (m_hasTriggered) {
            output = sc_wrap(static_cast<float>(m_phase), 0.0f, 1.0f);
        }
    
        // 4. Increment phase
        m_phase += slope;
    
        return output;
    }
    
    void reset() {
        m_phase = 0.0;
        m_hasTriggered = false;
    }
};

} // namespace Utils