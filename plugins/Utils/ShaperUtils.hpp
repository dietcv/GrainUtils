#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include <cmath>
#include <algorithm>

// ===== UNIT SHAPERS =====

namespace UnitShapers {

    inline float triangle(float phase, float skew) {

        // Handle edge case when skew is exactly 0
        if (skew < Utils::SAFE_DENOM_EPSILON) {
            return 1.0f - phase;
        }
        
        float safeInvSkew = sc_max(1.0f - skew, Utils::SAFE_DENOM_EPSILON);
        
        if (phase <= skew) {
            return phase / skew;
        } else {
            return 1.0f - ((phase - skew) / safeInvSkew);
        }
    }

    inline float kink(float phase, float skew) {
        
        // Handle edge case when skew is exactly 0
        if (skew < Utils::SAFE_DENOM_EPSILON) {
            return 0.5f * (1.0f + phase);
        }
        
        float safeInvSkew = sc_max(1.0f - skew, Utils::SAFE_DENOM_EPSILON);
        
        if (phase <= skew) {
            return 0.5f * (phase / skew);
        } else {
            return 0.5f * (1.0f + ((phase - skew) / safeInvSkew));
        }
    }

    inline float cubic(float phase, float index) {
        float indexScaled = index * 48.0f;
        float x1 = phase;
        float x2 = x1 * x1;
        float x3 = x2 * x1; 
        return (x1 * (1.0f + (indexScaled / 6.0f))) +
               (x2 * (-indexScaled / 2.0f)) +
               (x3 * (indexScaled / 3.0f));
    }

    inline float hanning(float phase) {
        return 0.5f * (1.0f - std::cos(phase * Utils::PI));
    }

    inline float welch(float phase) {
        float x1 = phase - 1.0f;
        return 1.0f - (x1 * x1);
    }

    inline float circular(float phase) {
        return std::sqrt(phase * (2.0f - phase));
    }

    inline float raisedCos(float phase, float index) {
        float cosine = std::cos(phase * Utils::PI);
        return std::exp(index * (-cosine - 1.0f));
    }

    inline float gaussian(float phase, float index) {
        float cosine = std::cos(phase * 0.5f * Utils::PI) * index;
        return std::exp(-cosine * cosine);
    }

    inline float trapezoid(float phase, float width, float duty) {
        float sustain = 1.0f - width;
        
        // Handle edge case when sustain is exactly 0
        if (sustain < Utils::SAFE_DENOM_EPSILON) {
            float offset = phase - (1.0f - duty);
            return offset > 0.0f ? 1.0f : 0.0f;
        }
        
        float offset = phase - (1.0f - duty);
        float trapezoid = ((offset / sustain) + (1.0f - duty));
        
        return sc_clip(trapezoid, 0.0f, 1.0f);
    }

} // namespace UnitShapers

// ===== EASING FUNCTIONS =====

namespace Easing {

    // ===== EASING CORES =====
    
    namespace Cores {
        
        // Cubic core
        inline float cubic(float x) {
            return x * x * x;
        }
        
        // Quintic core
        inline float quintic(float x) {
            return x * x * x * x * x;
        }
        
        // Sine core
        inline float sine(float x) {
            return 1.0f - std::cos(x * 0.5f * Utils::PI);
        }
        
        // Circular core
        inline float circular(float x) {
            return 1.0f - std::sqrt(1.0f - (x * x));
        }
        
        // Pseudo-exponential core
        inline float pseudoExp(float x) {
            float coef = 13.0f;
            return (std::pow(2.0f, coef * x) - 1.0f) / (std::pow(2.0f, coef) - 1.0f);
        }
             
    } // namespace Cores
    
    // ===== EASING TYPES =====

    namespace Types {
    
        template<typename CoreFunc>
        inline float easeIn(float x, CoreFunc core) {
            return core(x);
        }
    
        template<typename CoreFunc>
        inline float easeOut(float x, CoreFunc core) {
            return 1.0f - core(1.0f - x);
        }
    
        // Sigmoid with variable offset (easeInOut)
        template<typename CoreFunc>
        inline float easeInOut(float x, float offset, CoreFunc core) {
            
            // Add epsilon to prevent division by zero
            float safeOffset = sc_max(offset, Utils::SAFE_DENOM_EPSILON);
            float safeInvOffset = sc_max(1.0f - offset, Utils::SAFE_DENOM_EPSILON);
            
            if (x <= offset) {
                return offset * core(x / safeOffset);
            } else {
                return offset + ((1.0f - offset) * (1.0f - core((1.0f - x) / safeInvOffset)));
            }
        }
    
        // Seat with variable height (easeOutIn)
        template<typename CoreFunc>
        inline float easeOutIn(float x, float height, CoreFunc core) {

            // Add epsilon to prevent division by zero
            float safeHeight = sc_max(height, Utils::SAFE_DENOM_EPSILON);
            float safeInvHeight = sc_max(1.0f - height, Utils::SAFE_DENOM_EPSILON);
            
            if (x <= height) {
                return height * (1.0f - core((height - x) / safeHeight));
            } else {
                return height + ((1.0f - height) * core((x - height) / safeInvHeight));
            }
        }

    } // namespace Types
    
    // ===== EASING INTERPOLATION =====

    namespace Interp {

        // J-Curve: interpolate between easeOut and easeIn for any core
        template<typename CoreFunc>
        inline float jCurve(float x, float shape, CoreFunc core) {
            if (shape <= 0.5f) {
                float mix = shape * 2.0f;
                return lininterp(mix, Types::easeOut(x, core), x);
            } else {
                float mix = (shape - 0.5f) * 2.0f;
                return lininterp(mix, x, Types::easeIn(x, core));
            }
        }

        // S-Curve: interpolate between sigmoid and seat for any core
        template<typename CoreFunc>
        inline float sCurve(float x, float shape, float inflection, CoreFunc core) {
            if (shape <= 0.5f) {
                float mix = shape * 2.0f;
                return lininterp(mix, Types::easeInOut(x, inflection, core), x);
            } else {
                float mix = (shape - 0.5f) * 2.0f;
                return lininterp(mix, x, Types::easeOutIn(x, inflection, core));
            }
        }

    } // namespace Interp

} // namespace Easing

// ===== WINDOW FUNCTIONS =====

namespace WindowFunctions {

    inline float hanningWindow(float phase, float skew) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        return UnitShapers::hanning(warpedPhase);
    }

    inline float gaussianWindow(float phase, float skew, float index) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        float gaussian = UnitShapers::gaussian(warpedPhase, index);
        float hanning = UnitShapers::hanning(warpedPhase);
        return gaussian * hanning;
    }

    inline float trapezoidalWindow(float phase, float skew, float width, float duty) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        return UnitShapers::trapezoid(warpedPhase, width, duty);
    }

    inline float tukeyWindow(float phase, float skew, float width) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        float trapezoid = UnitShapers::trapezoid(warpedPhase, width, 1.0f);
        return UnitShapers::hanning(trapezoid);
    }

    inline float exponentialWindow(float phase, float skew, float shape) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        return Easing::Interp::jCurve(warpedPhase, 1.0f - shape, Easing::Cores::pseudoExp);
    }

} // namespace WindowFunctions

// ===== UNIT STEPS =====

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
                m_register = static_cast<int>(rgen.frand() * 255.0f);
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