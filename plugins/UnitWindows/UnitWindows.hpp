#pragma once
#include "SC_PlugIn.hpp"
#include "ShaperUtils.hpp"

// ===== HANNING WINDOW =====

class HanningWindow : public SCUnit {
public:
    HanningWindow();
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

// ===== GAUSSIAN WINDOW =====

class GaussianWindow : public SCUnit {
public:
    GaussianWindow();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew,
        Index
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== TRAPEZOIDAL WINDOW =====

class TrapezoidalWindow : public SCUnit {
public:
    TrapezoidalWindow();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew,
        Width,
        Duty
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== TUKEY WINDOW =====

class TukeyWindow : public SCUnit {
public:
    TukeyWindow();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew,
        Width
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};

// ===== EXPONENTIAL WINDOW =====

class ExponentialWindow : public SCUnit {
public:
    ExponentialWindow();
private:
    void next(int nSamples);
    
    // Input parameter indices
    enum Inputs {
        Phase,
        Skew,
        Shape
    };
    
    // Output indices
    enum Outputs {
        Out
    };
};