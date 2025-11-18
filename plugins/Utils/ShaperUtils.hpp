#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "EventUtils.hpp"
#include <cmath>
#include <algorithm>

// ===== UNIT SHAPERS =====

namespace UnitShapers {

    inline float triangle(float phase, float skew) {

        // Add epsilon to prevent division by zero
        float safeSkew = std::max(skew, Utils::SAFE_DENOM_EPSILON);
        float safeInvSkew = std::max(1.0f - skew, Utils::SAFE_DENOM_EPSILON);
            
        float warpedPhase;
            
        if (phase <= skew) {
            warpedPhase = phase / safeSkew;
        } else {
            warpedPhase = 1.0f - ((phase - skew) / safeInvSkew);
        }
            
        // Handle edge case when skew is exactly 0
        if (skew < Utils::SAFE_DENOM_EPSILON) {
            warpedPhase = 1.0f - phase;
        }
            
        return warpedPhase;
    }

    inline float kink(float phase, float skew) {
        
        // Add epsilon to prevent division by zero
        float safeSkew = std::max(skew, Utils::SAFE_DENOM_EPSILON);
        float safeInvSkew = std::max(1.0f - skew, Utils::SAFE_DENOM_EPSILON);
            
        float warpedPhase;
            
        if (phase <= skew) {
            warpedPhase = 0.5f * (phase / safeSkew);
        } else {
            warpedPhase = 0.5f * (1.0f + ((phase - skew) / safeInvSkew));
        }
            
        // Handle edge case when skew is exactly 0
        if (skew < Utils::SAFE_DENOM_EPSILON) {
            warpedPhase = 0.5f * (1.0f + phase);
        }
            
        return warpedPhase;
    }

    inline float cubic(float phase, float index) {
        float indexScaled = index * 48.0f;
        float x = phase;
        float x2 = x * x;
        float x3 = x2 * x;
        
        return (x * (1.0f + (indexScaled / 6.0f))) +
               (x2 * (-indexScaled / 2.0f)) +
               (x3 * (indexScaled / 3.0f));
    }

    inline float hanning(float phase) {
        return 0.5f * (1.0f - std::cos(phase * Utils::PI));
    }

    inline float welch(float phase) {
        float term = phase - 1.0f;
        return 1.0f - (term * term);
    }

    inline float circular(float phase) {
        return std::sqrt(phase * (2.0f - phase));
    }

    inline float raisedCos(float phase, float index) {
        float cosine = std::cos(phase * Utils::PI);
        return std::exp(std::abs(index) * (-cosine - 1.0f));
    }

    inline float gaussian(float phase, float index) {
        float cosine = std::cos(phase * 0.5f * Utils::PI) * index;
        return std::exp(-cosine * cosine);
    }

    inline float trapezoid(float phase, float width, float duty = 1.0f) {
        float sustain = 1.0f - width;
        float offset = phase - (1.0f - duty);
        float trapezoid = ((offset / sustain) + (1.0f - duty));
        trapezoid = sc_clip(trapezoid, 0.0f, 1.0f);
        
        // When width = 1, it becomes a pulse
        if (width >= 1.0f - Utils::SAFE_DENOM_EPSILON) {
            return offset > 0.0f ? 1.0f : 0.0f;
        }
        
        return trapezoid;
    }

    inline float tukey(float phase, float width) {
        float warpedPhase = trapezoid(phase, width, 1.0f);
        return hanning(warpedPhase);
    }

    inline float planck(float phase, float width, float index) {
        float indexScaled = (index * 47.0f) + 1.0f;
        float warpedPhase = trapezoid(phase, width, 1.0f);
        return 1 / (1 + exp(indexScaled * ((1 / warpedPhase) - (1 / (1 - warpedPhase)))));
    }

} // namespace UnitShapers

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
                return Utils::cosineInterp(m_currentValue, m_nextValue, phase);
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
                return Utils::cosineInterp(m_currentValue, m_nextValue, phase);
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
                output.out3Bit = Utils::cosineInterp(m_current3Bit, m_next3Bit, phase);
                output.out8Bit = Utils::cosineInterp(m_current8Bit, m_next8Bit, phase);
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

// ===== EASING CORES =====

namespace EasingCores {

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

    // Pseudo-logarithmic core
    inline float pseudoLog2(float x) {
        float coef = 12.5f;
        return 1.0f - (std::log2((1.0f - x) * (std::pow(2.0f, coef) - 1.0f) + 1.0f) / coef);
    }

} // namespace EasingCores

// ===== EASING FUNCTIONS =====

namespace EasingFunctions {

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

        float safeOffset = std::max(offset, Utils::SAFE_DENOM_EPSILON);
        float safeInvOffset = std::max(1.0f - offset, Utils::SAFE_DENOM_EPSILON);
        
        if (x <= offset) {
            return offset * core(x / safeOffset);
        } else {
            return offset + ((1.0f - offset) * (1.0f - core((1.0f - x) / safeInvOffset)));
        }
    }

    // Seat with variable height (easeOutIn)
    template<typename CoreFunc>
    inline float easeOutIn(float x, float height, CoreFunc core) {

        float safeHeight = std::max(height, Utils::SAFE_DENOM_EPSILON);
        float safeInvHeight = std::max(1.0f - height, Utils::SAFE_DENOM_EPSILON);
        
        if (x <= height) {
            return height * (1.0f - core((height - x) / safeHeight));
        } else {
            return height + ((1.0f - height) * core((x - height) / safeInvHeight));
        }
    }

} // namespace EasingFunctions

// ===== INTERPOLATION FUNCTIONS =====

namespace InterpFunctions {

    // InterpEasing: linear interpolation of easing functions
    template<typename EasingFuncA, typename EasingFuncB>
    inline float interpEasing(float x, float shape, EasingFuncA easeA, EasingFuncB easeB) {

        auto easingToLinear = [&](float x, float shape, float easeValue) {
            float mix = shape * 2.0f;
            return easeValue * (1.0f - mix) + x * mix;
        };
            
        auto linearToEasing = [&](float x, float shape, float easeValue) {
            float mix = (shape - 0.5f) * 2.0f;
            return x * (1.0f - mix) + easeValue * mix;
        };
            
        if (shape <= 0.5f) {
            return easingToLinear(x, shape, easeA(x));
        } else {
            return linearToEasing(x, shape, easeB(x));
        }
    }

    // J-Curve: interpolate between easeOut and easeIn for any core
    template<typename CoreFunc>
    inline float jCurve(float x, float shape, CoreFunc core) {
        auto easeOutFunc = [&core](float t) { return EasingFunctions::easeOut(t, core); };
        auto easeInFunc = [&core](float t) { return EasingFunctions::easeIn(t, core); };
        return interpEasing(x, shape, easeOutFunc, easeInFunc);
    }

    // S-Curve: interpolate between sigmoid and seat for any core
    template<typename CoreFunc>
    inline float sCurve(float x, float shape, float inflection, CoreFunc core) {
        auto sigmoidFunc = [&core, inflection](float t) { return EasingFunctions::easeInOut(t, inflection, core); };
        auto seatFunc = [&core, inflection](float t) { return EasingFunctions::easeOutIn(t, inflection, core); };
        return interpEasing(x, shape, sigmoidFunc, seatFunc);
    }

} // namespace InterpFunctions

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
        return UnitShapers::tukey(warpedPhase, width);
    }

    inline float exponentialWindow(float phase, float skew, float shape) {
        float warpedPhase = UnitShapers::triangle(phase, skew);
        return InterpFunctions::jCurve(warpedPhase, 1.0f - shape, EasingCores::pseudoExp);
    }

} // namespace WindowFunctions