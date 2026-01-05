#pragma once
#include "SC_PlugIn.hpp"
#include "Utils.hpp"
#include <array>
#include <algorithm>

namespace DemandUtils {

    // ===== DEMAND URN =====
    
    template<int MaxSize>
    struct Durn {
        
        std::array<int, MaxSize> m_deck;
        int m_size{0};
        int m_position{0};
        int m_lastDrawn{-1};
        bool m_initialized{false};
        
        void initDeck(int size) {
            for (int i = 0; i < size; ++i) {
                m_deck[i] = i;
            }
            m_size = size;
            m_position = 0;
        }
        
        void shuffleDeck(RGen& rgen) {
            for (int i = m_size - 1; i > 0; --i) {
                int j = rgen.irand(i + 1);
                std::swap(m_deck[i], m_deck[j]);
            }
        }
        
        float process(float chance, int size, RGen& rgen) {
            
            // Rebuild deck if size changed
            if (size != m_size) {
                m_initialized = false;
            }
            
            // Initialize
            if (!m_initialized) {
                initDeck(size);
                shuffleDeck(rgen);
                m_initialized = true;
            }
            
            // Wrap position
            if (m_position >= m_size) {
                m_position = 0;
            }
            
            // Swap current card with random remaining card
            if (rgen.frand() < chance) {
                int remaining = m_size - m_position;
                int swapOffset = rgen.irand(remaining);
                std::swap(m_deck[m_position], m_deck[m_position + swapOffset]);
            }
            
            // Prevent repeats across cycle boundaries
            if (m_position == 0 && m_deck[0] == m_lastDrawn) {
                int remaining = m_size - 1;
                int swapOffset = rgen.irand(remaining);
                std::swap(m_deck[0], m_deck[1 + swapOffset]);
            }
            
            // Draw card and advance position
            m_lastDrawn = m_deck[m_position++];
            return static_cast<float>(m_lastDrawn);
        }
        
        void reset() {
            m_position = 0;
            m_lastDrawn = -1;
            m_initialized = false;
        }
    };
    
} // namespace DemandUtils