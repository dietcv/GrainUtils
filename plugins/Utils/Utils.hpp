#pragma once
#include "SC_PlugIn.hpp"
#include <array>

namespace Utils {

// ===== CONSTANTS =====

inline constexpr float SAFE_DENOM_EPSILON = 1e-10f;
inline constexpr float PI = 3.14159265358979323846f;
inline constexpr float TWO_PI = 6.28318530717958647692f;
inline constexpr float HALF_PI = 1.57079632679489661923f;

// ===== LINEAR PARAMETER INTERPOLATION =====

struct ParamInterp {
    double m_value{0.0};
    double m_slope{0.0};

    // Calculate interpolation slope toward new target value
    void update(float value, int numSteps) {
        const double delta = static_cast<double>(value) - m_value;
        m_slope = delta / static_cast<double>(numSteps);
    }

    // Advance interpolation by one step
    float process() {
        m_value += m_slope;
        return static_cast<float>(m_value);
    }

    void reset() {
        m_value = 0.0;
        m_slope = 0.0;
    }
};

// ===== CIRCULAR PARAMETER INTERPOLATION =====

struct PhaseInterp {
    double m_value{0.0};
    double m_slope{0.0};
 
    // Calculate interpolation slope toward new target phase
    void update(float value, int numSteps) {
        const double delta = sc_wrap(static_cast<double>(value) - m_value, -0.5, 0.5);
        m_slope = delta / static_cast<double>(numSteps);
    }
 
    // Advance interpolation by one step and wrap between 0 and 1
    float process() {
        m_value += m_slope;
        m_value = sc_frac(m_value);
        return static_cast<float>(m_value);
    }
 
    void reset() {
        m_value = 0.0;
        m_slope = 0.0;
    }
};

} // namespace Utils