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

# Distortion Plugins
set(Distortion_cpp_files
    plugins/Distortion/Distortion.cpp
)
set(Distortion_sc_files
    plugins/Distortion/Classes/Distortion.sc
)
set(Distortion_schelp_files
    plugins/Distortion/HelpSource/BuchlaFoldADAA.schelp
)
sc_add_server_plugin(
    "GrainUtils/Distortion" # destination directory
    "Distortion" # target name
    "${Distortion_cpp_files}"
    "${Distortion_sc_files}"
    "${Distortion_schelp_files}"
    ""
)

# Disperser Plugin
set(Disperser_cpp_files
    plugins/Disperser/Disperser.cpp
)
set(Disperser_sc_files
    plugins/Disperser/Classes/Disperser.sc
)
set(Disperser_schelp_files
    plugins/Disperser/HelpSource/Disperser.schelp
)
sc_add_server_plugin(
    "GrainUtils/Disperser" # destination directory
    "Disperser" # target name
    "${Disperser_cpp_files}"
    "${Disperser_sc_files}"
    "${Disperser_schelp_files}"
    ""
)

# GrainDelay Plugin
set(GrainDelay_cpp_files
    plugins/GrainDelay/GrainDelay.cpp
)
set(GrainDelay_sc_files
    plugins/GrainDelay/Classes/GrainDelay.sc
)
set(GrainDelay_schelp_files
    plugins/GrainDelay/HelpSource/GrainDelay.schelp
)
sc_add_server_plugin(
    "GrainUtils/GrainDelay" # destination directory
    "GrainDelay" # target name
    "${GrainDelay_cpp_files}"
    "${GrainDelay_sc_files}"
    "${GrainDelay_schelp_files}"
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
    plugins/Oscs/HelpSource/DualOscOS.schelp
    plugins/Oscs/HelpSource/SingleOscOS.schelp
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
    plugins/UnitSteps/HelpSource/UnitUrn.schelp
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