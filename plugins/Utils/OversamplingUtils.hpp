#pragma once
#include "FilterUtils.hpp"
#include <array>

namespace OversamplingUtils {

// ===== PARAMETER INTERPOLATION =====

struct OSParamInterp {
    float m_lastValue{0.0f};
    float m_currentValue{0.0f};

    // Latch parameter value for oversampling
    void update(float value) {
        m_lastValue = m_currentValue;
        m_currentValue = value;
    }

    // Interpolate parameter value at fractional position
    float process(float frac) const {
        return lininterp(frac, m_lastValue, m_currentValue);
    }

    void reset(float value) {
        m_lastValue = value;
        m_currentValue = value;
    }
};

// ===== POLYPHASE HALFBAND FILTER =====

struct PolyphaseHalfband {
    static constexpr int NUM_SECTIONS = 2;

    // Allpass coefficients per branch
    static constexpr float COEFFS_A[NUM_SECTIONS] = {0.079866426202558f, 0.545323651182583f};
    static constexpr float COEFFS_B[NUM_SECTIONS] = {0.283829344898100f, 0.834411891201724f};

    std::array<FilterUtils::AllpassOne, NUM_SECTIONS> branchA;
    std::array<FilterUtils::AllpassOne, NUM_SECTIONS> branchB;

    PolyphaseHalfband() = default;

    // Decimate two samples into one
    inline float downsample(float x0, float x1) {

        float a = x1;
        float b = x0;

        for (int i = 0; i < NUM_SECTIONS; ++i) {
            a = branchA[i].process(a, COEFFS_A[i]);
            b = branchB[i].process(b, COEFFS_B[i]);
        }

        return 0.5f * (a + b);
    }

    // Interpolate one sample into two
    inline void upsample(float x, float& y0, float& y1) {

        float a = x;
        float b = x;

        for (int i = 0; i < NUM_SECTIONS; ++i) {
            a = branchA[i].process(a, COEFFS_A[i]);
            b = branchB[i].process(b, COEFFS_B[i]);
        }

        y0 = a;
        y1 = b;
    }

    void reset() {
        for (int i = 0; i < NUM_SECTIONS; ++i) {
            branchA[i].reset();
            branchB[i].reset();
        }
    }
};

// ===== VARIABLE OVERSAMPLING =====

struct VariableOversampling {
    static constexpr int MAX_STAGES = 4;

    std::array<PolyphaseHalfband, MAX_STAGES> aiStages;
    std::array<PolyphaseHalfband, MAX_STAGES> aaStages;

    VariableOversampling() = default;

    inline void upsample(float x, float* osBuffer, int osRatio) {
        osBuffer[0] = x;

        for (int stage = 0; (1 << stage) < osRatio; ++stage) {
            int count = 1 << stage;
            for (int i = count - 1; i >= 0; --i) {
                aiStages[stage].upsample(osBuffer[i], osBuffer[2 * i], osBuffer[2 * i + 1]);
            }
        }
    }

    inline float downsample(float* osBuffer, int osRatio) {

        for (int stage = 0; (osRatio >> stage) > 1; ++stage) {
            int count = (osRatio >> stage) / 2;
            for (int i = 0; i < count; ++i) {
                osBuffer[i] = aaStages[stage].downsample(osBuffer[2 * i], osBuffer[2 * i + 1]);
            }
        }

        return osBuffer[0];
    }

    void reset() {
        for (int s = 0; s < MAX_STAGES; ++s) {
            aiStages[s].reset();
            aaStages[s].reset();
        }
    }
};

} // namespace OversamplingUtils