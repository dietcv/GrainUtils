#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include <cmath>
#include <algorithm>

namespace UnitSteps {

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
                return Utils::cosInterp(phase, m_currentValue, m_nextValue);
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
                return Utils::cosInterp(phase, m_currentValue, m_nextValue);
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
                m_current3Bit = Utils::getMSBBits(m_register, 3, 8);
                m_current8Bit = 1.0f - Utils::getLSBBits(m_register, 8, 8);
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
                int rotated = Utils::rotateBits(m_register, rotation, length);
                
                // Extract LSB for feedback
                int extractedBit = rotated % 2;
                int withoutLSB = rotated - extractedBit;
                
                // XOR with random value
                bool feedbackBit = rgen.frand() < chance;
                int newBit = extractedBit ^ static_cast<int>(feedbackBit);
                
                // Update Shift Register
                m_register = withoutLSB + newBit;
                
                // Calculate next values
                m_next3Bit = Utils::getMSBBits(m_register, 3, 8);
                m_next8Bit = 1.0f - Utils::getLSBBits(m_register, 8, 8);
            }
            
            // Interpolation: true for cosine, false for stepped
            if (interp) {
                output.out3Bit = Utils::cosInterp(phase, m_current3Bit, m_next3Bit);
                output.out8Bit = Utils::cosInterp(phase, m_current8Bit, m_next8Bit);
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