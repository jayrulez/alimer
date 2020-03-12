# CMake version
cmake_minimum_required(VERSION 3.10)

# Project declaration
project (Alimer)

# Target C++14 without extensions
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

# Options
if (WIN32)
	option(ALIMER_D3D12 "Include Direct3D12 backend" ON)
    option(ALIMER_VULKAN "Include Vulkan backend" ON)
elseif (WINDOWS_STORE)
    set (ALIMER_D3D12 ON CACHE BOOL "Use Direct3D12 Graphics backend" FORCE)
elseif (EMSCRIPTEN)
    set (ALIMER_WGPU ON CACHE BOOL "Use WebGPU Graphics backend" FORCE)
else()
	set (ALIMER_VULKAN ON CACHE BOOL "Use Vulkan Graphics backend" FORCE)
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

# Install paths
set (DEST_BASE_INCLUDE_DIR include)
set (DEST_INCLUDE_DIR ${DEST_BASE_INCLUDE_DIR}/alimer)
set (DEST_THIRDPARTY_HEADERS_DIR ${DEST_INCLUDE_DIR}/ThirdParty)
set (DEST_ARCHIVE_DIR lib)
if (ANDROID)
    set (DEST_LIBRARY_DIR ${DEST_ARCHIVE_DIR})
else ()
    set (DEST_LIBRARY_DIR bin)
endif ()
set (DEST_BIN_DIR bin)

if (MSVC OR "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR}/$<CONFIG>)
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR}/$<CONFIG>)
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR}/$<CONFIG>)
else ()
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR})
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR})
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR})
endif ()

# OS specific
if (UNIX AND NOT APPLE AND NOT ANDROID AND NOT EMSCRIPTEN)
    
endif()

if (ANDROID OR IOS OR EMSCRIPTEN)
    set (MOBILE ON CACHE BOOL "" FORCE)
elseif (APPLE OR "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set (APPLE ON CACHE BOOL "" FORCE)
endif ()

# Add colors to ninja builds
if (CMAKE_GENERATOR STREQUAL "Ninja")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif()

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
if (ALIMER_VULKAN)
    message (STATUS "Graphics API:          Vulkan")
endif ()

if (ALIMER_D3D12)
    message (STATUS "Graphics API:          Direct3D12")
endif ()

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
