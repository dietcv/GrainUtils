#include "SC_PlugIn.hpp"

InterfaceTable* ft;

// Forward declarations
void Delays_setup();
void Demand_setup();
void Distortion_setup();
void EventSystem_setup();
void Filters_setup();
void Oscs_setup();
void UnitEasing_setup();
void UnitShapers_setup();
void UnitSteps_setup();
void UnitWindows_setup();

PluginLoad(GrainUtils) {
    ft = inTable;
    Delays_setup();
    Demand_setup();
    Distortion_setup();
    EventSystem_setup();
    Filters_setup();
    Oscs_setup();
    UnitEasing_setup();
    UnitShapers_setup();
    UnitSteps_setup();
    UnitWindows_setup();
}