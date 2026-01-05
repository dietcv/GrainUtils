#include "SC_PlugIn.hpp"
#include "Demand.hpp"

static InterfaceTable* ft;

// ===== DEMAND URN =====

Durn::Durn() {
    mCalcFunc = make_calc_function<Durn, &Durn::next>();
    next(0);
    out0(0) = 0.0f;
}

void Durn::next(int nSamples) {
    Unit* unit = this;
    
    if (nSamples) {
        RGen& rgen = *mParent->mRGen;

        // Initialize repeats on first call
        if (m_repeats < 0.0) {
            float x = DEMANDINPUT_A(Length, nSamples);
            m_repeats = sc_isnan(x) ? 0.0 : floor(x + 0.5);
        }
        
        // Check if exhausted
        if (m_repeatCount >= m_repeats) {
            out0(0) = NAN;
            return;
        }

        // Increment counter
        m_repeatCount++;

        // Get and cache parameters
        float chanceParam = DEMANDINPUT_A(Chance, nSamples);
        if (!sc_isnan(chanceParam)) {
            m_chance = chanceParam;
        }

        float sizeParam = DEMANDINPUT_A(Size, nSamples);
        if (!sc_isnan(sizeParam)) {
            m_size = static_cast<int>(sizeParam);
        }
    
        // Draw from urn
        float output = m_urn.process(m_chance, m_size, rgen);
        out0(0) = output;
    
    } else {
        m_repeats = -1.0;
        m_repeatCount = 0;
        m_urn.reset();
    }
}

PluginLoad(DemandUGens) {
    ft = inTable;
    registerUnit<Durn>(ft, "Durn", false);
}