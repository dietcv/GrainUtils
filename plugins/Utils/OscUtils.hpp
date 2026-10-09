#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "BufferUtils.hpp"
#include "FilterUtils.hpp"
#include <array>
#include <algorithm>

namespace OscUtils {

// ===== SINC INTERPOLATION TABLE =====

struct SincTable {
    static constexpr int POINTS = 8;
    static constexpr int HALF_POINTS = POINTS / 2;
    static constexpr int SIZE = 8192;
    static constexpr int SPACING = SIZE / POINTS;
    static constexpr double ALPHA = 3.0;

    // Modified Bessel function I₀
    static inline double bessel_i0(double x) {
        double abs_x = sc_abs(x);
        
        if (abs_x < 3.75) {
            // Polynomial approximation for |x| < 3.75
            double t = (x / 3.75) * (x / 3.75);
            return 1.0 + 3.5156229 * t + 3.0899424 * (t * t) + 
                   1.2067492 * (t * t * t) + 0.2659732 * (t * t * t * t) + 
                   0.0360768 * (t * t * t * t * t) + 0.0045813 * (t * t * t * t * t * t);
        } else {
            // Asymptotic expansion for |x| >= 3.75
            double t = 3.75 / abs_x;
            double result = (std::exp(abs_x) / std::sqrt(abs_x)) *
                          (0.39894228 + 0.01328592 * t + 0.00225319 * (t * t) - 
                           0.00157565 * (t * t * t) + 0.00916281 * (t * t * t * t) - 
                           0.02057706 * (t * t * t * t * t) + 
                           0.02635537 * (t * t * t * t * t * t) - 
                           0.01647633 * (t * t * t * t * t * t * t) + 
                           0.00392377 * (t * t * t * t * t * t * t * t));
            return result;
        }
    }

    // Kaiser window
    static inline double kaiser(double x, double alpha) {
        double beta = alpha * Utils::PI;
        return bessel_i0(beta * std::sqrt(1.0 - x * x)) / bessel_i0(beta);
    }

    // Sinc function
    static inline double sincPi(double x, int ripples) {
        // Handle edge case with safe denom
        if (sc_abs(x) < Utils::SAFE_DENOM_EPSILON) {
            return 1.0;
        }

        double arg = x * ripples * Utils::PI;
        return std::sin(arg) / arg;
    }
        
