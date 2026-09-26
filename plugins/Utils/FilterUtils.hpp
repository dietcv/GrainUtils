#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include <array>

namespace FilterUtils {

// ===== FIRST ORDER FILTERS =====

struct LowpassOne {
    // State variable
    float m_state{0.0f};

    LowpassOne() = default;

    // Process audio through one pole lowpass
    inline float process(float x, float coeff) {
        m_state = x * (1.0f - coeff) + m_state * coeff;

        // Denormal protection
        m_state = zapgremlins(m_state);

        return m_state;
    }

    void reset() {
        m_state = 0.0f;
    }
};

struct HighpassOne {
    // State variable
    float m_state{0.0f};

    HighpassOne() = default;

    // Process audio through one pole highpass
    inline float process(float x, float coeff) {
        m_state = x * (1.0f - coeff) + m_state * coeff;

        // Denormal protection
        m_state = zapgremlins(m_state);

        return x - m_state;
    }

    void reset() {
        m_state = 0.0f;
    }
};

struct AllpassOne {
    // State variables
    float m_z1{0.0f};
    float m_z2{0.0f};

    AllpassOne() = default;

    // Process audio through first order allpass
    inline float process(float x, float coeff) {
        float y = m_z1 + coeff * (x - m_z2);

        // Denormal protection
        m_z1 = zapgremlins(x);
        m_z2 = zapgremlins(y);

        return y;
    }

    void reset() {
        m_z1 = 0.0f;
        m_z2 = 0.0f;
    }
};

// ===== ONE POLE COEFFICIENTS =====

struct OnePoleCoefficients {
    float g;
    float gt0, gt1;
    float m0, m1;
 
    enum FilterType {
        LOW_PASS,
        HIGH_PASS,
        ALL_PASS
    };
 
    static OnePoleCoefficients calculate(float cutoff, FilterType type, float sampleRate) {
        OnePoleCoefficients coeffs;
        
        // Calculate base g
        float w = (cutoff / sampleRate) * Utils::PI;
        float g0 = std::tan(w);
        
        // Set g and m coefficients based on filter type
        switch (type) {
            case LOW_PASS:
                coeffs.g = g0;
                coeffs.m0 = 0.0f;
                coeffs.m1 = 1.0f;
                break;
                
            case HIGH_PASS:
                coeffs.g = g0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = -1.0f;
                break;
                
            case ALL_PASS:
                coeffs.g = g0;
                coeffs.m0 = -1.0f;
                coeffs.m1 = 2.0f;
                break;
        }
        
        // Calculate shared coefficients
        float gInv = 1.0f / (1.0f + coeffs.g);
        coeffs.gt0 = coeffs.g * gInv;
        coeffs.gt1 = gInv;
        
        return coeffs;
    }
};

// ===== ONE POLE FILTER =====

struct OnePoleFilter {
    // State variable
    float m_ic1eq{0.0f};

    OnePoleFilter() = default;
    
    // Process audio through one pole filter
    inline float process(float vin, const OnePoleCoefficients& coeffs) {
        float v1 = coeffs.gt0 * vin + coeffs.gt1 * m_ic1eq;
        
        // State update
        m_ic1eq = 2.0f * v1 - m_ic1eq;
        
        // Denormal protection
        m_ic1eq = zapgremlins(m_ic1eq);
        
        // Mix outputs
        return coeffs.m0 * vin + coeffs.m1 * v1;
    }
    
    void reset() {
        m_ic1eq = 0.0f;
    }
};
 
// ===== DC BLOCKER =====

struct DCBlocker {
    OnePoleFilter filter;

    DCBlocker() = default;

    // Process audio through DC blocker
    inline float process(float x, float sampleRate) {

        // Calculate coefficients
        auto coeffs = OnePoleCoefficients::calculate(3.0f, OnePoleCoefficients::HIGH_PASS, sampleRate);

        return filter.process(x, coeffs);
    }

    void reset() {
        filter.reset();
    }
};

// ===== DAMPING FILTER =====

struct DampingFilter {
    OnePoleFilter filter;

    DampingFilter() = default;

    // Process audio through damping filter
    inline float process(float x, float damping, float sampleRate) {

        // Calculate cutoff from damping
        float safeSlope = (1.0f - sc_clip(damping, 0.0f, 1.0f)) * 0.49f;
        float cutoff = safeSlope * sampleRate;

        // Calculate coefficients
        auto coeffs = OnePoleCoefficients::calculate(cutoff, OnePoleCoefficients::LOW_PASS, sampleRate);

        return filter.process(x, coeffs);
    }

    void reset() {
        filter.reset();
    }
};

// ===== SLOPE-TRACKING FILTER =====

struct TrackingFilter {
    OnePoleFilter filter;

    TrackingFilter() = default;

    // Process audio through slope-tracking filter
    inline float process(float x, float slope, float sampleRate) {

        // Calculate cutoff from slope
        float safeSlope = std::abs(sc_clip(slope, -0.49f, 0.49f));
        float cutoff = safeSlope * sampleRate;

        // Calculate coefficients
        auto coeffs = OnePoleCoefficients::calculate(cutoff, OnePoleCoefficients::LOW_PASS, sampleRate);

        return filter.process(x, coeffs);
    }

