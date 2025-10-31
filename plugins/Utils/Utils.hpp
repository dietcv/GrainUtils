#pragma once
#include "SC_PlugIn.hpp"
#include <array>
#include <cmath>  
#include <algorithm>

namespace Utils {

// ===== CONSTANTS =====

inline constexpr float PI = 3.14159265358979323846f;
inline constexpr float TWO_PI = 6.28318530717958647692f;
inline constexpr float TWO_PI_INV = 1.0f / 6.28318530717958647692f;

// ===== BASIC MATH UTILITIES =====

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float cosineInterp(float a, float b, float t) {
    float mu2 = (1.0f - std::cos(t * PI)) / 2.0f;
    return a * (1.0f - mu2) + b * mu2;
}

// ===== HIGH-PERFORMANCE BUFFER ACCESS UTILITIES =====

// Fast wrapping within a cycle range (startPos to endPos-1) - (for power-of-2 sizes)
inline int wrapIndex(int index, int startPos, int mask) {
    return startPos + (index & mask);
}

// Fast no-interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekNoInterp(const float* buffer, int index, int mask) {
    const int wrappedIndex = index & mask;
    return buffer[wrappedIndex];
}

// Fast linear interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekLinearInterp(const float* buffer, float phase, int mask) {

    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    
    const float a = buffer[idx1];
    const float b = buffer[idx2];
    
    return lerp(a, b, fracPart);
}

// Fast cubic interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekCubicInterp(const float* buffer, float phase, int mask) {
    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx0 = (intPart - 1) & mask;
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    const int idx3 = (intPart + 2) & mask;
    
    const float a = buffer[idx0];
    const float b = buffer[idx1];
    const float c = buffer[idx2];
    const float d = buffer[idx3];
    
    return cubicinterp(fracPart, a, b, c, d);
}

// ===== ONE POLE FILTER UTILITIES =====

struct OnePole {
    float m_state{0.0f};
    
    float processLowpass(float input, float coeff) {

        // Clip coefficient
        coeff = sc_clip(coeff, 0.0f, 1.0f);

        // OnePole formula: y[n] = x[n] * (1-b) + y[n-1] * b
        m_state = input * (1.0f - coeff) + m_state * coeff;

        return m_state;
    }

    float processHighpass(float input, float coeff) {
        float lowpassed = processLowpass(input, coeff);
        return input - lowpassed;
    }

    void reset() {
        m_state = 0.0f;
    }
};

struct OnePoleHz {
    float m_state{0.0f};
   
    float processLowpass(float input, float cutoffHz, float sampleRate) {

        // Clip slope to Nyquist range and take absolute value
        float slope = cutoffHz / sampleRate;
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
        
        // Calculate coefficient: b = exp(-2π * slope)
        float coeff = std::exp(-TWO_PI * safeSlope);
        
        // OnePole formula: y[n] = x[n] * (1-b) + y[n-1] * b
        m_state = input * (1.0f - coeff) + m_state * coeff;
        
        return m_state;
    }
   
    float processHighpass(float input, float cutoffHz, float sampleRate) {
        float lowpassed = processLowpass(input, cutoffHz, sampleRate);
        return input - lowpassed;
    }

    void reset() {
        m_state = 0.0f;
    }
};

struct OnePoleSlope {
    float m_state{0.0f};
      
    float processLowpass(float input, float slope) {

        // Clip slope to Nyquist range and take absolute value
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
       
        // Calculate coefficient: b = exp(-2π * slope)
        float coeff = std::exp(-2.0f * Utils::PI * safeSlope);
       
        // OnePole formula: y[n] = x[n] * (1-b) + y[n-1] * b
        m_state = input * (1.0f - coeff) + m_state * coeff;
       
        return m_state;
    }
   
    float processHighpass(float input, float slope) {
        float lowpassed = processLowpass(input, slope);
        return input - lowpassed;
    }

    void reset() {
        m_state = 0.0f;
    }
};

// ===== BIT MANIPULATION UTILITIES =====

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

// ===== TRIGGER AND TIMING UTILITIES =====

struct IsTrigger {
    float m_prevIn{0.0f};
    
    bool process(float currentIn) {
        // SC canonical trigger: rising edge detection (prevIn <= 0 && currentIn > 0)
        bool trigger = (currentIn > 0.0f && m_prevIn <= 0.0f);
        
        // Update state for next sample
        m_prevIn = currentIn;
        
        return trigger;
    }
    
