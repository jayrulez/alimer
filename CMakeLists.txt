# CMake version
cmake_minimum_required(VERSION 3.10)

# Project declaration
project (
    Alimer
    VERSION "0.1.0"
    DESCRIPTION "Cross-platform modern C++ 2D and 3D game engine."
    HOMEPAGE_URL "https://github.com/amerkoleci/alimer"
    LANGUAGES C CXX
)

# Options
if (WIN32)
    set(AGPU_API vk CACHE STRING  "Select GPU API [vk | d3d12]") 
	set_property(CACHE AGPU_API PROPERTY STRINGS vk d3d12)
elseif (WINDOWS_STORE)
    set(AGPU_API d3d12 CACHE STRING "Use d3d12 GPU API" FORCE)
elseif (EMSCRIPTEN)
    set(AGPU_API wgpu CACHE STRING "Use WebGPU GPU API" FORCE)
else()
    set(AGPU_API vk CACHE STRING  "Select GPU API [vk | gl]") 
	set_property(CACHE AGPU_API PROPERTY STRINGS vk gl)
endif ()

option(ALIMER_BUILD_SHARED "Build as a shared library" OFF)
option(ALIMER_BUILD_SAMPLES "Build sample projects" ON)
option(ALIMER_SKIP_INSTALL "Skips installation targets." OFF)

if (ANDROID OR IOS OR EMSCRIPTEN)
    set(ALIMER_BUILD_TOOLS OFF CACHE INTERNAL "Disable tools" FORCE)
    set(ALIMER_BUILD_EDITOR OFF CACHE INTERNAL "Disable C# editor" FORCE)
else ()
    option(ALIMER_BUILD_TOOLS "Build tools" ON)
    option(ALIMER_BUILD_EDITOR "Build Editor" ON)
endif ()

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

# Configure additional cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(alimer)

# third_party
add_subdirectory(third_party)

# engine
add_subdirectory(engine)

if (ALIMER_BUILD_TOOLS)
    add_subdirectory(tools)
endif ()

if (ALIMER_BUILD_SAMPLES)
    add_subdirectory(samples)
endif ()

# Print current build configuration
message (STATUS "Build Configuration:")
message (STATUS "GPU API:          ${AGPU_API}")

# Set VS Startup project.
if(CMAKE_VERSION VERSION_GREATER "3.6" AND ALIMER_BUILD_EDITOR)
    set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT "Editor")
endif()

# Install files
if (NOT ALIMER_SKIP_INSTALL)
    # README.md and license
    install (FILES
        ${ALIMER_ROOT_DIR}/README.md
        ${ALIMER_ROOT_DIR}/LICENSE
        DESTINATION .
    )

    # CMake modules
    install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/ DESTINATION cmake/cmake/)
endif ()
