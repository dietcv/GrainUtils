#pragma once
#include "FilterUtils.hpp"
#include <array>
#include <algorithm>

namespace OversamplingUtils {

// ===== HIGH-ORDER BUTTERWORTH LOWPASS =====

template<int NumBiquads>
struct AAFilter {
    std::array<FilterUtils::BiquadFilter, NumBiquads> filters;
    std::array<FilterUtils::BiquadCoefficients, NumBiquads> coeffs;

    AAFilter() = default;

    // Calculate Q values for Butterworth filter of given order
    static std::array<float, NumBiquads> calculateButterQs(int order) {
        std::array<float, NumBiquads> Qs{};

        const int lim = order / 2;

        for (int k = 1; k <= lim; ++k) {
            float t = static_cast<float>(2 * k + order - 1);
            float alpha = -2.0f * std::cos(Utils::PI * t / (2.0f * static_cast<float>(order)));
            Qs[lim - k] = 1.0f / alpha;
        }

        return Qs;
    }

    // Update filter coefficients
    void update(float sampleRate, int osRatio) {

        // Calculate Q values
        auto Qs = calculateButterQs(2 * NumBiquads);

        const float nyquist = sampleRate * 0.49f;
        const float osRate  = sampleRate * static_cast<float>(osRatio);

        for (int i = 0; i < NumBiquads; ++i) {
            coeffs[i] = FilterUtils::BiquadCoefficients::lowpass(nyquist, Qs[i], osRate);
        }
    }

    // Process audio through high-order lowpass filter
    inline float process(float input) {

        // Cascade lowpass filters
        float processed = input;
        for (int i = 0; i < NumBiquads; ++i) {
            processed = filters[i].process(processed, coeffs[i]);
        }

        return processed;
    }

    void reset() {
        for (int i = 0; i < NumBiquads; ++i) {
            filters[i].reset();
        }
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
        aaFilter.update(sampleRate, Ratio);
        aiFilter.update(sampleRate, Ratio);
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