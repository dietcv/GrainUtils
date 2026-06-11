#pragma once
#include "SC_PlugIn.hpp"
#include <limits>
#include <cstring>

extern InterfaceTable* ft;

namespace PluginUtils {

// ===== BUFFER ALLOCATION =====

inline void allocBuffer(Unit* unit, World* world, int numSamples, float*& buffer) {
    // Allocate audio buffer
    buffer = (float*)RTAlloc(world, numSamples * sizeof(float));

    // Check the result of RTAlloc!
    ClearUnitIfMemFailed(buffer);
    
    // Initialize the allocated buffer with zeros
    memset(buffer, 0, numSamples * sizeof(float));
}

// ===== BUFFER MANAGEMENT =====

struct BufUnit {
    float m_fbufnum{std::numeric_limits<float>::quiet_NaN()};
    SndBuf* m_buf{nullptr};
    bool m_buf_failed{false};

    struct Output {
        bool valid;
        const float* data;
        int size;
    };

    BufUnit() = default;

    Output GetTable(Unit* unit, float fbufnum, const char* unitName) {
        if (fbufnum < 0.f) {
            fbufnum = 0.f;
        }
        
        if (fbufnum != m_fbufnum) {
            uint32 bufnum = static_cast<uint32>(fbufnum);
            World* world = unit->mWorld;
            if (bufnum >= world->mNumSndBufs) {
                uint32 localBufNum = bufnum - world->mNumSndBufs;
                Graph* parent = unit->mParent;
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

        if (!m_buf || !m_buf->data) {
            if (!m_buf_failed) {
                Print("%s: buffer not found\n", unitName);
                m_buf_failed = true;
            }
            return {false, nullptr, 0};
        }

        m_buf_failed = false;
        return {true, m_buf->data, m_buf->samples};
    }
};

} // namespace PluginUtils