#include "EventSystem.hpp"
#include "UnitShapers.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    
    // Event System
    registerUnit<EventScheduler>(ft, "EventSchedulerUGen", false);
    registerUnit<VoiceAllocator>(ft, "VoiceAllocatorUGen", false);
    registerUnit<RampIntegrator>(ft, "RampIntegrator", false);
    
    // Unit Shapers
    registerUnit<UnitTriangle>(ft, "UnitTriangle", false);
    registerUnit<UnitKink>(ft, "UnitKink", false);

    registerUnit<HanningWindow>(ft, "HanningWindow", false);
    registerUnit<GaussianWindow>(ft, "GaussianWindow", false);
    registerUnit<TrapezoidalWindow>(ft, "TrapezoidalWindow", false);
    registerUnit<TukeyWindow>(ft, "TukeyWindow", false);
    registerUnit<ExponentialWindow>(ft, "ExponentialWindow", false);
    
    registerUnit<SCurve>(ft, "SCurve", false);
}