    void reset() {
        m_prevIn = 0.0f;
    }
};

struct RampToTrig {
    double m_lastPhase{0.0};
    bool m_lastWrap{false};
    
    bool process(double currentPhase) {
        // Detect wrap with proportional change
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
        m_lastPhase = 1.0;  // Prime to allow initial trigger
        m_lastWrap = false;
    }
};

struct StepToTrig {
    double m_lastCeiling{0.0};
    bool m_lastStep{false};     
    
    bool process(double phaseScaled) {
        // Calculate current ceiling
        double currentCeiling = sc_ceil(phaseScaled);
        
        // Calculate change in ceiling
        double delta = currentCeiling - m_lastCeiling;
        bool currentStep = delta > 0.0;
        
        // Edge detection - only trigger on rising edge of step
        bool trigger = currentStep && !m_lastStep;
        
        // Update state for next sample
        m_lastCeiling = currentCeiling;
        m_lastStep = currentStep;
        
        return trigger;
    }
    
    void reset() {
        m_lastCeiling = -1.0;  // Prime to allow initial trigger
        m_lastStep = false;
    }
};

struct RampToSlope {
    float m_lastPhase{0.0f};
   
    float process(float currentPhase) {
        // Calculate ramp slope
        float delta = currentPhase - m_lastPhase;

        // Wrap delta to recenter between -0.5 and 0.5, for corrected slope during wrap
        if (delta > 0.5f)
            delta -= 1.0f;
        else if (delta < -0.5f)
            delta += 1.0f;    
    
        // Update state for next sample
        m_lastPhase = currentPhase;

        return delta;
    }
   
    void reset() {
        m_lastPhase = 0.0f;
    }
};

// ===== SCHEDULER CYCLE =====

struct SchedulerCycle {

    RampToTrig trigDetect;
   
    double m_phase{0.0};        
    double m_slope{0.0};       
    bool m_wrapNext{false};
   
    struct Output {
        bool trigger = false;
        float phase = 0.0f;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
   
