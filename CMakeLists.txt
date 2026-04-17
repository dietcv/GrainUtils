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

# Demand Plugins
set(Demand_cpp_files
    plugins/Demand/Demand.cpp
)
set(Demand_sc_files
    plugins/Demand/Classes/Demand.sc
)
set(Demand_schelp_files
    plugins/Demand/HelpSource/Durn.schelp
)
sc_add_server_plugin(
    "GrainUtils/Demand" # destination directory
    "Demand" # target name
    "${Demand_cpp_files}"
    "${Demand_sc_files}"
    "${Demand_schelp_files}"
    ""
)

# Distortion Plugins
set(Distortion_cpp_files
    plugins/Distortion/Distortion.cpp
)
set(Distortion_sc_files
    plugins/Distortion/Classes/Distortion.sc
)
set(Distortion_schelp_files
    plugins/Distortion/HelpSource/BuchlaFold.schelp
)
sc_add_server_plugin(
    "GrainUtils/Distortion" # destination directory
    "Distortion" # target name
    "${Distortion_cpp_files}"
    "${Distortion_sc_files}"
    "${Distortion_schelp_files}"
    ""
)

# Filter Plugins
set(Filters_cpp_files
    plugins/Filters/Filters.cpp
)
set(Filters_sc_files
    plugins/Filters/Classes/Filters.sc
)
set(Filters_schelp_files
    plugins/Filters/HelpSource/Disperser.schelp
)
sc_add_server_plugin(
    "GrainUtils/Filters" # destination directory
    "Filters" # target name
    "${Filters_cpp_files}"
    "${Filters_sc_files}"
    "${Filters_schelp_files}"
    ""
)

# Delay Plugins
set(Delays_cpp_files
    plugins/Delays/Delays.cpp
)
set(Delays_sc_files
    plugins/Delays/Classes/Delays.sc
)
set(Delays_schelp_files
    plugins/Delays/HelpSource/GrainDelay.schelp
)
sc_add_server_plugin(
    "GrainUtils/Delays" # destination directory
    "Delays" # target name
    "${Delays_cpp_files}"
    "${Delays_sc_files}"
    "${Delays_schelp_files}"
    ""
)

# EventSystem Plugins
set(EventSystem_cpp_files
    plugins/EventSystem/EventSystem.cpp
)
set(EventSystem_sc_files
    plugins/EventSystem/Classes/EventSystem.sc
)
set(EventSystem_schelp_files
    plugins/EventSystem/HelpSource/Classes/SchedulerCycle.schelp
    plugins/EventSystem/HelpSource/Classes/SchedulerBurst.schelp
    plugins/EventSystem/HelpSource/Classes/VoiceAllocator.schelp
    plugins/EventSystem/HelpSource/Classes/RampIntegrator.schelp
    plugins/EventSystem/HelpSource/Classes/RampAccumulator.schelp
    plugins/EventSystem/HelpSource/Classes/RampDivider.schelp
)
sc_add_server_plugin(
    "GrainUtils/EventSystem" # destination directory
    "EventSystem" # target name
    "${EventSystem_cpp_files}"
    "${EventSystem_sc_files}"
    "${EventSystem_schelp_files}"
    ""
)

# Install GrainUtils Overview
install(FILES HelpSource/Overviews/GrainUtils.schelp
	DESTINATION GrainUtils/HelpSource/Overviews)

# Install guides to the HelpSource/Guides directory 
install(DIRECTORY plugins/EventSystem/HelpSource/Guides/
        DESTINATION GrainUtils/EventSystem/HelpSource/Guides
        FILES_MATCHING PATTERN "*.schelp")

# Osc Plugins
set(Oscs_cpp_files
    plugins/Oscs/Oscs.cpp
)
set(Oscs_sc_files
    plugins/Oscs/Classes/Oscs.sc
)
set(Oscs_schelp_files
    plugins/Oscs/HelpSource/SingleOscOS.schelp
    plugins/Oscs/HelpSource/DualOscOS.schelp
    plugins/Oscs/HelpSource/PulsarOS.schelp
    plugins/Oscs/HelpSource/DualPulsarOS.schelp
)
sc_add_server_plugin(
    "GrainUtils/Oscs" # destination directory
    "Oscs" # target name
    "${Oscs_cpp_files}"
    "${Oscs_sc_files}"
    "${Oscs_schelp_files}"
    ""
)

# UnitEasing Plugins
set(UnitEasing_cpp_files
    plugins/UnitEasing/UnitEasing.cpp
)
set(UnitEasing_sc_files
    plugins/UnitEasing/Classes/UnitEasing.sc
)
set(UnitEasing_schelp_files
    plugins/UnitEasing/HelpSource/JCurve.schelp
    plugins/UnitEasing/HelpSource/SCurve.schelp
)
sc_add_server_plugin(
    "GrainUtils/UnitEasing" # destination directory
    "UnitEasing" # target name
    "${UnitEasing_cpp_files}"
    "${UnitEasing_sc_files}"
    "${UnitEasing_schelp_files}"
    ""
)        

# UnitShaper Plugins
set(UnitShapers_cpp_files
    plugins/UnitShapers/UnitShapers.cpp
)
set(UnitShapers_sc_files
    plugins/UnitShapers/Classes/UnitShapers.sc
)
set(UnitShapers_schelp_files
    plugins/UnitShapers/HelpSource/UnitKink.schelp
    plugins/UnitShapers/HelpSource/UnitTriangle.schelp
    plugins/UnitShapers/HelpSource/UnitCubic.schelp
)
sc_add_server_plugin(
    "GrainUtils/UnitShapers" # destination directory
    "UnitShapers" # target name
    "${UnitShapers_cpp_files}"
    "${UnitShapers_sc_files}"
    "${UnitShapers_schelp_files}"
    ""
)

# UnitStep Plugins
set(UnitSteps_cpp_files
    plugins/UnitSteps/UnitSteps.cpp
)
set(UnitSteps_sc_files
    plugins/UnitSteps/Classes/UnitSteps.sc
)
set(UnitSteps_schelp_files
    plugins/UnitSteps/HelpSource/UnitStep.schelp
    plugins/UnitSteps/HelpSource/UnitWalk.schelp
    plugins/UnitSteps/HelpSource/UnitRegister.schelp
)
sc_add_server_plugin(
    "GrainUtils/UnitSteps" # destination directory
    "UnitSteps" # target name
    "${UnitSteps_cpp_files}"
    "${UnitSteps_sc_files}"
    "${UnitSteps_schelp_files}"
    ""
)

# UnitWindow Plugins
set(UnitWindows_cpp_files
    plugins/UnitWindows/UnitWindows.cpp
)
set(UnitWindows_sc_files
    plugins/UnitWindows/Classes/UnitWindows.sc
)
set(UnitWindows_schelp_files
    plugins/UnitWindows/HelpSource/HanningWindow.schelp
    plugins/UnitWindows/HelpSource/GaussianWindow.schelp
    plugins/UnitWindows/HelpSource/TrapezoidalWindow.schelp
    plugins/UnitWindows/HelpSource/TukeyWindow.schelp
    plugins/UnitWindows/HelpSource/ExponentialWindow.schelp
)
sc_add_server_plugin(
    "GrainUtils/UnitWindows" # destination directory
    "UnitWindows" # target name
    "${UnitWindows_cpp_files}"
    "${UnitWindows_sc_files}"
    "${UnitWindows_schelp_files}"
    ""
)

message(STATUS "Generating plugin targets done")
