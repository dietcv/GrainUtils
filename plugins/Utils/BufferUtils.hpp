#pragma once
#include "SC_PlugIn.hpp"
#include <limits>

extern InterfaceTable* ft;

namespace BufferUtils {

// ===== BUFFER ACCESS =====

// Fast no-interpolation peek with bitwise wrapping and optional offset - (for power-of-2 sizes)
inline float peekNoInterp(const float* buffer, int index, int startPos, int mask) {
    const int wrappedIndex = startPos + (index & mask);
    return buffer[wrappedIndex];
}

// Fast linear interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekLinearInterp(const float* buffer, float phase, int mask) {

    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    
    const float a = buffer[idx1];
    const float b = buffer[idx2];
    
    return lininterp(fracPart, a, b);
}

// Fast cubic interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekCubicInterp(const float* buffer, float phase, int mask) {
    
    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx0 = (intPart - 1) & mask;
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    const int idx3 = (intPart + 2) & mask;
    
    const float a = buffer[idx0];
    const float b = buffer[idx1];
    const float c = buffer[idx2];
    const float d = buffer[idx3];
    
    return cubicinterp(fracPart, a, b, c, d);
}

// ===== BUFFER ALLOCATION =====

inline void allocBuffer(Unit* unit, World* world, int numSamples, float*& buffer) {
    // Allocate audio buffer
    buffer = (float*)RTAlloc(world, numSamples * sizeof(float));

    // Check the result of RTAlloc!
    ClearUnitIfMemFailed(buffer);
    
    // Initialize the allocated buffer with zeros
    memset(buffer, 0, numSamples * sizeof(float));
}

// ===== BUFFER LOOKUP =====

struct BufUnit {
    float m_bufNum{std::numeric_limits<float>::quiet_NaN()};
    SndBuf* m_buf{nullptr};

    const SndBuf* update(Unit* unit, World* world, int nSamples, float bufNum) {

        // Look up buffer
        if (bufNum != m_bufNum) {
            uint32 index = static_cast<uint32>(bufNum);
            if (index >= world->mNumSndBufs) {
                uint32 localIndex = index - world->mNumSndBufs;
                Graph* parent = unit->mParent;
                if (localIndex < static_cast<uint32>(parent->localBufNum)) {
                    m_buf = parent->mLocalSndBufs + localIndex;
                } else {
                    m_buf = world->mSndBufs;
                }
            } else {
                m_buf = world->mSndBufs + index;
            }
            m_bufNum = bufNum;
        }

        // Check for allocated buffer
        if (!m_buf->data) {
            ClearUnitOutputs(unit, nSamples);
            return nullptr;
        }

        return m_buf;
    }
};

// ===== WAVETABLE LOOKUP =====

struct Wavetable {
    BufUnit m_bufUnit;
    bool m_bufFailed{false};

    struct Output {
        const float* data;
        int samplesPerCycle;
        int numCycles;
    };

    Output update(Unit* unit, World* world, int nSamples, float bufNum, int numCycles, const char* unitName) {

        // Look up buffer
        const SndBuf* buf = m_bufUnit.update(unit, world, nSamples, bufNum);

        // Check for allocated buffer
        if (!buf) {
            return {nullptr, 0, numCycles};
        }

        // Check if numCycles fits into buffer
        if (numCycles > buf->samples) {
            if (!m_bufFailed && world->mVerbosity >= -1) {
                Print("Warning: numCycles exceeds buffer size (%s)\n", unitName);
            }
            m_bufFailed = true;
            ClearUnitOutputs(unit, nSamples);
            return {nullptr, 0, numCycles};
        }

        // Calculate samples per single cycle
        int samplesPerCycle = buf->samples / numCycles;

        // Check for power-of-two cycles
        if (!ISPOWEROFTWO(samplesPerCycle)) {
            if (!m_bufFailed && world->mVerbosity >= -1) {
                Print("Warning: samples per cycle not a power of two (%s)\n", unitName);
            }
            m_bufFailed = true;
            ClearUnitOutputs(unit, nSamples);
            return {nullptr, 0, numCycles};
        }

        m_bufFailed = false;
        return {buf->data, samplesPerCycle, numCycles};
    }
};

} // namespace BufferUtils