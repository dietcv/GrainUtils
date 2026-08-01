#pragma once
#include "SC_PlugIn.hpp"
#include <array>

namespace DistortionUtils {

// ===== FIRST-ORDER ANTIDERIVATIVE ANTI-ALIASING =====

template<auto NlFunc, auto NlFuncAD1>
struct ADAA1 {
    
    // State variables
    double m_x1{0.0};
    double m_ad1_x1{0.0};
    
    // Ill-conditioning tolerance
    static constexpr double TOL = 1e-2;
    
    inline double process(double x) noexcept {
        double delta = x - m_x1;
        bool illCondition = std::abs(delta) < TOL;
        
        double y;
        if (illCondition) {
            // Fallback: evaluate at midpoint when delta is too small
            y = NlFunc(0.5 * (x + m_x1));
        } else {
            // Standard divided difference: (F1(x) - F1(x1)) / (x - x1)
            double ad1_x = NlFuncAD1(x);
            y = (ad1_x - m_ad1_x1) / delta;
        }
        
        // Update state
        m_x1 = x;
        m_ad1_x1 = NlFuncAD1(x);
        
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
    
    constexpr BuchlaCell(double g, double b, double t, double m)
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

struct BuchlaFold {
    
    // Circuit constants
    static constexpr int NUM_CELLS = 5;

    // Parallel folding cells (gain, bias, threshold, mix)
    static constexpr std::array<BuchlaCell, NUM_CELLS> CELLS{{
        {0.8333,  0.5,    0.6,    -12.0},
        {0.3768,  1.1281, 2.994,  -27.777},
        {0.2829,  1.5446, 5.46,   -21.428},
        {0.5743,  1.0338, 1.8,    17.647},
        {0.2673,  1.0907, 4.08,   36.363}
    }};

    // Small-signal slope f'(0)
    static constexpr double SLOPE = 5.0;
    // Peak output: the largest |nlFunc(x)| the folder produces over [0, MAX_GAIN]
    static constexpr double FOLD_PEAK = 0.6784012128;
    // Escape gain: the largest g with |nlFunc(g)| <= FOLD_PEAK
    static constexpr double MAX_GAIN = 9.513465730;
    // Gain floor: the smallest cell threshold
    static constexpr double MIN_GAIN = 0.6;
    
    // Transfer function: F(x) = [SLOPE*x + Σ(mix * cell(x))] / SLOPE
    static inline double nlFunc(double x) {
        double y = SLOPE * x;   // direct signal path
        for (const auto& cell : CELLS) {
            y += cell.mix * cell.func(x);
        }
        return y * (1.0 / SLOPE);
    }
    
    // First antiderivative: F1(x) = [SLOPE*x²/2 + Σ(mix * cell_AD1(x))] / SLOPE
    static inline double nlFunc_AD1(double x) {
        double y = 0.5 * SLOPE * x * x;
        for (const auto& cell : CELLS) {
            y += cell.mix * cell.AD1(x);
        }
        return y * (1.0 / SLOPE);
    }
    
    // First-Order ADAA processor
    ADAA1<&BuchlaFold::nlFunc, &BuchlaFold::nlFunc_AD1> adaa;
    
    BuchlaFold() = default;
    
    // Main processing function
    inline float process(float input, float drive) noexcept {

        // Convert drive (0 - 1) to input gain (MIN_GAIN - MAX_GAIN)
        double gain = MIN_GAIN + static_cast<double>(drive) * (MAX_GAIN - MIN_GAIN);

        // Process wavefolder
        double y = adaa.process(static_cast<double>(input) * gain);

        // Apply makeup gain
        return static_cast<float>(y / sc_min(gain, FOLD_PEAK));
    }
    
    void reset() {
        adaa.reset();
    }
};

// ===== SERGE WAVEFOLDER CELL =====

struct SergeCell {
    static constexpr double D = 2.45;
    static constexpr double B = 0.96;

    // Transfer function: F(x) = (2/D) * tanh(D*x) - B*x
    static inline double func(double x) noexcept {
        return (2.0 / D) * std::tanh(D * x) - B * x;
    }

    // First antiderivative: F1(x) = (2/D²) * log(cosh(D*x)) - B*x²/2
    static inline double AD1(double x) noexcept {
        return (2.0 / (D * D)) * std::log(std::cosh(D * x)) - 0.5 * B * x * x;
    }
};

// ===== SERGE WAVEFOLDER WITH ADAA =====

struct SergeFold {

    // Circuit constants
    static constexpr int NUM_STAGES = 6;
    // Small-signal slope f'(0)
    static constexpr double SLOPE = 1.04;
    // Peak output: nlFunc(x*) at the cell's fold point x*, where
    // nlFunc'(x*) = 0, i.e. x* = atanh(sqrt(1 - B/2)) / D = 0.3714098642
    static constexpr double FOLD_PEAK = 0.2231807338;
    // Escape gain: the largest g with |cascade(g)| <= FOLD_PEAK
    static constexpr double MAX_GAIN = 6.638878743;
    // Gain floor (MAX_GAIN * 0.001, -60 dB)
    static constexpr double MIN_GAIN = MAX_GAIN * 0.001;

    // Transfer function: F(x) = cell(x) / SLOPE
    static inline double nlFunc(double x) {
        return SergeCell::func(x) * (1.0 / SLOPE);
    }

    // First antiderivative: F1(x) = cell_AD1(x) / SLOPE
    static inline double nlFunc_AD1(double x) {
        return SergeCell::AD1(x) * (1.0 / SLOPE);
    }

    // First-Order ADAA processors
    std::array<ADAA1<&SergeFold::nlFunc, &SergeFold::nlFunc_AD1>, NUM_STAGES> adaa{};

    SergeFold() = default;

    // Main processing function
    inline float process(float input, float drive) noexcept {

        // Convert drive (0 - 1) to input gain (MIN_GAIN - MAX_GAIN)
        double gain = MIN_GAIN + static_cast<double>(drive) * (MAX_GAIN - MIN_GAIN);

        // Cascade wavefolder stages
        double x = static_cast<double>(input) * gain;
        for (int i = 0; i < NUM_STAGES; ++i) {
            x = adaa[i].process(x);
        }

        // Apply makeup gain
        return static_cast<float>(x / sc_min(gain, FOLD_PEAK));
    }

    void reset() {
        for (int i = 0; i < NUM_STAGES; ++i) {
            adaa[i].reset();
        }
    }
};

} // namespace DistortionUtils