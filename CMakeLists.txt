# CMake version
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project declaration
project (
    Alimer
    VERSION "0.1.0"
    DESCRIPTION "Cross-platform modern C++ 2D and 3D game engine."
    HOMEPAGE_URL "https://github.com/amerkoleci/alimer"
    LANGUAGES C CXX
)

# Options
set(ALIMER_WARNINGS_AS_ERRORS ON CACHE BOOL "Enable Warnings as Errors")
option(ALIMER_BUILD_SHARED "Build as a shared library" OFF)
option(ALIMER_BUILD_SAMPLES "Build sample projects" ON)

# Graphics module backend
if (WIN32)
    option(ALIMER_OPENGL "Include OpenGL backend" ON)
    option(ALIMER_D3D11 "Include Direct3D11 backend" ON)
    #option(ALIMER_D3D12 "Include Direct3D12 backend" ON)
    #option(ALIMER_VULKAN "Include Vulkan backend" ON)
elseif (WINDOWS_STORE)
    set(ALIMER_D3D11 ON CACHE BOOL "Use Direct3D11 backend")
else ()
    set(ALIMER_OPENGL ON CACHE BOOL "Use OpenGL backend")
    #set(ALIMER_VULKAN ON CACHE BOOL "Use Vulkan backend")
endif ()

if (ANDROID OR IOS OR EMSCRIPTEN)
    set(ALIMER_BUILD_TOOLS OFF CACHE INTERNAL "Disable tools" FORCE)
    set(ALIMER_BUILD_EDITOR OFF CACHE INTERNAL "Disable editor" FORCE)
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

# Third Party 
add_subdirectory(third_party)

# Source
add_subdirectory(Source)

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
message(STATUS "Build Configuration:")
if (ALIMER_BUILD_SHARED)
    message(STATUS "  Library         SHARED")
else ()
    message(STATUS "  Library         STATIC")
endif ()
if (ALIMER_D3D11)
    message (STATUS "  Renderer:       Direct3D11")
endif()
if (ALIMER_OPENGL)
    message (STATUS "  Renderer:       OpenGL")
endif()
if (ALIMER_D3D12)
    message (STATUS "  Renderer:       Direct3D12")
endif()
if (ALIMER_VULKAN)
    message (STATUS "  Renderer:       Vulkan")
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
