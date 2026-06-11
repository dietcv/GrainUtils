cmake_minimum_required(VERSION 3.10)
set(project_name "GrainUtils")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)

# Load modules
include(SuperColliderServerPlugin RESULT_VARIABLE server_plugin_found)
if(NOT server_plugin_found)
    message(FATAL_ERROR "Could not find server plugin functions module")
endif()

include(SuperColliderCompilerConfig RESULT_VARIABLE compiler_config_found)
if(NOT compiler_config_found)
    message(FATAL_ERROR "Could not find compiler config module")
endif()

# Windows - puts redistributable DLLs in install directory
include(InstallRequiredSystemLibraries)

sc_check_sc_path("${SC_PATH}")
message(STATUS "Found SuperCollider: ${SC_PATH}")
set(SC_PATH "${SC_PATH}" CACHE PATH
    "Path to SuperCollider source. Relative paths are treated as relative to this script" FORCE)

include("${SC_PATH}/SCVersion.txt")
message(STATUS "Building plugins for SuperCollider version: ${SC_VERSION}")

project(${project_name})
sc_do_initial_compiler_config()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
    message(WARNING "No install prefix provided, defaulting to $BUILD_DIR/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install prefix" FORCE)
endif()

message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")

# Options
option(SUPERNOVA "Build plugins for supernova" ON)
option(SCSYNTH "Build plugins for scsynth" ON)
option(NATIVE "Optimize for native architecture" OFF)
option(STRICT "Use strict warning flags" OFF)
option(NOVA_SIMD "Build plugins with nova-simd support." ON)

# Include directories for shared headers
include_directories(
    ${CMAKE_SOURCE_DIR}/plugins/Utils     
)

# Nova SIMD support
if (NOVA_SIMD)
    add_definitions(-DNOVA_SIMD)
    include_directories(${SC_PATH}/external_libraries/nova-simd)
endif()

set(GrainUtils_cpp_files
    plugins/Main/Main.cpp
    plugins/Delays/Delays.cpp
    plugins/Demand/Demand.cpp
    plugins/Distortion/Distortion.cpp
    plugins/EventSystem/EventSystem.cpp
    plugins/Filters/Filters.cpp
    plugins/Oscs/Oscs.cpp
    plugins/UnitEasing/UnitEasing.cpp
    plugins/UnitShapers/UnitShapers.cpp
    plugins/UnitSteps/UnitSteps.cpp
    plugins/UnitWindows/UnitWindows.cpp
)

set(GrainUtils_sc_files
    plugins/Delays/Classes/Delays.sc
    plugins/Demand/Classes/Demand.sc
    plugins/Distortion/Classes/Distortion.sc
    plugins/EventSystem/Classes/EventSystem.sc
    plugins/Filters/Classes/Filters.sc
    plugins/Oscs/Classes/Oscs.sc
    plugins/UnitEasing/Classes/UnitEasing.sc
    plugins/UnitShapers/Classes/UnitShapers.sc
    plugins/UnitSteps/Classes/UnitSteps.sc
    plugins/UnitWindows/Classes/UnitWindows.sc
)

set(GrainUtils_schelp_files
    # Delays
    plugins/Delays/HelpSource/GrainDelay.schelp

    # Demand
    plugins/Demand/HelpSource/Durn.schelp

    # Distortion
    plugins/Distortion/HelpSource/BuchlaFold.schelp

    # EventSystem
    plugins/EventSystem/HelpSource/RampAccumulator.schelp
    plugins/EventSystem/HelpSource/RampDivider.schelp
    plugins/EventSystem/HelpSource/RampIntegrator.schelp
    plugins/EventSystem/HelpSource/SchedulerBurst.schelp
    plugins/EventSystem/HelpSource/SchedulerCycle.schelp
    plugins/EventSystem/HelpSource/VoiceAllocator.schelp

    # Filters
    plugins/Filters/HelpSource/Disperser.schelp
    plugins/Filters/HelpSource/MorphSVF.schelp

    # Oscs
    plugins/Oscs/HelpSource/SingleOscOS.schelp
    plugins/Oscs/HelpSource/DualOscOS.schelp
    plugins/Oscs/HelpSource/PulsarOS.schelp
    plugins/Oscs/HelpSource/DualPulsarOS.schelp

    # UnitEasing
    plugins/UnitEasing/HelpSource/JCurve.schelp
    plugins/UnitEasing/HelpSource/SCurve.schelp

    # UnitShapers
    plugins/UnitShapers/HelpSource/UnitCubic.schelp
    plugins/UnitShapers/HelpSource/UnitKink.schelp
    plugins/UnitShapers/HelpSource/UnitTriangle.schelp

    # UnitSteps
    plugins/UnitSteps/HelpSource/UnitRegister.schelp
    plugins/UnitSteps/HelpSource/UnitStep.schelp
    plugins/UnitSteps/HelpSource/UnitWalk.schelp

    # UnitWindows
    plugins/UnitWindows/HelpSource/ExponentialWindow.schelp
    plugins/UnitWindows/HelpSource/GaussianWindow.schelp
    plugins/UnitWindows/HelpSource/HanningWindow.schelp
    plugins/UnitWindows/HelpSource/TrapezoidalWindow.schelp
    plugins/UnitWindows/HelpSource/TukeyWindow.schelp
)

sc_add_server_plugin(
    "GrainUtils"
    "GrainUtils"
    "${GrainUtils_cpp_files}"
    "${GrainUtils_sc_files}"
    "${GrainUtils_schelp_files}"
    ""
)

# Install Overviews
install(FILES plugins/Main/HelpSource/Overviews/GrainUtils.schelp
    DESTINATION GrainUtils/HelpSource/Overviews)

# Install Guides
install(DIRECTORY plugins/Main/HelpSource/Guides/
    DESTINATION GrainUtils/HelpSource/Guides
    FILES_MATCHING PATTERN "*.schelp")

message(STATUS "Generating plugin targets done")
