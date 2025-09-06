#include "EventSystem.hpp"
#include "ShiftRegister.hpp"
#include "UnitShapers.hpp"
#include "GrainDelay.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

PluginLoad(GrainUtilsUGens) {
    ft = inTable;

    // Grain Delay
    registerUnit<GrainDelay>(ft, "GrainDelay", false);

    // Event System
    registerUnit<SchedulerCycle>(ft, "SchedulerCycleUGen", false);
    registerUnit<SchedulerBurst>(ft, "SchedulerBurstUGen", false);
    registerUnit<VoiceAllocator>(ft, "VoiceAllocatorUGen", false);
    registerUnit<RampIntegrator>(ft, "RampIntegrator", false);
    registerUnit<RampAccumulator>(ft, "RampAccumulator", false);

    // Shift Register
    registerUnit<ShiftRegister>(ft, "ShiftRegisterUgen", false);
    
    // Unit Shapers
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);
    registerUnit<UnitCubic>(ft, "UnitCubic", false);

    // Window Functions
    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<PlanckWindow>(ft, "PlanckWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
    
    // Easing Functions
    registerUnit<JCurve>(ft, "JCurve", false);
    registerUnit<SCurve>(ft, "SCurve", false);
}