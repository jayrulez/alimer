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
option(ALIMER_BUILD_SHARED "Build as a shared library" OFF)
option(ALIMER_BUILD_SAMPLES "Build sample projects" ON)
option(ALIMER_SKIP_INSTALL "Skips installation targets." OFF)

# Options
if (WIN32)
    #option(ALIMER_GRAPHICS_D3D11 "Include Direct3D11 backend" ON)
    option(ALIMER_GRAPHICS_D3D12 "Include Direct3D12 backend" ON)
    option(ALIMER_GRAPHICS_VULKAN "Include Vulkan backend" ON)
elseif (WINDOWS_STORE)
    #set(ALIMER_GRAPHICS_D3D11 ON CACHE BOOL "Include Direct3D11 renderer")
    set(ALIMER_GRAPHICS_D3D12 ON CACHE BOOL "Include Direct3D12 backend")
else ()
    set(ALIMER_GRAPHICS_VULKAN ON CACHE BOOL "Include Vulkan backend")
endif ()

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
if (ALIMER_GRAPHICS_D3D12)
    message (STATUS "  Renderer:       Direct3D12")
endif()
if (ALIMER_GRAPHICS_VULKAN)
    message (STATUS "  Renderer:       Vulkan")
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
