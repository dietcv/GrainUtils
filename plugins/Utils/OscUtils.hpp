#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "FilterUtils.hpp"
#include <array>
#include <cmath>

namespace OscUtils {
    
// ===== BUFFER MANAGEMENT =====

struct BufUnit {
    float m_fbufnum{std::numeric_limits<float>::quiet_NaN()};
    SndBuf* m_buf{nullptr};
    bool m_buf_failed{false};
   
    BufUnit() {
        m_fbufnum = std::numeric_limits<float>::quiet_NaN();
        m_buf = nullptr;
        m_buf_failed = false;
    }
   
    bool GetTable(World* world, Graph* parent, float fbufnum, int inNumSamples, const SndBuf*& buf, const float*& bufData, int& tableSize) {
        if (fbufnum < 0.f) {                                                                      
            fbufnum = 0.f;                                                                                  
        }    
        if (fbufnum != m_fbufnum) {
            uint32 bufnum = static_cast<uint32>(fbufnum);
            if (bufnum >= world->mNumSndBufs) {
                uint32 localBufNum = bufnum - world->mNumSndBufs;
                if (localBufNum <= static_cast<uint32>(parent->localBufNum)) {
                    m_buf = parent->mLocalSndBufs + localBufNum;
                } else {
                    bufnum = 0;
                    m_buf = world->mSndBufs + bufnum;
                }
            } else {
                m_buf = world->mSndBufs + bufnum;
            }
            m_fbufnum = fbufnum;
        }
        buf = m_buf;
        if (!buf) {
            return false;
        }
        bufData = buf->data;
       
        if (!bufData) {
            return false;
        }
        tableSize = buf->samples;
        return true;
    }
};

// ===== SINC INTERPOLATION UTILITIES =====

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
        // Handle edge case when x is exactly 0
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

// ===== HIGH-PERFORMANCE SINC INTERPOLATION =====

inline float sincInterp(float scaledPhase, const float* buffer, int startPos, int endPos, int sampleSpacing) {

    // const pointer to sincTable data
    const float* const sincData = SincTable::TABLE.data();

    const float sampleIndex = scaledPhase / static_cast<float>(sampleSpacing);
    const int intPart = static_cast<int>(sampleIndex);
    const float fracPart = sampleIndex - static_cast<float>(intPart);

    // Pre-calculate offsets
    const float sincOffset = fracPart * SincTable::SPACING;
    const int waveOffset = intPart * sampleSpacing;

    // Pre-calculate masks
    const int sincMask = SincTable::SIZE - 1;
    const int waveMask = (endPos - startPos) - 1;

    float result = 0.0f;
    for (int i = 0; i < SincTable::POINTS; ++i) {

        // === WAVEFORM BUFFER ACCESS (no interpolation) ===
        const int waveIndex = (i - SincTable::HALF_POINTS) * sampleSpacing + waveOffset;
        const float waveSample = Utils::peekNoInterp(buffer, waveIndex, startPos, waveMask);
        
        // === SINC TABLE ACCESS (linear interpolation) ===
        const float sincPos = static_cast<float>(i * SincTable::SPACING) - sincOffset;
        const float sincSample = Utils::peekLinearInterp(sincData, sincPos, sincMask);
        
        result += waveSample * sincSample;
    }
    
    return result;
}

// ===== MIPMAP UTILITIES =====

inline float mipmapInterp(float phase, const float* buffer, int startPos, int endPos, 
                               int spacing1, int spacing2, float crossfade) {
    
    // Scale phase to cycle range
    const float rangeSize = static_cast<float>(endPos - startPos);
    const float scaledPhase = phase * rangeSize;
    
    // Check for sinc kernel bandwidth limit (1024)
    if (spacing1 >= SincTable::SPACING) {
        // no crossfade to next mipmap layer
        return sincInterp(scaledPhase, buffer, startPos, endPos, SincTable::SPACING);
    } else {
        // Crossfade between adjacent mipmap layers
        const float sig1 = sincInterp(scaledPhase, buffer, startPos, endPos, spacing1);
        const float sig2 = sincInterp(scaledPhase, buffer, startPos, endPos, spacing2);
        return lininterp(crossfade, sig1, sig2);
    }
}

// ===== MULTI-CYCLE WAVETABLE UTILITIES =====

inline float wavetableOsc(float phase, const float* buffer, int cycleSamples, int numCycles, float cyclePos, 
                                  int spacing1, int spacing2, float crossfade) {

    // Scale cyclePos and calculate frac and int part
    const float scaledPos = cyclePos * static_cast<float>(numCycles - 1);
    const int intPart = static_cast<int>(scaledPos);
    const float fracPart = scaledPos - static_cast<float>(intPart);
    
    // intPart ∈ [0, numCycles-1], no wrapping needed already guaranteed < numCycles
    const int cycleIndex1 = intPart;
    const int startPos1 = cycleIndex1 * cycleSamples;
    const int endPos1 = startPos1 + cycleSamples;
    
    // Early exit for fracPart == 0 (no crossfade needed)
    if (fracPart == 0.0f) {
        return mipmapInterp(phase, buffer, startPos1, endPos1, spacing1, spacing2, crossfade);
    }
    
    // Calculate second cycle only when needed
    const int cycleIndex2 = (intPart + 1) % numCycles;
    const int startPos2 = cycleIndex2 * cycleSamples;
    const int endPos2 = startPos2 + cycleSamples;
    
    // Process each cycle
    float sig1 = mipmapInterp(phase, buffer, startPos1, endPos1, spacing1, spacing2, crossfade);
    float sig2 = mipmapInterp(phase, buffer, startPos2, endPos2, spacing1, spacing2, crossfade);
    
    // Crossfade between the two cycles
    return lininterp(fracPart, sig1, sig2);
}

// ===== DUAL OSCILLATOR WITH CROSS-MODULATION =====

struct DualOsc {