    // Windowed sinc interpolation table
    static inline const std::array<float, SIZE> TABLE = []() {
        std::array<float, SIZE> result{};

        for (int i = 0; i < SIZE; ++i) {
            double x = (static_cast<double>(i) / (SIZE - 1)) * 2.0 - 1.0;
            
            double sinc = sincPi(x, HALF_POINTS);
            double window = kaiser(x, ALPHA);
            
            result[i] = static_cast<float>(sinc * window);
        }
        
        return result;
    }();
};

// ===== SINC INTERPOLATION =====

inline float sincInterp(float scaledPhase, const float* buffer, int startPos, int endPos, int sampleSpacing) {

    // Sinc-table data pointer
    const float* sincData = SincTable::TABLE.data();

    float sampleIndex = scaledPhase / static_cast<float>(sampleSpacing);
    int intPart = static_cast<int>(sampleIndex);
    float fracPart = sampleIndex - static_cast<float>(intPart);

    // Pre-calculate offsets
    float sincOffset = fracPart * SincTable::SPACING;
    int waveOffset = intPart * sampleSpacing;

    // Pre-calculate masks
    int sincMask = SincTable::SIZE - 1;
    int waveMask = (endPos - startPos) - 1;

    float result = 0.0f;
    for (int i = 0; i < SincTable::POINTS; ++i) {

        // === WAVEFORM BUFFER ACCESS (no interpolation) ===
        int waveIndex = (i - SincTable::HALF_POINTS) * sampleSpacing + waveOffset;
        float waveSample = BufferUtils::peekNoInterp(buffer, waveIndex, startPos, waveMask);
        
        // === SINC TABLE ACCESS (linear interpolation) ===
        float sincPos = static_cast<float>(i * SincTable::SPACING) - sincOffset;
        float sincSample = BufferUtils::peekLinearInterp(sincData, sincPos, sincMask);
        
        result += waveSample * sincSample;
    }
    
    return result;
}

// ===== MIPMAP INTERPOLATION =====

inline float mipmapInterp(float phase, float slope, const float* buffer, int startPos, int endPos) {
    
    // Scale phase to cycle range
    float rangeSize = static_cast<float>(endPos - startPos);
    float scaledPhase = phase * rangeSize;

    // Calculate mipmap parameters
    float samplesPerFrame = sc_abs(slope) * rangeSize;
    float octave = sc_max(0.0f, sc_log2(samplesPerFrame) + 1.0f);
    int layer = static_cast<int>(sc_floor(octave));
    
    // Calculate spacings for adjacent mipmap levels
    int spacing1 = 1 << layer;
    int spacing2 = spacing1 << 1;
    float crossfade = sc_frac(octave);

    // Early exit at maximum sinc spacing (no crossfade needed)
    if (spacing1 >= SincTable::SPACING) {
        return sincInterp(scaledPhase, buffer, startPos, endPos, SincTable::SPACING);
    }
    
    // Process each mipmap layer
    float sig1 = sincInterp(scaledPhase, buffer, startPos, endPos, spacing1);
    float sig2 = sincInterp(scaledPhase, buffer, startPos, endPos, spacing2);

    // Crossfade between the two mipmap layers
    return lininterp(crossfade, sig1, sig2);
}

// ===== MULTI-CYCLE WAVETABLE OSCILLATOR =====

inline float wavetableOsc(float phase, float slope, const BufferUtils::Wavetable::Output& table, float cyclePos) {

    // Scale cyclePos and calculate frac and int part
    float scaledPos = cyclePos * static_cast<float>(table.numCycles - 1);
    int intPart = static_cast<int>(scaledPos);
    float fracPart = scaledPos - static_cast<float>(intPart);
    
    // Calculate first cycle 
    int startPos1 = intPart * table.samplesPerCycle;
    int endPos1 = startPos1 + table.samplesPerCycle;
    
    // Early exit when positioned exactly on a single cycle (no crossfade needed)
    if (fracPart == 0.0f) {
        return mipmapInterp(phase, slope, table.data, startPos1, endPos1);
    }
    
    // Calculate second cycle only when needed
    int startPos2 = (intPart + 1) * table.samplesPerCycle;
    int endPos2 = startPos2 + table.samplesPerCycle;
    
    // Process each cycle
    float sig1 = mipmapInterp(phase, slope, table.data, startPos1, endPos1);
    float sig2 = mipmapInterp(phase, slope, table.data, startPos2, endPos2);
    
    // Crossfade between the two cycles
    return lininterp(fracPart, sig1, sig2);
}

// ===== MULTI-CYCLE WAVETABLE OSCILLATOR WITH CROSS-PHASE MODULATION =====

struct WavetableOscXPM {

    FilterUtils::TrackingLowpass m_oscPmFilter;
    FilterUtils::TrackingLowpass m_modPmFilter;

    float m_prevOsc{0.0f};
    float m_prevMod{0.0f};
    
    float process(
        float oscPhase, float modPhase,
        float oscSlope, float modSlope,
        float oscPmIndex, float modPmIndex,
        float oscPmDamping, float modPmDamping,
        float oscCyclePos, float modCyclePos,
        const BufferUtils::Wavetable::Output& oscTable,
        const BufferUtils::Wavetable::Output& modTable,
        float sampleRate
    ) {

        // Convert damping (0 - 1) to filter ratio (4 - 1)
        float oscPmFltRatio = std::exp2((1.0f - oscPmDamping) * 2.0f);
        float modPmFltRatio = std::exp2((1.0f - modPmDamping) * 2.0f);

        // Filter previous outputs with slope-tracking lowpass filters
        float modFiltered = m_modPmFilter.process(m_prevMod, modSlope * oscPmFltRatio, sampleRate);
        float oscFiltered = m_oscPmFilter.process(m_prevOsc, oscSlope * modPmFltRatio, sampleRate);
        
        // Apply cross-phase modulation and wrap between 0 and 1
        float modulatedOscPhase = sc_frac(oscPhase + (modFiltered / Utils::TWO_PI * oscPmIndex));
        float modulatedModPhase = sc_frac(modPhase + (oscFiltered / Utils::TWO_PI * modPmIndex));

        // Generate wavetable oscillators
        float osc = wavetableOsc(modulatedOscPhase, oscSlope, oscTable, oscCyclePos);
        float mod = wavetableOsc(modulatedModPhase, modSlope, modTable, modCyclePos);
        
        // Store current outputs for next sample
        m_prevOsc = osc;
        m_prevMod = mod;
        
        return osc;
    }

    void reset() {
        m_oscPmFilter.reset();
        m_modPmFilter.reset();
        m_prevOsc = 0.0f;
        m_prevMod = 0.0f;
    }   
};

} // namespace OscUtils