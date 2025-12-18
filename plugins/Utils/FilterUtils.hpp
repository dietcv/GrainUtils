#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include <array>
#include <cmath>  

namespace FilterUtils {

// ===== ONE POLE FILTERS =====

namespace OnePole {
    // Core processing functions
    inline float lowpass(float& state, float input, float coeff) {
        state = input * (1.0f - coeff) + state * coeff;
        return state;
    }
    
    inline float highpass(float& state, float input, float coeff) {
        state = input * (1.0f - coeff) + state * coeff;
        return input - state;
    }
}

struct OnePoleDirect {
    float m_state{0.0f};
    
    float processLowpass(float input, float coeff) {
        coeff = sc_clip(coeff, 0.0f, 1.0f);
        return OnePole::lowpass(m_state, input, coeff);
    }

    float processHighpass(float input, float coeff) {
        coeff = sc_clip(coeff, 0.0f, 1.0f);
        return OnePole::highpass(m_state, input, coeff);
    }

    void reset() { 
        m_state = 0.0f; 
    }
};

struct OnePoleSlope {
    float m_state{0.0f};
      
    float processLowpass(float input, float slope) {
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
        float coeff = std::exp(-Utils::TWO_PI * safeSlope);
        return OnePole::lowpass(m_state, input, coeff);
    }
   
    float processHighpass(float input, float slope) {
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
        float coeff = std::exp(-Utils::TWO_PI * safeSlope);
        return OnePole::highpass(m_state, input, coeff);
    }

    void reset() { 
        m_state = 0.0f; 
    }
};

struct OnePoleHz {
    float m_state{0.0f};
   
    float processLowpass(float input, float freq, float sampleRate) {
        float slope = freq / sampleRate;
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
        float coeff = std::exp(-Utils::TWO_PI * safeSlope);
        return OnePole::lowpass(m_state, input, coeff);
    }
   
    float processHighpass(float input, float freq, float sampleRate) {
        float slope = freq / sampleRate;
        float safeSlope = std::abs(sc_clip(slope, -0.5f, 0.5f));
        float coeff = std::exp(-Utils::TWO_PI * safeSlope);
        return OnePole::highpass(m_state, input, coeff);
    }

    void reset() { 
        m_state = 0.0f; 
    }
};

// ===== BIQUAD COEFFICIENTS =====

struct BiquadCoefficients {
    float a1, a2;
    float b0, b2;

    // RBJ Audio EQ Cookbook lowpass
    static BiquadCoefficients lowpass(float freq, float q, float sampleRate) {
        BiquadCoefficients coeffs;
        
        float w0 = Utils::TWO_PI * freq / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);
        
        float a0 = 1.0f + alpha;
        
        coeffs.a1 = -2.0f * cosw0 / a0;
        coeffs.a2 = (1.0f - alpha) / a0;
        coeffs.b0 = ((1.0f - cosw0) / 2.0f) / a0;
        coeffs.b2 = coeffs.b0;  // b2 = b0 for lowpass
        
        return coeffs;
    }
    
    // RBJ Audio EQ Cookbook bandpass
    static BiquadCoefficients bandpass(float freq, float q, float sampleRate) {
        BiquadCoefficients coeffs;
        
        float w0 = Utils::TWO_PI * freq / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);
        
        float a0 = 1.0f + alpha;
        
        coeffs.a1 = -2.0f * cosw0 / a0;
        coeffs.a2 = (1.0f - alpha) / a0;
        coeffs.b0 = (sinw0 / 2.0f) / a0;   // Constant skirt gain: peak gain = Q
        coeffs.b2 = -(sinw0 / 2.0f) / a0;
        
        return coeffs;
    }

    // RBJ Audio EQ Cookbook allpass
    static BiquadCoefficients allpass(float freq, float q, float sampleRate) {
        BiquadCoefficients coeffs;
        
        float w0 = Utils::TWO_PI * freq / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);
        
        float a0 = 1.0f + alpha;
        
        coeffs.a1 = -2.0f * cosw0 / a0;
        coeffs.a2 = (1.0f - alpha) / a0;
        coeffs.b0 = 0.0f;  // Not used for allpass
        coeffs.b2 = 0.0f;  // Not used for allpass
        
        return coeffs;
    }
};

// ===== BIQUAD LOWPASS FILTER =====

struct BiquadLowpass_TDF2 {
    float z1{0.0f}, z2{0.0f};
    
    inline float process(float x, const BiquadCoefficients& coeffs) {
        float y = coeffs.b0 * x + z1;
        z1 = 2.0f * coeffs.b0 * x - coeffs.a1 * y + z2;
        z2 = coeffs.b0 * x - coeffs.a2 * y;
        
        z1 = zapgremlins(z1);
        z2 = zapgremlins(z2);
        
        return y;
    }
    
    void reset() {
        z1 = z2 = 0.0f;
    }
};

// ===== BIQUAD BANDPASS FILTER =====

struct BiquadBandpass_TDF2 {
    float z1{0.0f}, z2{0.0f};
    
    inline float process(float x, const BiquadCoefficients& coeffs) {
        float y = coeffs.b0 * x + z1;
        z1 = -coeffs.a1 * y + z2;
        z2 = coeffs.b2 * x - coeffs.a2 * y;
        
        z1 = zapgremlins(z1);
        z2 = zapgremlins(z2);
        
        return y;
    }
    
    void reset() {
        z1 = z2 = 0.0f;
    }
};

// ===== BIQUAD ALLPASS FILTER =====

struct BiquadAllpass_TDF2 {
    float z1{0.0f}, z2{0.0f};
    
    inline float process(float x, const BiquadCoefficients& coeffs) {
        float y = coeffs.a2 * x + z1;
        z1 = coeffs.a1 * x - coeffs.a1 * y + z2;
        z2 = x - coeffs.a2 * y;
        
        z1 = zapgremlins(z1);
        z2 = zapgremlins(z2);
        
        return y;
    }
    
    void reset() {
        z1 = z2 = 0.0f;
    }
};

// ===== BIQUAD ALLPASS CASCADE =====

template<int NumAllpasses>
struct AllpassCascade {
    std::array<BiquadAllpass_TDF2, NumAllpasses> allpasses;
    
    AllpassCascade() = default;
    
    // Process audio through cascaded allpass filters
    inline float process(float input, float freq, float resonance, float sampleRate) {

        // Convert resonance (0 - 1) to Q (0.5 - 2.0)
        float q = 0.5f + std::sqrt(sc_clip(resonance, 0.0f, 1.0f)) * 1.5f;
        
        // Calculate coefficients
        auto coeffs = BiquadCoefficients::allpass(freq, q, sampleRate);
        
        // Cascade allpass filters
        float processed = input;
        for (int i = 0; i < NumAllpasses; ++i) {
            processed = allpasses[i].process(processed, coeffs);
        }
        
        return processed;
    }
    
    void reset() {
        for (int i = 0; i < NumAllpasses; ++i) {
            allpasses[i].reset();
        }
    }
};

} // namespace FilterUtils