    Output process(float rate, bool resetTrigger, float sampleRate) {
        Output output;
       
        // Handle reset
        if (resetTrigger) {
            reset();
        }
     
        // Initialize slope
        if (m_slope <= 0.0) {
            m_slope = rate / sampleRate;
        }
       
        // 1. Handle wrap from previous sample
        if (m_wrapNext) {
            m_phase -= 1.0;                 // Wrap the phase
            m_slope = rate / sampleRate;    // Latch new slope for next period
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
};

// ===== SCHEDULER BURST =====

struct SchedulerBurst {

    StepToTrig trigDetect;

    double m_phaseScaled{0.0};          
    double m_slope{0.0};          
    bool m_hasTriggered{false};    
   
    struct Output {
        bool trigger = false;
        float phase = 0.0f;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
   
    Output process(bool initTrigger, float duration, float cycles, float sampleRate) {
        Output output;
    
        // Reset on new trigger
        if (initTrigger) {
            reset();
            m_hasTriggered = true;
        }
    
        // Calculate slope
        if (duration > 0.0f) {
            m_slope = 1.0 / (duration * sampleRate);
        } else {
            m_slope = 1.0 / sampleRate;
        }
    
        // Process only if we have been triggered
        if (m_hasTriggered) {

            // 1. Detect trigger
            bool trigger = trigDetect.process(m_phaseScaled);

            // 2. Wrap scaled phase between 0 and 1
            double phase = sc_frac(m_phaseScaled);
        
            // 3. Calculate subsample offset
            double subSampleOffset = 0.0;
            if (trigger && m_slope != 0.0) {
                subSampleOffset = phase / m_slope;
            }
        
             // 4. Prepare output
            output.trigger = trigger;
            output.phase = static_cast<float>(phase);
            output.subSampleOffset = static_cast<float>(subSampleOffset);
            output.rate = static_cast<float>(m_slope * sampleRate);
            
            // 5. Increment phase
            m_phaseScaled += m_slope;

            // 6. Clip phase between 0 and cycles
            if (m_phaseScaled > cycles) {
                m_phaseScaled = cycles;
            }
        }
    
        return output;
    }
   
    void reset() {
        m_phaseScaled = 0.0;
        m_slope = 0.0;
        m_hasTriggered = false;
        trigDetect.reset();
    }
};

// ===== VOICE ALLOCATOR =====

template<int NumChannels>
struct VoiceAllocator {
    // Internal processing state
    std::array<double, NumChannels> localPhases{};      
    std::array<double, NumChannels> localSlopes{};      
    std::array<bool, NumChannels> isActive{};           
    
    // Output interface
    std::array<float, NumChannels> phases{};            
    std::array<bool, NumChannels> triggers{};           
    
    VoiceAllocator() = default;
    
    void process(bool trigger, float rate, float subSampleOffset, float sampleRate) {
        // Clear output triggers
        std::fill(triggers.begin(), triggers.end(), false);
        
        // 1. Free completed voices
        for (int ch = 0; ch < NumChannels; ++ch) {
            if (isActive[ch] && localPhases[ch] >= 1.0) {
                isActive[ch] = false;
                localPhases[ch] = 0.0;
            }
        }
        
        // 2. Allocate new voice if trigger
        if (trigger) {
            for (int ch = 0; ch < NumChannels; ++ch) {
                if (!isActive[ch]) {
                    localSlopes[ch] = rate / sampleRate;
                    localPhases[ch] = localSlopes[ch] * subSampleOffset;
                    isActive[ch] = true;
                    triggers[ch] = true;
                    break;
                }
            }
        }
        
        // 3. Output current phase
        for (int ch = 0; ch < NumChannels; ++ch) {
            if (isActive[ch] && localPhases[ch] < 1.0) {
                phases[ch] = static_cast<float>(localPhases[ch]);
            } else {
                phases[ch] = 0.0f;
            }
        }
        
        // 4. Increment all active voices
        for (int ch = 0; ch < NumChannels; ++ch) {
            if (isActive[ch]) {
                localPhases[ch] += localSlopes[ch];
            }
        }
    }
    
    void reset() {
        std::fill(localPhases.begin(), localPhases.end(), 0.0);
        std::fill(localSlopes.begin(), localSlopes.end(), 0.0);
        std::fill(isActive.begin(), isActive.end(), false);
        std::fill(phases.begin(), phases.end(), 0.0f);
        std::fill(triggers.begin(), triggers.end(), false);
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
            output = sc_frac(static_cast<float>(m_phase));
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

// ===== RAMP ACCUMULATOR =====

struct RampAccumulator {
    double m_sampleCount{0.0};
    bool m_hasTriggered{false};
   
    float process(bool trigger, float subSampleOffset) {
        // 1. Handle trigger - reset sample count with subsample offset
        if (trigger) {
            m_sampleCount = subSampleOffset;
            m_hasTriggered = true;
        }
       
        // 2. Output current count
        float output = 0.0f;
        if (m_hasTriggered) {
            output = static_cast<float>(m_sampleCount);
        }
   
        // 3. Increment sample count
        m_sampleCount++;
   
        return output;
    }
   
    void reset() {
        m_sampleCount = 0.0;
        m_hasTriggered = false;
    }
};

// ===== SHIFT REGISTER =====

struct ShiftRegister {
    int m_register{0};
    bool m_initialized{false};

    struct Output {
        float out3Bit = 0.0f;
        float out8Bit = 0.0f;
    };
    
    Output process(bool trigger, bool resetTrigger, float chance, int length, int rotation, RGen& rgen) {
        Output output;

        // Handle reset
        if (resetTrigger) {
            reset();
        }

        // Process trigger
        if (trigger) {
            if (!m_initialized) {
                // Initialize on first trigger
                m_register = 0;
                m_initialized = true;
            } else {
                // Rotate shift register
                int rotated = rotateBits(m_register, rotation, length);
                
                // Extract LSB for feedback
                int extractedBit = rotated % 2;
                int withoutLSB = rotated - extractedBit;
                
                // XOR with random value
                bool feedbackBit = (rgen.frand() < chance);
                int newBit = extractedBit ^ static_cast<int>(feedbackBit);
                
                // Update Shift Register
                m_register = withoutLSB + newBit;
            }
        }

        // Calculate outputs
        if (m_initialized) {
            output.out3Bit = getMSBBits(m_register, 3, 8);
            output.out8Bit = 1.0f - getLSBBits(m_register, 8, 8);
        }

        return output;
    }

    void reset() {
        m_register = 0;
        m_initialized = false;
    }
};

} // namespace Utils