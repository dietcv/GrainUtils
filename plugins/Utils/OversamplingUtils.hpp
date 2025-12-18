#pragma once
#include "FilterUtils.hpp"
#include <array>
#include <algorithm>

namespace OversamplingUtils {

// ===== HIGH-ORDER BUTTERWORTH LOWPASS =====

template<int NumBiquads>
struct AAFilter {
    std::array<FilterUtils::BiquadLowpass_TDF2, NumBiquads> filters;
    std::array<FilterUtils::BiquadCoefficients, NumBiquads> coeffs;
    
    AAFilter() = default;
    
    // Calculate Q values for Butterworth filter of given order
    static void calculateButterQs(std::array<float, NumBiquads>& Qs, int order) {
        const int lim = order / 2;

        for (int k = 1; k <= lim; ++k) {
            float angle = (2.0f * static_cast<float>(k) + static_cast<float>(order) - 1.0f) * Utils::PI / (2.0f * static_cast<float>(order));
            float b = -2.0f * std::cos(angle);
            Qs[lim - k] = 1.0f / b;
        }
    }
    
    // Reset filter state and calculate coefficients for osRate
    void reset(float sampleRate, int osRatio) {
        std::array<float, NumBiquads> Qs;
        calculateButterQs(Qs, 2 * NumBiquads);
        
        float nyquist = sampleRate * 0.49f;
        float osRate = sampleRate * static_cast<float>(osRatio);
        
        for (int i = 0; i < NumBiquads; ++i) {
            coeffs[i] = FilterUtils::BiquadCoefficients::lowpass(nyquist, Qs[i], osRate);
        }
    }
    
    // Cascade Butterworth lowpass filters
    inline float process(float input) {
        float processed = input;
        for (int i = 0; i < NumBiquads; ++i) {
            processed = filters[i].process(processed, coeffs[i]);
        }
        return processed;
    }
};

// ===== BASE OVERSAMPLING =====

template<int Ratio, int NumBiquads = 4>
struct Oversampling {
    AAFilter<NumBiquads> aaFilter;
    AAFilter<NumBiquads> aiFilter;
    std::array<float, Ratio> osBuffer;
    
    Oversampling() = default;
    
    void reset(float sampleRate) {
        aaFilter.reset(sampleRate, Ratio);
        aiFilter.reset(sampleRate, Ratio);
        std::fill(osBuffer.begin(), osBuffer.end(), 0.0f);
    }
    
    inline void upsample(float x) {
        osBuffer[0] = static_cast<float>(Ratio) * x;
        std::fill(osBuffer.begin() + 1, osBuffer.end(), 0.0f);
        
        for (int k = 0; k < Ratio; ++k) {
            osBuffer[k] = aiFilter.process(osBuffer[k]);
        }
    }
    
    inline float downsample() {
        float y = 0.0f;
        for (int k = 0; k < Ratio; ++k) {
            y = aaFilter.process(osBuffer[k]);
        }
        return y;
    }
    
    inline float* getOSBuffer() {
        return osBuffer.data();
    }
};

// ===== VARIABLE OVERSAMPLING =====

template<int NumBiquads = 4>
struct VariableOversampling {
    Oversampling<1, NumBiquads> os0;
    Oversampling<2, NumBiquads> os1;
    Oversampling<4, NumBiquads> os2;
    Oversampling<8, NumBiquads> os3; 
    Oversampling<16, NumBiquads> os4;
    
    int osIdx{0};
    
    VariableOversampling() = default;
    
    void reset(float sampleRate) {
        os0.reset(sampleRate);
        os1.reset(sampleRate);
        os2.reset(sampleRate);
        os3.reset(sampleRate);
        os4.reset(sampleRate);
    }
    
    void setOversamplingIndex(int newIdx) { 
        osIdx = sc_clip(newIdx, 0, 4);
    }
    
    int getOversamplingIndex() const { 
        return osIdx; 
    }
    
    int getOversamplingRatio() const { 
        return 1 << osIdx; 
    }
    
    inline void upsample(float x) {
        switch (osIdx) {
            case 0: os0.upsample(x); break;
            case 1: os1.upsample(x); break;
            case 2: os2.upsample(x); break;
            case 3: os3.upsample(x); break;
            case 4: os4.upsample(x); break;
        }
    }
    
    inline float downsample() {
        switch (osIdx) {
            case 0: return os0.downsample();
            case 1: return os1.downsample();
            case 2: return os2.downsample();
            case 3: return os3.downsample();
            case 4: return os4.downsample();
            default: return 0.0f;
        }
    }
    
    inline float* getOSBuffer() {
        switch (osIdx) {
            case 0: return os0.getOSBuffer();
            case 1: return os1.getOSBuffer();
            case 2: return os2.getOSBuffer();
            case 3: return os3.getOSBuffer();
            case 4: return os4.getOSBuffer();
            default: return nullptr;
        }
    }
};

} // namespace OversamplingUtils