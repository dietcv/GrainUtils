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
    float b0, b1, b2;

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
        coeffs.b1 = 2.0f * coeffs.b0;
        coeffs.b2 = coeffs.b0;
        
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

        coeffs.b0 = (sinw0 / 2.0f) / a0;
        coeffs.b1 = 0.0f;                  
        coeffs.b2 = -coeffs.b0;            
        
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

        coeffs.b0 = coeffs.a2;
        coeffs.b1 = coeffs.a1;
        coeffs.b2 = 1.0f;
        
        return coeffs;
    }
};

// ===== BIQUAD FILTER (TDF-II) =====

struct BiquadFilter {
    // State variables
    float m_z1{0.0f};
    float m_z2{0.0f};
    
    inline float process(float x, const BiquadCoefficients& coeffs) {
        float y = coeffs.b0 * x + m_z1;
        m_z1 = coeffs.b1 * x - coeffs.a1 * y + m_z2;
        m_z2 = coeffs.b2 * x - coeffs.a2 * y;
        
        // Denormal protection
        m_z1 = zapgremlins(m_z1);
        m_z2 = zapgremlins(m_z2);
        
        return y;
    }
    
    void reset() {
        m_z1 = 0.0f;
        m_z2 = 0.0f;
    }
};

// ===== SVF COEFFICIENTS =====

struct SVFCoefficients {
    float g, k;
    float gt0, gk0;
    float gt1, gk1, gt2;
    float m0, m1, m2;

    enum FilterType {
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS,
        NOTCH,
        PEAK,
        ALL_PASS,
        BELL,
        LOW_SHELF,
        HIGH_SHELF
    };

    static SVFCoefficients calculate(float cutoff, float q, FilterType type, float sampleRate, float gainDb = 0.0f) {
        SVFCoefficients coeffs;
        
        // Calculate base g and k
        float w = (cutoff / sampleRate) * Utils::PI;
        float g0 = std::tan(w);
        float k0 = 1.0f / q;
        
        // For shelving and bell filters
        float A = std::pow(10.0f, gainDb / 40.0f);
        
        // Set g, k, and m coefficients based on filter type
        switch (type) {
            case LOW_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 0.0f;
                coeffs.m1 = 0.0f;
                coeffs.m2 = 1.0f;
                break;
                
            case HIGH_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = 0.0f;
                coeffs.m2 = 0.0f;
                break;
                
            case BAND_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 0.0f;
                coeffs.m1 = 1.0f;
                coeffs.m2 = 0.0f;
                break;
                
            case NOTCH:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = 0.0f;
                coeffs.m2 = 1.0f;
                break;
                
            case PEAK:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = 0.0f;
                coeffs.m2 = -1.0f;
                break;
                
            case ALL_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = -k0;
                coeffs.m2 = 1.0f;
                break;
                
            case BELL:
                coeffs.g = g0;
                coeffs.k = k0 / A;
                coeffs.m0 = 1.0f;
                coeffs.m1 = k0 * A;
                coeffs.m2 = 1.0f;
                break;
                
            case LOW_SHELF:
                coeffs.g = g0 / std::sqrt(A);
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = k0 * A;
                coeffs.m2 = A * A;
                break;
                
            case HIGH_SHELF:
                coeffs.g = g0 * std::sqrt(A);
                coeffs.k = k0;
                coeffs.m0 = A * A;
                coeffs.m1 = k0 * A;
                coeffs.m2 = 1.0f;
                break;
        }
        
        // Calculate shared coefficients
        float gk = coeffs.g + coeffs.k;
        coeffs.gt0 = 1.0f / (1.0f + coeffs.g * gk);
        coeffs.gk0 = gk * coeffs.gt0;
        
        // For parallel version
        coeffs.gt1 = coeffs.g * coeffs.gt0;
        coeffs.gk1 = coeffs.g * coeffs.gk0;
        coeffs.gt2 = coeffs.g * coeffs.gt1;
        
        return coeffs;
    }
};

// ===== STATE VARIABLE FILTER =====

struct StateVariableFilter {
    // State variables
    float m_ic1eq{0.0f};
    float m_ic2eq{0.0f};
    
    inline float process(float vin, const SVFCoefficients& coeffs) {
        float t0 = vin - m_ic2eq;
        float v0 = coeffs.gt0 * t0 - coeffs.gk0 * m_ic1eq;
        float t1 = coeffs.gt1 * t0 - coeffs.gk1 * m_ic1eq;
        float t2 = coeffs.gt2 * t0 + coeffs.gt1 * m_ic1eq;
        
        float v1 = m_ic1eq + t1;
        float v2 = m_ic2eq + t2;
        
        // State update
        m_ic1eq += 2.0f * t1;
        m_ic2eq += 2.0f * t2;
        
        // Denormal protection
        m_ic1eq = zapgremlins(m_ic1eq);
        m_ic2eq = zapgremlins(m_ic2eq);
        
        // Mix outputs
        return coeffs.m0 * v0 + coeffs.m1 * v1 + coeffs.m2 * v2;
    }
    
    void reset() {
        m_ic1eq = 0.0f;
        m_ic2eq = 0.0f;
    }
};

// ===== SVF ALLPASS CASCADE =====

template<int NumAllpasses>
struct AllpassCascade {
    std::array<StateVariableFilter, NumAllpasses> allpasses;
    
    AllpassCascade() = default;
    
    // Process audio through cascaded allpass filters
    inline float process(float input, float freq, float resonance, float sampleRate) {

        // Convert resonance (0 - 1) to Q (0.707 - 2.0)
        float q = 0.707f + std::sqrt(sc_clip(resonance, 0.0f, 1.0f)) * 1.293f;
        
        // Calculate coefficients
        auto coeffs = SVFCoefficients::calculate(freq, q, SVFCoefficients::ALL_PASS, sampleRate);
        
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