#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"

namespace UnitSteps {

    // ===== BIT MANIPULATION =====

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

    // ===== COSINE INTERPOLATION =====

    inline float cosInterp(float x, float a, float b) {
        float mix = (1.0f - std::cos(x * Utils::PI)) * 0.5f;
        return lininterp(mix, a, b);
    }

    // ===== UNIT STEP =====

    struct UnitStep {
        EventUtils::RampToTrig m_trigDetect;

        float m_currentValue{0.0f};
        float m_nextValue{0.0f};
        bool m_initialized{false};
        
        float process(float phase, bool interp, RGen& rgen) {

            // Initialize
            if (!m_initialized) {
                m_currentValue = rgen.frand();
                m_nextValue = m_currentValue;
                m_initialized = true;
            }

            // Detect trigger
            bool trigger = m_trigDetect.process(phase);
            
            // Get random value for each trigger
            if (trigger) {
                m_currentValue = m_nextValue;
                m_nextValue = rgen.frand();
            }
            
            // Interpolation: true for cosine, false for stepped
            if (interp) {
                return cosInterp(phase, m_currentValue, m_nextValue);
            } else {
                return m_currentValue;
            }
        }
        
        void reset() {
            m_currentValue = 0.0f;
            m_nextValue = 0.0f;
            m_initialized = false;
            m_trigDetect.reset();
        }
    };

    // ===== UNIT WALK =====

    struct UnitWalk {
        EventUtils::RampToTrig m_trigDetect;

        float m_currentValue{0.0f};
        float m_nextValue{0.0f};
        bool m_initialized{false};
        
        float process(float phase, float step, bool interp, RGen& rgen) {

            // Initialize
            if (!m_initialized) {
                m_currentValue = rgen.frand();
                m_nextValue = m_currentValue;
                m_initialized = true;
            }

            // Detect trigger
            bool trigger = m_trigDetect.process(phase);
            
            // Make a random step for each trigger (gaussian distribution)
            if (trigger) {
                m_currentValue = m_nextValue;
                m_nextValue += rgen.fsum3rand() * step;
                m_nextValue = sc_fold(m_nextValue, 0.0f, 1.0f);
            }
            
            // Interpolation: true for cosine, false for stepped
            if (interp) {
                return cosInterp(phase, m_currentValue, m_nextValue);
            } else {
                return m_currentValue;
            }
        }
        
        void reset() {
            m_currentValue = 0.0f;
            m_nextValue = 0.0f;
            m_initialized = false;
            m_trigDetect.reset();
        }
    };

    // ===== UNIT REGISTER =====

    struct UnitRegister {
        EventUtils::RampToTrig m_trigDetect;

        int m_register{0};
        float m_current3Bit{0.0f};
        float m_current8Bit{0.0f};
        float m_next3Bit{0.0f};
        float m_next8Bit{0.0f};
        bool m_initialized{false};
        
        struct Output {
            float out3Bit = 0.0f;
            float out8Bit = 0.0f;
        };
        
        Output process(float phase, float chance, int length, int rotation, bool interp, bool resetTrigger, RGen& rgen) {
            Output output;

            // Handle reset
            if (resetTrigger) {
                reset();
            }

            // Initialize
            if (!m_initialized) {
                m_register = rgen.irand(256);
                m_current3Bit = getMSBBits(m_register, 3, 8);
                m_current8Bit = 1.0f - getLSBBits(m_register, 8, 8);
                m_next3Bit = m_current3Bit;
                m_next8Bit = m_current8Bit;
                m_initialized = true;
            }

            // Detect trigger
            bool trigger = m_trigDetect.process(phase);
            
            // Get new shift register value for each trigger
            if (trigger) {
                m_current3Bit = m_next3Bit;
                m_current8Bit = m_next8Bit;
                
                // Rotate shift register
                int rotated = rotateBits(m_register, rotation, length);
                
                // Extract LSB for feedback
                int extractedBit = rotated % 2;
                int withoutLSB = rotated - extractedBit;
                
                // XOR with random value
                bool feedbackBit = rgen.frand() < chance;
                int newBit = extractedBit ^ static_cast<int>(feedbackBit);
                
                // Update Shift Register
                m_register = withoutLSB + newBit;
                
                // Calculate next values
                m_next3Bit = getMSBBits(m_register, 3, 8);
                m_next8Bit = 1.0f - getLSBBits(m_register, 8, 8);
            }
            
            // Interpolation: true for cosine, false for stepped
            if (interp) {
                output.out3Bit = cosInterp(phase, m_current3Bit, m_next3Bit);
                output.out8Bit = cosInterp(phase, m_current8Bit, m_next8Bit);
            } else {
                output.out3Bit = m_current3Bit;
                output.out8Bit = m_current8Bit;
            }
            
            return output;
        }
        
        void reset() {
            m_register = 0;
            m_current3Bit = 0.0f;
            m_current8Bit = 0.0f;
            m_next3Bit = 0.0f;
            m_next8Bit = 0.0f;
            m_initialized = false;
            m_trigDetect.reset();
        }
    };

} // namespace UnitSteps