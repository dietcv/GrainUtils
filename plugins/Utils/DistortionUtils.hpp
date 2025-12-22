#pragma once
#include "SC_PlugIn.hpp"
#include <array>

namespace DistortionUtils {

// ===== FIRST-ORDER ANTIDERIVATIVE ANTI-ALIASING =====

template<typename FuncF0, typename FuncF1>
struct ADAA1 {
    
    // State variables
    double m_x1{0.0};
    double m_ad1_x1{0.0};
    
    // Waveshaper function pointers
    FuncF0 nlFunc;
    FuncF1 nlFunc_AD1;
    
    // Ill-conditioning tolerance
    static constexpr double TOL = 1e-2;
    
    ADAA1(FuncF0 f0, FuncF1 f1) 
        : nlFunc(f0), nlFunc_AD1(f1) 
    {}
    
    inline double process(double x) noexcept {
        double delta = x - m_x1;
        bool illCondition = std::abs(delta) < TOL;
        
        double y;
        if (illCondition) {
            // Fallback: evaluate at midpoint when delta is too small
            y = nlFunc(0.5 * (x + m_x1));
        } else {
            // Standard divided difference: (F1(x) - F1(x1)) / (x - x1)
            double ad1_x = nlFunc_AD1(x);
            y = (ad1_x - m_ad1_x1) / delta;
        }
        
        // Update state
        m_x1 = x;
        m_ad1_x1 = nlFunc_AD1(x);
        
        return y;
    }
    
    void reset() {
        m_x1 = 0.0;
        m_ad1_x1 = 0.0;
    }
};    

// ===== BUCHLA 259 WAVEFOLDER CELL =====

struct BuchlaCell {
    double gain, bias, thresh, mix;
    double Bp;
    
    BuchlaCell(double g, double b, double t, double m)
        : gain(g), bias(b), thresh(t), mix(m),
          Bp(0.5 * g * t * t - b * t)
    {}
    
    // Cell transfer function
    inline double func(double x) const noexcept {
        if (std::abs(x) > thresh) {
            return gain * x - bias * sc_sign(x);
        }
        return 0.0;
    }
    
    // First antiderivative of cell function
    inline double AD1(double x) const noexcept {
        if (std::abs(x) > thresh) {
            return 0.5 * gain * x * x - bias * x * sc_sign(x) - Bp;
        }
        return 0.0;
    }
};

// ===== BUCHLA 259 WAVEFOLDER WITH ADAA =====

struct BuchlaFoldADAA {
    
    // Circuit constants
    static constexpr double X_MIX = 5.0;
    static constexpr double IN_GAIN = 0.6;
    static constexpr double OUT_GAIN = 1.6666666666666667;
    
    // Parallel folding cells (gain, bias, threshold, mix)
    inline static const std::array<BuchlaCell, 5> CELLS{{
        {0.8333,  0.5,    0.6,    -12.0},
        {0.3768,  1.1281, 2.994,  -27.777},
        {0.2829,  1.5446, 5.46,   -21.428},
        {0.5743,  1.0338, 1.8,    17.647},
        {0.2673,  1.0907, 4.08,   36.363}
    }};
    
    // Transfer function: F(x) = 5x + Σ(mix * cell(x))
    static inline double nlFunc(double x) {
        double y = X_MIX * x;
        for (const auto& cell : CELLS) {
            y += cell.mix * cell.func(x);
        }
        return y;
    }
    
    // First antiderivative: F1(x) = 2.5x² + Σ(mix * cell_AD1(x))
    static inline double nlFunc_AD1(double x) {
        double y = 0.5 * X_MIX * x * x;
        for (const auto& cell : CELLS) {
            y += cell.mix * cell.AD1(x);
        }
        return y;
    }
    
    // First-Order ADAA processor
    ADAA1<decltype(&nlFunc), decltype(&nlFunc_AD1)> adaa{
        &nlFunc, 
        &nlFunc_AD1
    };
    
    BuchlaFoldADAA() = default;
    
    // Main processing function
    inline float process(float input, float drive) noexcept {
        double x = input * (drive + 1.0) * IN_GAIN;
        double y = adaa.process(x);
        return static_cast<float>(y / X_MIX * OUT_GAIN);
    }
    
    void reset() {
        adaa.reset();
    }
};

} // namespace DistortionUtils