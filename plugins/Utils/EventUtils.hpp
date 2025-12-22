#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include <array>
#include <cmath>  
#include <algorithm>

namespace EventUtils {

// ===== TRIGGER UTILITIES =====

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
   
    Output process(bool initTrigger, float duration, int cycles, float sampleRate) {
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

// ===== RAMP DIVIDER =====

struct RampDivider {
    RampToTrig m_wrapDetect;
    RampToSlope m_slopeCalc;
    
    double m_phase{0.0};
    double m_lastRatio{1.0};
    bool m_syncRequest{false};
    
    float process(float phase, float ratio, bool resetTrigger, bool autosync, float threshold) {
        
        // Calculate slope and scale by ratio
        float safeRatio = sc_max(std::abs(ratio), Utils::SAFE_DENOM_EPSILON);
        float slope = m_slopeCalc.process(phase);
        float scaledSlope = slope / safeRatio;
        
        // Detect wrap trigger
        bool wrapTrigger = m_wrapDetect.process(phase);
        
        // Detect proportional change in ratio
        double delta = safeRatio - m_lastRatio;
        double sum = safeRatio + m_lastRatio;
        bool ratioChanged = (sum != 0.0) && (std::abs(delta / sum) > threshold);
        
        // Latch sync request (only if autosync enabled)
        if (ratioChanged && autosync) {
            m_syncRequest = true;
        }
        
        // Request sync on wrap trigger
        bool syncTrigger = false;
        if (wrapTrigger) {
            syncTrigger = m_syncRequest;
            m_syncRequest = false;
        }

        // Update phase: sync to grid on sync request or reset otherwise increment 
        if (syncTrigger || resetTrigger) {
            float scaledPhase = phase / safeRatio;
            double nextPhase = m_phase + scaledSlope;
            double offset = nextPhase - scaledPhase;
            double quantized = std::trunc(offset * safeRatio) / safeRatio;
            m_phase = quantized + scaledPhase;
        } else {
            m_phase += scaledSlope;
        }
        
        // Wrap phase between 0 and 1
        float output = sc_frac(static_cast<float>(m_phase));
        
        // Update state for next sample
        m_lastRatio = safeRatio;
        
        return output;
    }
    
    void reset() {
        m_wrapDetect.reset();
        m_slopeCalc.reset();
        m_phase = 0.0;
        m_lastRatio = 1.0;
        m_syncRequest = false;
    }
};

} // namespace EventUtils