#include "EventSystem.hpp"
#include "ShiftRegister.hpp"
#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== TRIGGER DETECTION UTILITIES =====

struct Change {
    double m_lastValue{0.0};
    
    double process(double currentValue) {
        double delta = currentValue - m_lastValue;
        m_lastValue = currentValue;
        
        // Return sign of change
        if (delta > 0.0) return 1.0;
        else if (delta < 0.0) return -1.0;
        else return 0.0;
    }
    
    void reset() {
        m_lastValue = 0.0;
    }
};

struct StepToTrig {
    Change changeDetect;
    
    bool process(double phaseScaled) {
        double phaseStepped = std::ceil(phaseScaled);
        double changed = changeDetect.process(phaseStepped);
        //Print("StepToTrig: phase=%f, ceil=%f, result=%s\n", 
              //phaseScaled, phaseStepped, (changed > 0.0) ? "TRUE" : "FALSE");
        return changed > 0.0;
    }
    
    void reset() {
        changeDetect.reset();
    }
};

struct RampToTrigSimple {
    double m_lastPhase{0.0};
    bool m_lastWrap{false};
    
    bool process(double currentPhase) {
        // Detect wrap using current vs last phase
        double delta = currentPhase - m_lastPhase;
        bool currentWrap = std::abs(delta) > 0.5;
        
        // Edge detection - only trigger on rising edge of wrap
        bool trigger = currentWrap && !m_lastWrap;
        
        //Print("RampToTrig: currentPhase=%f, lastPhase=%f, result=%s\n", 
              //currentPhase, m_lastPhase, trigger ? "TRUE" : "FALSE");
        
        // Update state for next sample
        m_lastPhase = currentPhase;
        m_lastWrap = currentWrap;
        
        return trigger;
    }
    
    void reset() {
        m_lastPhase = 0.0;
        m_lastWrap = false;
    }
};

struct RampToTrig {
    double m_lastPhase{0.0};
    bool m_lastWrap{false};
    
    bool process(double currentPhase) {
        // Detect wrap using current vs last phase
        double delta = currentPhase - m_lastPhase;
        double sum = currentPhase + m_lastPhase;
        bool currentWrap = (sum != 0.0) && (std::abs(delta / sum) > 0.5);
        
        // Edge detection - only trigger on rising edge of wrap
        bool trigger = currentWrap && !m_lastWrap;
        
        // Update state for next sample
        m_lastPhase = currentPhase;
        m_lastWrap = currentWrap;
        
        return trigger;
    }
    
    void reset() {
        m_lastPhase = 0.0;
        m_lastWrap = false;
    }
};

// ===== SCHEDULER BURST =====

struct SchedulerBurst {
    
    RampToTrigSimple trigDetect;
    //RampToTrig trigDetect;

    double m_phaseScaled{0.0};           
    double m_slope{0.0};           
    int m_cycleCount{0};           
    bool m_hasTriggered{false};    
    
    struct Output {
        bool trigger = false;
        float phase = 0.0f;
        float rate = 0.0f;
        float subSampleOffset = 0.0f;
    };
   