    void reset() {
        filter.reset();
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

    // RBJ Audio EQ Cookbook highpass
    static BiquadCoefficients highpass(float freq, float q, float sampleRate) {
        BiquadCoefficients coeffs;
        
        float w0 = Utils::TWO_PI * freq / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);
        
        float a0 = 1.0f + alpha;
        
        coeffs.a1 = -2.0f * cosw0 / a0;
        coeffs.a2 = (1.0f - alpha) / a0;
        
        coeffs.b0 = ((1.0f + cosw0) / 2.0f) / a0;
        coeffs.b1 = -2.0f * coeffs.b0;
        coeffs.b2 = coeffs.b0;
        
        return coeffs;
    }
    
    // RBJ Audio EQ Cookbook bandpass (constant skirt gain, peak gain = Q)
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

    BiquadFilter() = default;
    
    // Process audio through biquad filter
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

// ===== BUTTERWORTH FILTER =====

template<int Order>
struct ButterworthFilter {
    static constexpr int NUM_BIQUADS = Order / 2;

    std::array<BiquadFilter, NUM_BIQUADS> filters;
    std::array<BiquadCoefficients, NUM_BIQUADS> coeffs;

    ButterworthFilter() = default;

    // Calculate Q values for Butterworth filter
    static std::array<float, NUM_BIQUADS> calculateButterQs() {
        std::array<float, NUM_BIQUADS> Qs{};

        for (int k = 1; k <= NUM_BIQUADS; ++k) {
            float t = static_cast<float>(2 * k + Order - 1);
            float alpha = -2.0f * std::cos(Utils::PI * t / (2.0f * static_cast<float>(Order)));
            Qs[NUM_BIQUADS - k] = 1.0f / alpha;
        }

        return Qs;
    }

    // Calculate coefficients for Butterworth lowpass
    void lowpass(float cutoff, float sampleRate) {

        auto Qs = calculateButterQs();

        for (int i = 0; i < NUM_BIQUADS; ++i) {
            coeffs[i] = BiquadCoefficients::lowpass(cutoff, Qs[i], sampleRate);
        }
    }

    // Calculate coefficients for Butterworth highpass
    void highpass(float cutoff, float sampleRate) {

        auto Qs = calculateButterQs();

        for (int i = 0; i < NUM_BIQUADS; ++i) {
            coeffs[i] = BiquadCoefficients::highpass(cutoff, Qs[i], sampleRate);
        }
    }

    // Process audio through Butterworth filter
    inline float process(float input) {

        float processed = input;
        for (int i = 0; i < NUM_BIQUADS; ++i) {
            processed = filters[i].process(processed, coeffs[i]);
        }

        return processed;
    }

    void reset() {
        for (int i = 0; i < NUM_BIQUADS; ++i) {
            filters[i].reset();
        }
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
        MORPH
    };
 
    static SVFCoefficients calculate(float cutoff, float q, FilterType type, float sampleRate, float shape = 0.0f) {
        SVFCoefficients coeffs;
        
        // Calculate base g and k
        float w = (cutoff / sampleRate) * Utils::PI;
        float g0 = std::tan(w);
        float k0 = 1.0f / q;
        
        // Set g, k, and m coefficients based on filter type
        switch (type) {
            case LOW_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 0.0f;
                coeffs.m1 = 0.0f;
                coeffs.m2 = 1.0f;
                break;

            case BAND_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 0.0f;
                coeffs.m1 = 1.0f;
                coeffs.m2 = 0.0f;
                break;
                
            case HIGH_PASS:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = 1.0f;
                coeffs.m1 = 0.0f;
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
                
            case MORPH:
                coeffs.g = g0;
                coeffs.k = k0;
                coeffs.m0 = sc_max(0.0f, 2.0f * shape - 1.0f);
                coeffs.m2 = sc_max(0.0f, 1.0f - 2.0f * shape);
                coeffs.m1 = 1.0f - coeffs.m0 - coeffs.m2;
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

    StateVariableFilter() = default;
    
    // Process audio through state variable filter
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

// ===== MORPHING FILTER =====

struct MorphingFilter {
    StateVariableFilter svf;

    MorphingFilter() = default;
 
    // Process audio through state variable filter
    inline float process(float input, float freq, float resonance, float shape, float sampleRate) {
 
        // Convert resonance (0 - 1) to Q (0.707 - 25.0)
        float q = 0.707f + sc_squared(resonance) * 24.293f;
  
        // Calculate coefficients
        auto coeffs = SVFCoefficients::calculate(freq, q, SVFCoefficients::MORPH, sampleRate, shape);
 
        // Process audio through SVF
        float processed = svf.process(input, coeffs);
 
        return processed;
    }
 
    void reset() {
        svf.reset();
    }
};

// ===== ALLPASS CHAIN =====

template<int NumAllpasses>
struct AllpassChain {
    std::array<StateVariableFilter, NumAllpasses> allpasses;
    
    AllpassChain() = default;
    
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