# Build the GrainUtils plugins 
add_library(GrainUtils MODULE 
    EventSystem.cpp
    ShiftRegister.cpp
    UnitShapers.cpp
    GrainUtils.cpp
    GrainDelay.cpp
)

# platform-specific linking
if(APPLE)
    target_link_libraries(GrainUtils "-undefined dynamic_lookup")
endif()