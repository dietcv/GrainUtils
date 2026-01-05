#pragma once
#include "SC_PlugIn.hpp"
#include "DemandUtils.hpp"

// ===== DEMAND URN =====

class Durn : public SCUnit {
public:
    Durn();
    
private:
    void next(int nSamples);
    
    static constexpr int MAX_DECK_SIZE = 32;
    
    DemandUtils::Durn<MAX_DECK_SIZE> m_urn;
    
    double m_repeats{-1.0};
    int32 m_repeatCount{0};
    
    float m_chance{0.0f};
    int m_size{8};

    enum Inputs {
        Chance,
        Size,
        Length,
    };
    
    enum Outputs {
        Out
    };
};