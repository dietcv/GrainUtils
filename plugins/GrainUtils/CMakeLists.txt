# Build the GrainUtils plugins 
add_library(GrainUtils MODULE 
    EventSystem.cpp
    UnitShapers.cpp
    GrainUtils.cpp
)

# platform-specific linking
if(APPLE)
    target_link_libraries(GrainUtils "-undefined dynamic_lookup")
endif()