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
        // SCs canonical trigger definition
        bool trigger = currentIn > 0.0f && m_prevIn <= 0.0f;
        
        // Update state for next sample
        m_prevIn = currentIn;
        
        return trigger;
    }
    
    void reset() {
        m_prevIn = 0.0f;
    }
};

struct TrigLatch {
    bool m_latchedTrigger{false};
    
    bool process(bool triggerIn, bool syncTrigger) {
        // Latch incoming trigger
        if (triggerIn) {
            m_latchedTrigger = true;
        }
        
        // Release trigger on request
        bool trigger = false;
        if (syncTrigger) {
            trigger = m_latchedTrigger;
            m_latchedTrigger = false;
        }
        
        return trigger;
    }
    
    void reset() {
        m_latchedTrigger = false;
    }
};

struct Change {
    double m_lastValue{0.0};
    
    int process(double currentValue) {

        // Detect direction of change as sign of delta
        double delta = currentValue - m_lastValue;
        int sign = static_cast<int>(sc_sign(delta));

        // Update state for next sample
        m_lastValue = currentValue;

        return sign;
    }
    
    void reset() {
        m_lastValue = 0.0;
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
    double m_lastPhase{0.0};
   
    double process(double currentPhase) {
        // Calculate ramp slope
        double delta = currentPhase - m_lastPhase;

        // Wrap between -0.5 and 0.5, for corrected slope during wrap
        if (delta > 0.5) delta -= 1.0;
        else if (delta < -0.5) delta += 1.0;  

        // Update state for next sample
        m_lastPhase = currentPhase;

        return delta;
    }
   
    void reset() {
        m_lastPhase = 0.0;
    }
};

// ===== SCHEDULER CYCLE =====

struct SchedulerCycle {
    RampToTrig wrapDetect;
   
    double m_phase{0.0};        
    double m_slope{0.0};      
   
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

        // 1. Wrap phase between 0 and 1
        m_phase = sc_frac(m_phase);

        // 2. Derive trigger from wrap
        bool trigger = wrapDetect.process(m_phase);

        // 3. Latch slope for new cycle
        if (trigger) {
            m_slope = static_cast<double>(rate) / sampleRate;
        }

        // 4. Calculate subsample offset
        double subSampleOffset = 0.0;
        if (trigger && m_slope != 0.0) {
            subSampleOffset = m_phase / m_slope;
        }

        // 5. Prepare output
        output.trigger = trigger;
        output.phase = static_cast<float>(m_phase);
        output.rate = static_cast<float>(m_slope * sampleRate);
        output.subSampleOffset = static_cast<float>(subSampleOffset);

        // 6. Increment phase
        m_phase += m_slope;

        return output;
    }
   
    void reset() {
        m_phase = 0.0;
        m_slope = 0.0;
        wrapDetect.reset();
    }
};

// ===== SCHEDULER BURST =====

struct SchedulerBurst {
    StepToTrig stepDetect;

    double m_phaseScaled{0.0};          
    double m_slope{0.0};          
    bool m_hasTriggered{false};    
   
    struct Output {
        bool trigger = false;
        float phase = 0.0f;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
   
    Output process(bool trigger, float duration, int cycles, float sampleRate) {
        Output output;
    
        // Reset on new trigger
        if (trigger) {
            reset();
            m_hasTriggered = true;
        }

        // Calculate slope from duration
        double safeDuration = sc_max(static_cast<double>(duration), 1.0 / sampleRate);
        m_slope = 1.0 / (safeDuration * sampleRate);

        // Process only if we have been triggered
        if (m_hasTriggered) {

            // 1. Clip scaled phase between 0 and cycles
            m_phaseScaled = sc_clip(m_phaseScaled, 0.0, static_cast<double>(cycles));

            // 2. Derive trigger from step
            bool trigger = stepDetect.process(m_phaseScaled);

            // 3. Wrap scaled phase between 0 and 1
            double phase = sc_frac(m_phaseScaled);
        
            // 4. Calculate subsample offset
            double subSampleOffset = 0.0;
            if (trigger && m_slope != 0.0) {
                subSampleOffset = phase / m_slope;
            }
        
             // 5. Prepare output
            output.trigger = trigger;
            output.phase = static_cast<float>(phase);
            output.rate = static_cast<float>(m_slope * sampleRate);
            output.subSampleOffset = static_cast<float>(subSampleOffset);
            
            // 6. Increment phase
            m_phaseScaled += m_slope;
        }
    
        return output;
    }
   
