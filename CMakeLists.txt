cmake_minimum_required(VERSION 3.10)
set(lib_name "GrainUtils")
project("${lib_name}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set installation directory to build folder
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${lib_name}" CACHE PATH "Install prefix" FORCE)

if(NOT SC_PATH)
    set(SC_PATH "C:/Users/jlkle/Documents/C++Projects/supercollider")
endif()

message(STATUS "SC_PATH is: ${SC_PATH}")
message(STATUS "Install prefix is: ${CMAKE_INSTALL_PREFIX}")

if(EXISTS "${SC_PATH}/include/plugin_interface/SC_PlugIn.hpp")
    message(STATUS "Found SC_PlugIn.hpp")
else()
    message(FATAL_ERROR "Cannot find SC_PlugIn.hpp at ${SC_PATH}/include/plugin_interface/")
endif()

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)

set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")

add_subdirectory(plugins/GrainUtils)

# Install .sc files to Classes subdirectory
install(DIRECTORY plugins/GrainUtils/Classes
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.sc")

# Install .schelp files to HelpSource subdirectory  
install(DIRECTORY plugins/GrainUtils/HelpSource
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.schelp")

# Install .scx file to root plugin directory with platform-specific paths
if(APPLE OR UNIX)
  install(FILES ${CMAKE_BINARY_DIR}/plugins/GrainUtils/GrainUtils.scx 
          DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
if(WIN32)
  install(FILES ${CMAKE_BINARY_DIR}/plugins/GrainUtils/Release/GrainUtils.scx 
          DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()