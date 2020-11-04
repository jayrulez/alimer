# CMake version
cmake_minimum_required (VERSION 3.14)

# Configure CMake global variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project declaration
project (
    Alimer
    VERSION "0.1.0"
    DESCRIPTION "Cross-platform modern C++ 2D and 3D game engine."
    HOMEPAGE_URL "https://github.com/amerkoleci/alimer"
    LANGUAGES CXX
)

# For build type to relese if not specified
if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not set, forcing it to Debug")
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

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
  set(ALIMER_UWP_VERSION_MIN  "10.0.18362.0")
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
option(ALIMER_LOGGING "Enable logging macros" ON)
option(ALIMER_PROFILING "Enable performance profiling" ON)
option(ALIMER_THREADING "Enable multithreading" ON)
option(ALIMER_NETWORK "Enable Networking system " ON)
option(ALIMER_PHYSICS "Enable Physics system" ON)
option(ALIMER_IMGUI "Enable ImGui system" ON)

if (WIN32)
  option(ALIMER_D3D12 "Enable D3D12 backend" ON)
  option(ALIMER_VULKAN "Enable Vulkan backend" ON)
elseif (WINDOWS_STORE)
  set(ALIMER_D3D12 ON CACHE STRING "Use D3D12 Graphics API" FORCE)
elseif (IOS)
  set(ALIMER_METAL ON CACHE STRING "Use Metal Graphics API" FORCE)
else ()
  set(ALIMER_VULKAN ON CACHE STRING "Use Vulkan Graphics API" FORCE)
endif ()

if (NOT ANDROID AND NOT EMSCRIPTEN AND NOT IOS AND NOT WINDOWS_STORE)
    option(ALIMER_BUILD_TOOLS "Build tools" TRUE)
    option(ALIMER_BUILD_EDITOR "Build Editor" ON)
endif()

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
elseif(APPLE)
elseif(WINDOWS_STORE)
    # Disable run-time type information (RTTI)
    string(REGEX REPLACE "/GR" "/GR-" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(WIN32)

    # Disable run-time type information (RTTI)
    string(REGEX REPLACE "/GR" "/GR-" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    # Select static/dynamic runtime library
    option(ALIMER_USE_STATIC_CRT "Link against the static runtime libraries." OFF)
    if (ALIMER_USE_STATIC_CRT)
        string(REGEX REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
		string(REGEX REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        string(REGEX REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
		string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO }")
        string(REGEX REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
		string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	endif()
elseif(UNIX)
  find_package(PkgConfig)
endif()

set(ALIMER_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}")

# Setup SDK install destinations
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

# Third Party
add_subdirectory(third_party)

if (ALIMER_BUILD_TOOLS)
    add_subdirectory(tools/ShaderCompiler)
endif()

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

message(STATUS "  Logging         ${ALIMER_LOGGING}")
message(STATUS "  Profiling       ${ALIMER_PROFILING}")
message(STATUS "  Threading       ${ALIMER_THREADING}")
if (ALIMER_D3D12)
  message(STATUS "  Graphics API:   Direct3D12 (ALIMER_D3D12)")
endif ()
if (ALIMER_VULKAN)
  message(STATUS "  Graphics API:   Vulkan (ALIMER_VULKAN)")
endif ()
message(STATUS "  Network         ${ALIMER_NETWORK}")
message(STATUS "  Physics         ${ALIMER_PHYSICS}")

# Install files
# README.md and license
install (FILES
    ${ALIMER_ROOT_DIR}/README.md
    ${ALIMER_ROOT_DIR}/LICENSE
    DESTINATION .
)

# CMake modules
install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/ DESTINATION cmake/)

# Install data files
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOT ANDROID)
    foreach(dir assets)
        if (MSVC OR "${CMAKE_GENERATOR}" STREQUAL "Xcode")
            foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
                create_symlink(${CMAKE_CURRENT_SOURCE_DIR}/${dir} ${ALIMER_BIN_DIR}/${dir})
            endforeach ()
        else()
            create_symlink(${CMAKE_CURRENT_SOURCE_DIR}/${dir} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dir})
        endif ()
        install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir} DESTINATION ${DEST_BIN_DIR_CONFIG}/)
    endforeach()

    # DLLs
    if(WIN32 AND NOT (WINDOWS_STORE))
        install (FILES
            ${ALIMER_THIRD_PARTY_DIR}/dxc/bin/dxil.dll
            ${ALIMER_THIRD_PARTY_DIR}/dxc/bin/dxcompiler.dll
            DESTINATION ${DEST_BIN_DIR_CONFIG}
        )
    endif ()
endif ()