    void reset() {
        m_phaseScaled = 0.0;
        m_slope = 0.0;
        m_hasTriggered = false;
        stepDetect.reset();
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
                    localSlopes[ch] = static_cast<double>(rate) / sampleRate;
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
        double slope = static_cast<double>(rate) / sampleRate;
        
        // 2. Handle trigger - reset phase with subsample offset
        if (trigger) {
            m_phase = slope * subSampleOffset;
            m_hasTriggered = true;
        }
        
        // 3. Output current phase
        float output = 0.0f;
        if (m_hasTriggered) {
            output = static_cast<float>(sc_frac(m_phase));
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

// ===== RAMP DIVIDER (SIMPLE) =====

struct RampDividerSimple {
    RampToSlope m_slopeCalc;
    
    double m_phase{0.0};
    
    float process(double phase, float ratio, bool resetTrigger) {
        
        // 1. Derive slope from ramp and scale by ratio
        float safeRatio = sc_max(std::abs(ratio), Utils::SAFE_DENOM_EPSILON);
        double slope = m_slopeCalc.process(phase);
        double scaledSlope = slope / safeRatio;
        
        // 2. Update phase: reset to zero or free-running increment
        if (resetTrigger) {
            m_phase = 0.0;
        } else {
            m_phase += scaledSlope;
        }
        
        return static_cast<float>(sc_frac(m_phase));
    }
    
    void reset() {
        m_slopeCalc.reset();
        m_phase = 0.0;
    }
};

// ===== RAMP DIVIDER (GRID) =====

struct RampDividerGrid {
    RampToTrig m_wrapDetect;
    RampToSlope m_slopeCalc;
    TrigLatch m_syncLatch;
    
    double m_phase{0.0};
    double m_lastRatio{1.0};

    static constexpr double SYNC_THRESHOLD = 0.01;
    
    float process(double phase, float ratio, bool resetTrigger) {
        
        // 1. Derive slope from ramp and scale by ratio
        float safeRatio = sc_max(std::abs(ratio), Utils::SAFE_DENOM_EPSILON);
        double slope = m_slopeCalc.process(phase);
        double scaledSlope = slope / safeRatio;
        
        // 2. Derive trigger from wrap
        bool wrapTrigger = m_wrapDetect.process(phase);
        
        // 3. Detect proportional change in ratio above threshold
        double delta = safeRatio - m_lastRatio;
        double sum = safeRatio + m_lastRatio;
        bool ratioChanged = (sum != 0.0) && (std::abs(delta / sum) > SYNC_THRESHOLD);

        // 4. Latch sync request on ratio change, release on wrap
        bool syncTrigger = m_syncLatch.process(ratioChanged, wrapTrigger);

        // 5. Update phase: sync to grid on request or reset, otherwise free-running increment
        if (syncTrigger || resetTrigger) {
            double scaledPhase = phase / safeRatio;
            double nextPhase = m_phase + scaledSlope;
            double offset = nextPhase - scaledPhase;
            double quantized = std::trunc(offset * safeRatio) / safeRatio;
            m_phase = quantized + scaledPhase;
        } else {
            m_phase += scaledSlope;
        }
        
        // 6. Wrap phase between 0 and 1
        float output = static_cast<float>(sc_frac(m_phase));
        
        // 7. Update state for next sample
        m_lastRatio = safeRatio;
        
        return output;
    }
    
    void reset() {
        m_phase = 0.0;
        m_lastRatio = 1.0;
        m_wrapDetect.reset();
        m_slopeCalc.reset();
        m_syncLatch.reset();
    }
};

// ===== RAMP DIVIDER (OFFSET) =====

struct RampDividerOffset {
    RampToSlope m_slopeCalc;
    Change m_ratioChange;
    
    double m_target{0.0};
    double m_phase{0.0};
    double m_latchedSlope{0.0};
    bool m_lastSwitch{true};
    
    float process(double phase, float ratio, bool resetTrigger) {
        
        // 1. Derive slope from ramp and scale by ratio
        float safeRatio = sc_max(std::abs(ratio), Utils::SAFE_DENOM_EPSILON);
        double slope = m_slopeCalc.process(phase);
        double scaledSlope = slope / safeRatio;
        
        // 2. Latch slope when previous sample switched (held until next switch)
        if (m_lastSwitch) {
            m_latchedSlope = scaledSlope;
        }
        
        // 3. Detect any change in ratio
        bool ratioChanged = (m_ratioChange.process(safeRatio) != 0);
        
        // 4. Calculate next phase (needed for target sync and switch decision)
        double nextPhase = m_phase + m_latchedSlope;
        
        // 5. Update target: sync to grid on ratio change or reset, otherwise free-running increment
        if (ratioChanged || resetTrigger) {
            double scaledPhase = phase / safeRatio;
            double offset = nextPhase - scaledPhase;
            double quantized = std::trunc(offset * safeRatio) / safeRatio;
            m_target = quantized + scaledPhase;
        } else {
            m_target += scaledSlope;
        }
        m_target = sc_frac(m_target);
        
        // 6. Detect switch condition: phases close enough AND slopes differ
        double slopeDiff = std::abs(m_latchedSlope - scaledSlope);
        double phaseDiff = sc_wrap(nextPhase - m_target, -0.5, 0.5);
        bool phasesClose = std::abs(phaseDiff) < (slopeDiff * 2.0);
        bool canSwitch = phasesClose && (slopeDiff != 0.0);
        bool switchTrigger = canSwitch || resetTrigger;
        
        // 7. Update phase: switch to target on crossing, otherwise continue with latched slope
        if (switchTrigger) {
            m_phase = m_target;
        } else {
            m_phase = nextPhase;
        }
        m_phase = sc_frac(m_phase);

        // 8. Prepare output
        float output = static_cast<float>(m_phase);
        
        // 9. Update state for next sample
        m_lastSwitch = switchTrigger;
        
        return output;
    }
    
    void reset() {
        m_target = 0.0;
        m_phase = 0.0;
        m_latchedSlope = 0.0;
        m_lastSwitch = true;
        m_slopeCalc.reset();
        m_ratioChange.reset();
    }
};

} // namespace EventUtils