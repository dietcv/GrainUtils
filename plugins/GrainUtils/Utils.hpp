#pragma once
#include "SC_PlugIn.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace Utils {

// ===== TRIGGER AND TIMING UTILITIES =====

struct RampToTrig {
    double lastPhase = 0.0f;
    bool lastWrap = false;
    
    bool process(double currentPhase) {
        // Detect wrap using current vs last phase
        double delta = currentPhase - lastPhase;
        double sum = currentPhase + lastPhase;
        bool currentWrap = (sum != 0.0f) && (std::abs(delta / sum) > 0.5f);
        
        // Edge detection - only trigger on rising edge of wrap
        bool trigger = currentWrap && !lastWrap;
        
        // Update state for next sample
        lastPhase = currentPhase;
        lastWrap = currentWrap;
        
        return trigger;
    }
    
    void reset() {
        lastPhase = 0.0f;
        lastWrap = false;
    }
};

// ===== EVENT SCHEDULER =====

struct EventScheduler {
    // Core timing components
    RampToTrig trigDetect;
    
    // Ramp state
    double phase = 0.0f;        // Current ramp position [0,1)
    double prevPhase = 0.0f;    // Previous sample's ramp position
    double slope = 0.0f;        // Current slope (rate/sampleRate)
    bool wrapNext = false;     // Flag: will wrap on next sample
    
    struct EventOutput {
        bool trigger = false;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
    
    EventOutput process(float triggerRate, bool resetTrigger, float sampleRate) {
        EventOutput output;
        
        // Handle reset
        if (resetTrigger) {
            reset();
        }
        
        // Initialize on first sample
        if (slope == 0.0f) {
            slope = triggerRate / sampleRate;
        }
        
        // 1. Handle wrap from previous sample
        if (wrapNext) {
            phase -= 1.0f;                      // Wrap the phase
            slope = triggerRate / sampleRate;   // Latch new slope for next period
            wrapNext = false;
        }
        
        // 2. Detect trigger using previous sample's phase
        bool trigger = trigDetect.process(prevPhase);
        
        // 3. Calculate subsample offset when trigger occurs
        double subSampleOffset = 0.0f;
        if (trigger) {
            subSampleOffset = prevPhase / slope;
        }
        
        // 4. Prepare output
        output.trigger = trigger;
        output.rate = static_cast<float>(slope * sampleRate);
        output.subSampleOffset = static_cast<float>(subSampleOffset);
        
        // 5. Update for next sample
        prevPhase = phase;          // Store current as previous
        phase += slope;             // Increment current
        
        // 6. Check for wrap
        if (phase >= 1.0f) {
            wrapNext = true;
        }
        
        return output;
    }
    
    void reset() {
        phase = prevPhase = 0.0f;
        slope = 0.0f;
        wrapNext = false;
        trigDetect.reset();
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
    double phase = 0.0f;
    bool hasTriggered = false;
    
    float process(bool trigger, float rate, float subSampleOffset, float sampleRate) {

        double slope = rate / sampleRate;
        
        // Handle trigger - reset phase with subsample offset
        if (trigger) {
            phase = slope * subSampleOffset;
            hasTriggered = true;
        }
        
        // Output current phase, then increment
        float output = hasTriggered ? sc_wrap(static_cast<float>(phase), 0.0f, 1.0f) : 0.0f;
        phase += slope;
        
        return output;
    }
    
    void reset() {
        phase = 0.0f;
        hasTriggered = false;
    }
};

} // namespace Utils