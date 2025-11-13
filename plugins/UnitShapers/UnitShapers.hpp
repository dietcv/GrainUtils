#pragma once
#include "SC_PlugIn.hpp"
#include "ShaperUtils.hpp"

// ===== UNIT TRIANGLE =====

class UnitTriangle : public SCUnit {
public:
    UnitTriangle();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== UNIT KINK =====

class UnitKink : public SCUnit {
public:
    UnitKink();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== UNIT CUBIC =====

class UnitCubic : public SCUnit {
public:
    UnitCubic();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Index
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};