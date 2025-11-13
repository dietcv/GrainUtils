#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include "FilterUtils.hpp"
#include "wavetables.h"
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
    static constexpr int SIZE = 8192;
    static constexpr int COUNT = 8;
    static constexpr int SPACING = SIZE / COUNT;
    
    std::array<float, SIZE> table;
    std::array<int, COUNT> sinc_points{0, 1024, 2048, 3072, 4096, 5120, 6144, 7168};
    std::array<int, COUNT> wave_points{-4, -3, -2, -1, 0, 1, 2, 3};

    SincTable() {
        // Load table and convert in constructor
        auto doubleTable = get_sinc_window8();
        for (int i = 0; i < SIZE; ++i) {
            table[i] = static_cast<float>(doubleTable[i]);
        }
    }
    
    // table access
    const float* data() const { return table.data(); }
};

// ===== HIGH-PERFORMANCE SINC INTERPOLATION =====

inline float sincInterpolate(float scaledPhase, const float* buffer, int bufSize, int startPos, int endPos, int sampleSpacing, const SincTable& sincTable) {

    // const pointer to sincTable data
    const float* const sincData = sincTable.data();

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
    
    for (int i = 0; i < SincTable::COUNT; ++i) {

        // === WAVEFORM BUFFER ACCESS (no interpolation) ===
        const int waveIndex = sincTable.wave_points[i] * sampleSpacing + waveOffset;
        const float waveSample = Utils::peekNoInterp(buffer, waveIndex, startPos, waveMask);
        
        // === SINC TABLE ACCESS (linear interpolation) ===
        const float sincPos = static_cast<float>(sincTable.sinc_points[i]) - sincOffset;
        const float sincSample = Utils::peekLinearInterp(sincData, sincPos, sincMask);
        
        result += waveSample * sincSample;
    }
    
    return result;
}

// ===== MIPMAP UTILITIES =====

inline float mipmapInterpolate(float phase, const float* buffer, int bufSize, int startPos, int endPos, float slope, const SincTable& sincTable) {
    
    // Calculate mipmap parameters
    const float rangeSize = static_cast<float>(endPos - startPos);
    const float scaledPhase = phase * rangeSize;
    const float samplesPerFrame = std::abs(slope) * rangeSize;
    const float octave = std::max(0.0f, sc_log2(samplesPerFrame));
    const int layer = static_cast<int>(sc_ceil(octave)); 
    
    // Calculate spacings for adjacent mipmap levels  
    const int spacing1 = 1 << layer;     
    const int spacing2 = spacing1 << 1;
    
    // Check for sinc kernel bandwidth limit (1024)
    if (spacing1 >= SincTable::SPACING) {
        // no crossfade to next mipmap layer
        return sincInterpolate(scaledPhase, buffer, bufSize, startPos, endPos, SincTable::SPACING, sincTable);
    } else {
        // Crossfade between adjacent mipmap layers
        const float sig1 = sincInterpolate(scaledPhase, buffer, bufSize, startPos, endPos, spacing1, sincTable);
        const float sig2 = sincInterpolate(scaledPhase, buffer, bufSize, startPos, endPos, spacing2, sincTable);
        return Utils::lerp(sig1, sig2, sc_frac(octave));
    }
}

// ===== MULTI-CYCLE WAVETABLE UTILITIES =====

inline float wavetableInterpolate(float phase, const float* buffer, int bufSize, int cycleSamples, int numCycles, float cyclePos, float slope, const SincTable& sincTable) {

    // clip cyclePos to 0-1, then scale by (numCycles - 1)
    const float clippedPos = sc_clip(cyclePos, 0.0f, 1.0f);
    const float scaledPos = clippedPos * static_cast<float>(numCycles - 1);

    // Calculate frac and int part
    const int intPart = static_cast<int>(scaledPos);
    const float fracPart = scaledPos - static_cast<float>(intPart);
    
    // intPart ∈ [0, numCycles-1], no wrapping needed already guaranteed < numCycles
    const int cycleIndex1 = intPart;
    const int startPos1 = cycleIndex1 * cycleSamples;
    const int endPos1 = startPos1 + cycleSamples;
    
    // Early exit for fracPart == 0 (no crossfade needed)
    if (fracPart == 0.0f) {
        return mipmapInterpolate(phase, buffer, bufSize, startPos1, endPos1, slope, sincTable);
    }
    
    // Calculate second cycle only when needed
    const int cycleIndex2 = (intPart + 1) % numCycles;
    const int startPos2 = cycleIndex2 * cycleSamples;
    const int endPos2 = startPos2 + cycleSamples;
    
    // Process each cycle
    float sig1 = mipmapInterpolate(phase, buffer, bufSize, startPos1, endPos1, slope, sincTable);
    float sig2 = mipmapInterpolate(phase, buffer, bufSize, startPos2, endPos2, slope, sincTable);
    
    // Crossfade between the two cycles
    return Utils::lerp(sig1, sig2, fracPart);
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
        const float* bufferA, int bufSizeA, int cycleSamplesA, int numCyclesA,
        const float* bufferB, int bufSizeB, int cycleSamplesB, int numCyclesB,
        const SincTable& sincTable
    ) {

        // Generate phase modulation signals using previous sample outputs
        float pmSignalA = (m_prevOscB * Utils::TWO_PI_INV) * pmIndexA;
        float pmSignalB = (m_prevOscA * Utils::TWO_PI_INV) * pmIndexB;
        
        // Filter the phase modulation signals
        float filteredPmA = m_pmFilterA.processLowpass(pmSignalA, slopeA * pmFilterRatioA);
        float filteredPmB = m_pmFilterB.processLowpass(pmSignalB, slopeB * pmFilterRatioB);
        
        // Apply phase modulation and wrap between 0 and 1
        float modulatedPhaseA = sc_frac(phaseA + filteredPmA);
        float modulatedPhaseB = sc_frac(phaseB + filteredPmB);
        
        // Generate oscillator outputs
        float oscA = wavetableInterpolate(modulatedPhaseA, bufferA, bufSizeA, 
                                         cycleSamplesA, numCyclesA, cyclePosA, slopeA, sincTable);
        float oscB = wavetableInterpolate(modulatedPhaseB, bufferB, bufSizeB, 
                                         cycleSamplesB, numCyclesB, cyclePosB, slopeB, sincTable);
        
        // Store current outputs for next sample
        m_prevOscA = oscA;
        m_prevOscB = oscB;
        
        return {oscA, oscB};
    }
};

} // namespace OscUtils