    FilterUtils::OnePoleSlope m_pmFilterA;
    FilterUtils::OnePoleSlope m_pmFilterB;

    float m_prevOscA{0.0f};
    float m_prevOscB{0.0f};
    
    struct Output {
        float oscA;
        float oscB;
    };
    
    Output process(
        float phaseA, float phaseB,
        float cyclePosA, float cyclePosB,
        float slopeA, float slopeB,
        float pmIndexA, float pmIndexB,
        float pmFilterRatioA, float pmFilterRatioB,
        int spacing1A, int spacing2A, float crossfadeA,
        int spacing1B, int spacing2B, float crossfadeB,
        const float* bufferA, int cycleSamplesA, int numCyclesA,
        const float* bufferB, int cycleSamplesB, int numCyclesB
    ) {

        // Generate phase modulation signals using previous sample outputs
        float pmSignalA = m_prevOscB / Utils::TWO_PI;
        float pmSignalB = m_prevOscA / Utils::TWO_PI;
        
        // Filter the phase modulation signals
        float filteredPmA = m_pmFilterA.processLowpass(pmSignalA, slopeA * pmFilterRatioA);
        float filteredPmB = m_pmFilterB.processLowpass(pmSignalB, slopeB * pmFilterRatioB);
        
        // Apply phase modulation and wrap between 0 and 1
        float modulatedPhaseA = sc_frac(phaseA + (filteredPmA * pmIndexA));
        float modulatedPhaseB = sc_frac(phaseB + (filteredPmB * pmIndexB));

        // Generate oscillator outputs
        float oscA = wavetableOsc(modulatedPhaseA, bufferA, cycleSamplesA, numCyclesA, cyclePosA, 
                                         spacing1A, spacing2A, crossfadeA);
        float oscB = wavetableOsc(modulatedPhaseB, bufferB, cycleSamplesB, numCyclesB, cyclePosB, 
                                         spacing1B, spacing2B, crossfadeB);
        
        // Store current outputs for next sample
        m_prevOscA = oscA;
        m_prevOscB = oscB;
        
        return {oscA, oscB};
    }

    void reset() {
        m_pmFilterA.reset();
        m_pmFilterB.reset();
        m_prevOscA = 0.0f;
        m_prevOscB = 0.0f;
    }   
};

} // namespace OscUtils