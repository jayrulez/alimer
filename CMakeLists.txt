# CMake version
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project declaration
project (
    Alimer
    VERSION "0.1.0"
    DESCRIPTION "Cross-platform modern C++ 2D and 3D game engine."
    HOMEPAGE_URL "https://github.com/amerkoleci/alimer"
    LANGUAGES C CXX
)

# For build type to relese if not specified
if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not set, forcing it to Debug")
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# Enable folders in IDE (VisualStudio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Build version
set (ALIMER_VERSION_MAJOR 0)
set (ALIMER_VERSION_MINOR 1)
set (ALIMER_VERSION_PATCH 0)
set (ALIMER_VERSION ${ALIMER_VERSION_MAJOR}.${ALIMER_VERSION_MINOR}.${ALIMER_VERSION_PATCH})
set (ALIMER_VERSION_ALIAS "WIP")

# Root and build dir.
set (ALIMER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ALIMER_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set (ALIMER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
mark_as_advanced(ALIMER_ROOT_DIR)
mark_as_advanced(ALIMER_THIRD_PARTY_DIR)

# Configure CMake global variables
set (CMAKE_INSTALL_MESSAGE LAZY)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ALIMER_BUILD_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ALIMER_BUILD_DIR}/lib")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ALIMER_BUILD_DIR}/lib")

# Detect platform
if(EMSCRIPTEN)
  set(ALIMER_NATIVE_PLATFORM "Web")
elseif(ANDROID)  
  set(ALIMER_NATIVE_PLATFORM "Android")
elseif(APPLE)
  set(ALIMER_NATIVE_PLATFORM "Apple")
elseif(WINDOWS_STORE)
  set(ALIMER_NATIVE_PLATFORM "UWP")
elseif(WIN32)
  set(ALIMER_NATIVE_PLATFORM "Win32")
elseif(UNIX)
  set(ALIMER_NATIVE_PLATFORM "Unix") 
else()
  message(FATAL_ERROR "Unrecognized platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Configure additional cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(alimer)

# Options
option(BUILD_SHARED_LIBS  "Build shared libraries" OFF)
option(ALIMER_BUILD_SAMPLES "Build sample projects" ON)

if (ANDROID OR IOS OR EMSCRIPTEN OR WINDOWS_STORE)
    set(ALIMER_BUILD_TOOLS OFF CACHE INTERNAL "Disable tools" FORCE)
    set(ALIMER_BUILD_EDITOR OFF CACHE INTERNAL "Disable editor" FORCE)
else ()
    option(ALIMER_BUILD_TOOLS "Build tools" ON)
    option(ALIMER_BUILD_EDITOR "Build Editor" ON)
endif ()

# Default values for the backend-enabling options
set(ENABLE_D3D11 OFF)
set(ENABLE_METAL OFF)
set(ENABLE_OPENGL OFF)
set(ENABLE_VULKAN OFF)

if(EMSCRIPTEN)
    set(ENABLE_OPENGL ON)
elseif(ANDROID)
    set(ENABLE_OPENGL ON)
    set(ENABLE_VULKAN ON)
elseif(WINDOWS_STORE)
    set(ENABLE_D3D11 ON)
elseif (WIN32)
    set(ENABLE_D3D11 ON)
    set(ENABLE_VULKAN ON)
    set(ENABLE_OPENGL ON)
elseif(APPLE)
    set(ENABLE_METAL ON)
elseif(UNIX)
    set(ENABLE_OPENGL ON)
    set(ENABLE_VULKAN ON)
endif()
option(ALIMER_ENABLE_D3D11 "Enable compilation of the D3D11 backend" ${ENABLE_D3D11})
option(ALIMER_ENABLE_METAL "Enable compilation of the Metal backend" ${ENABLE_METAL})
option(ALIMER_ENABLE_NULL "Enable compilation of the Null backend" ON)
option(ALIMER_ENABLE_OPENGL "Enable compilation of the OpenGL backend" ${ENABLE_OPENGL})
option(ALIMER_ENABLE_VULKAN "Enable compilation of the Vulkan backend" ${ENABLE_VULKAN})

# Setup
if(EMSCRIPTEN)
  string(CONCAT ALIMER_EMSCRIPTEN_FLAGS
    "-Os "
    "-s USE_WEBGL2=1 "
    "-s FORCE_FILESYSTEM=1 "
    "-s USE_PTHREADS=1 "
  )

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALIMER_EMSCRIPTEN_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALIMER_EMSCRIPTEN_FLAGS}")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
elseif(ANDROID)
  find_package(Java REQUIRED)
elseif(UNIX)
  find_package(PkgConfig)
  if(NOT APPLE)
    set(CMAKE_SKIP_RPATH OFF)
  endif()
endif()

# Third Party 
add_subdirectory(third_party)

# Engine
add_subdirectory(engine)

if (ALIMER_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if (ALIMER_BUILD_SAMPLES)
    add_subdirectory(samples)
endif ()

# Set VS Startup project.
if(CMAKE_VERSION VERSION_GREATER "3.6" AND ALIMER_BUILD_EDITOR)
    set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT "Editor")
endif()

# Print current build configuration
message(STATUS "Alimer Engine version ${ALIMER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} - BuildType: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build platform: ${ALIMER_NATIVE_PLATFORM}")

if (BUILD_SHARED_LIBS)
    message(STATUS "  Library         SHARED")
else ()
    message(STATUS "  Library         STATIC")
endif ()

if(ALIMER_WARNINGS_AS_ERRORS)
    message(STATUS "  Warnings as Errors Enabled")
endif()

# Print graphics backends.
if(ALIMER_ENABLE_D3D11)
    message(STATUS "  Build Renderer: Direct3D 11")
endif()
if(ALIMER_ENABLE_METAL)
    message(STATUS "  Build Renderer: Metal")
endif()
if(ALIMER_ENABLE_OPENGL)
    message(STATUS "  Build Renderer: OpenGL")
endif()
if(ALIMER_ENABLE_NULL)
    message(STATUS "  Build Renderer: Null")
endif()

if(ALIMER_ENABLE_VULKAN)
    message(STATUS "  Build Renderer: Vulkan")
endif()

# Install files
# README.md and license
install (FILES
    ${ALIMER_ROOT_DIR}/README.md
    ${ALIMER_ROOT_DIR}/LICENSE
    DESTINATION .
)

# CMake modules
install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/ DESTINATION cmake/cmake/)