    Output process(bool initTrigger, float duration, float cycles, float sampleRate) {
        Output output;
        
        // Reset on new trigger
        
        if (initTrigger) {
            m_phaseScaled = 0.0;
            m_cycleCount = 0;
            m_hasTriggered = true;
            trigDetect.reset();  // Reset detector state
        }
        /*
        if (initTrigger) {
            //init();
            trigDetect.reset();
            m_cycleCount = 0;
            m_hasTriggered = true;
        }
        */
        // Skip processing until first trigger
        if (!m_hasTriggered && duration > 0.0f) {
            return output;
        }
        
        // Calculate slope
        if(duration > 0.0f) {
            m_slope = 1.0 / (duration * sampleRate);
        } else {
            m_slope = 1.0 / sampleRate;
        }

        // Wrap phase for trigger detection
        double phase = sc_wrap(m_phaseScaled, 0.0, 1.0);
        
        // Detect wrap trigger  
        bool derivedTrigger = trigDetect.process(phase);
 
        // Increment cycle count on wrap BEFORE gating decision
        if (derivedTrigger) {
            m_cycleCount++;
        }
        
        // Gate the triggers
        bool m_triggerLast = m_cycleCount < static_cast<int>(cycles);
        bool trigger = (initTrigger || derivedTrigger) && m_triggerLast;
        //bool trigger = derivedTrigger && m_triggerLast;
        //bool trigger = stepToTrig.process(m_phaseScaled);
        
        // Calculate subsample offset
        double subSampleOffset = 0.0;
        if (trigger && m_slope != 0.0) {
            subSampleOffset = phase / m_slope;
        }
        
        // Prepare output
        output.trigger = trigger;
        output.phase = static_cast<float>(phase);
        output.rate = static_cast<float>(m_slope * sampleRate);
        output.subSampleOffset = static_cast<float>(subSampleOffset);
        
        // Increment
        m_phaseScaled += m_slope;

        // clamp phase
        if (m_phaseScaled > cycles) {
            m_phaseScaled = cycles;
        }
        
        return output;
    }
    
    void reset() {
        m_phaseScaled = 0.0;
        m_slope = 0.0;
        m_cycleCount = 0;
        m_hasTriggered = false;
        trigDetect.reset();
    }

    void init() {
        m_phaseScaled = 0.0;
        m_slope = 0.0;
        //m_wrapNext = false;
        // Prime trigger detector to fire on next sample
        trigDetect.m_lastPhase = 1.0; // As if coming from wrap
        trigDetect.m_lastWrap = false; // Ready to detect next wrap
    }
};

// ===== SCHEDULER BURST UGEN =====
class SchedulerBurstUGen : public SCUnit {
public:
    SchedulerBurstUGen();
    ~SchedulerBurstUGen();
    
private:
    void next_aa(int nSamples);
    
    // Core processing
    SchedulerBurst m_scheduler;
    
    // Constants
    const float m_sampleRate;
    
    // Input parameters
    enum InputParams {
        InitTrigger,    // 0 - Trigger to start sequence
        Duration,       // 1 - Duration of one cycle
        Cycles          // 2 - Number of cycles/events to generate
    };
    
    enum Outputs {
        Trigger,        // Event triggers
        RateLatched,           // Event rate in Hz
        SubSampleOffset,    // sub-sample offset
        Phase           // Event phase [0,1) wrapped
    };
};

SchedulerBurstUGen::SchedulerBurstUGen() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<SchedulerBurstUGen, &SchedulerBurstUGen::next_aa>();
    next_aa(1);
    //m_scheduler.init();
}

SchedulerBurstUGen::~SchedulerBurstUGen() = default;

void SchedulerBurstUGen::next_aa(int nSamples) {
    // Input parameters
    const float* initTriggerIn = in(InitTrigger);
    const float* durationIn = in(Duration);
    const float* cyclesIn = in(Cycles);
    
    // Output pointers
    float* triggerOut = out(Trigger);
    float* rateOut = out(RateLatched);
    float* offsetOut = out(SubSampleOffset);
    float* phaseOut = out(Phase);
    
    for (int i = 0; i < nSamples; ++i) {
        auto event = m_scheduler.process(
            initTriggerIn[i] > 0.5f,
            durationIn[i],
            cyclesIn[i],
            m_sampleRate
        );
        
        triggerOut[i] = event.trigger;
        phaseOut[i] = event.phase;
        rateOut[i] = event.rate;
        offsetOut[i] = event.subSampleOffset;
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;

    // Event System
    registerUnit<SchedulerBurstUGen>(ft, "SchedulerBurstUGen", false);
    registerUnit<SchedulerCycle>(ft, "SchedulerCycleUGen", false);
    registerUnit<VoiceAllocator>(ft, "VoiceAllocatorUGen", false);
    registerUnit<RampIntegrator>(ft, "RampIntegrator", false);

    // Shift Register
    registerUnit<ShiftRegister>(ft, "ShiftRegisterUgen", false);
    
    // Unit Shapers
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);

    // Window Functions
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<RaisedCosWindow>(ft, "RaisedCosWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
    
    // Easing Functions
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}