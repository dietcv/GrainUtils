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

    // Calculate filter coefficients
    void calculate(float sampleRate, int osRatio) {

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

// ===== VARIABLE OVERSAMPLING =====

struct VariableOversampling {
    AAFilter<4> aaFilter;
    AAFilter<4> aiFilter;
    
    float* m_osBuffer{nullptr};
    int m_osRatio{1};
 
    VariableOversampling() = default;
 
    // Initialize oversampling filters
    void init(int osRatio, float sampleRate, float* osBuffer) {
        m_osRatio = osRatio;
        m_osBuffer = osBuffer;
        aaFilter.calculate(sampleRate, osRatio);
        aiFilter.calculate(sampleRate, osRatio);
    }
 
    // Process anti-imaging filter
    inline void upsample(float x) {
        m_osBuffer[0] = static_cast<float>(m_osRatio) * x;
        memset(m_osBuffer + 1, 0, (m_osRatio - 1) * sizeof(float));
 
        for (int k = 0; k < m_osRatio; ++k) {
            m_osBuffer[k] = aiFilter.process(m_osBuffer[k]);
        }
    }
 
    // Process anti-aliasing filter
    inline float downsample() {
        float y = 0.0f;
        for (int k = 0; k < m_osRatio; ++k) {
            y = aaFilter.process(m_osBuffer[k]);
        }
        return y;
    }
};

} // namespace OversamplingUtils