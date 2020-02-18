# CMake version
cmake_minimum_required(VERSION 3.10)

# Project declaration
project (alimer)

# Options
#option(ALIMER_BUILD_SHARED "Build as a shared library instad of executable" OFF)
option(ALIMER_BUILD_SAMPLES "Build sample projects" ON)

if (WIN32)
    # Link statically against c/c++ lib to avoid missing redistriburable such as
    # "VCRUNTIME140.dll not found. Try reinstalling the app.", but give users
    # a choice to opt for the shared runtime if they want.
    option(ALIMER_STATIC_RUNTIME  "Link against the static runtime libraries." ON)
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
    set (LINUX ON CACHE BOOL "" FORCE)
endif()

if (ANDROID OR IOS OR EMSCRIPTEN)
    set (MOBILE ON CACHE BOOL "" FORCE)
elseif (APPLE OR "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set (APPLE ON CACHE BOOL "" FORCE)
endif ()

# Compiler check
set(MIN_CLANG_VERSION "6.0")

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    if (CMAKE_C_COMPILER_VERSION VERSION_LESS MIN_CLANG_VERSION)
        message(FATAL_ERROR "Detected C compiler Clang ${CMAKE_C_COMPILER_VERSION} < ${MIN_CLANG_VERSION}")
    endif()
elseif (NOT MSVC)
    message(FATAL_ERROR "Detected C compiler ${CMAKE_C_COMPILER_ID} is unsupported")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_CLANG_VERSION)
        message(FATAL_ERROR "Detected CXX compiler Clang ${CMAKE_CXX_COMPILER_VERSION} < ${MIN_CLANG_VERSION}")
    endif()
elseif (NOT MSVC)
    message(FATAL_ERROR "Detected CXX compiler ${CMAKE_CXX_COMPILER_ID} is unsupported")
endif()

# Detect use of the clang-cl.exe frontend, which does not support all of clangs normal options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(CLANG_CL ON CACHE BOOL "" FORCE)
    endif()
elseif (MSVC)
    set(MSVC_NATIVE ON CACHE BOOL "" FORCE)
endif()

# General compiler flags
if (CLANG_CL OR MSVC)
    if (ALIMER_STATIC_RUNTIME)
        set(CRT_FLAGS_RELEASE "/MT")
        set(CRT_FLAGS_DEBUG "/MTd")
    else ()
        set(CRT_FLAGS_RELEASE "/MD")
        set(CRT_FLAGS_DEBUG "/MDd")
    endif ()

    # In DEBUG, avoid generating a PDB file which seems to mess with incremental compilation.
    # Instead generate debug info directly inside obj files.
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CRT_FLAGS_DEBUG} /Z7")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CRT_FLAGS_DEBUG} /Z7")

     # In RELEASE, also generate PDBs.
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CRT_FLAGS_RELEASE} /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CRT_FLAGS_RELEASE} /Zi")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${CRT_FLAGS_RELEASE} /Z7")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CRT_FLAGS_RELEASE} /Z7")

    # Disable exceptions
    if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif ()

# Add colors to ninja builds
if (CMAKE_GENERATOR STREQUAL "Ninja")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif()

set(CXX_STANDARD "-std=c++14")
if (WIN32)
    set(CXX_STANDARD "/std:c++14")
endif()

if (MSVC_NATIVE)
    set(CXX_STANDARD "/std:c++latest")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /W0 /Zc:__cplusplus")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function")
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES=1")
endif()

# third_party
add_subdirectory(third_party)

# src
add_subdirectory(src)

if (ALIMER_BUILD_SAMPLES)
    add_subdirectory(samples)
endif ()

# Install files
# README.md and license
install (FILES
    ${ALIMER_ROOT_DIR}/README.md
    ${ALIMER_ROOT_DIR}/LICENSE
    DESTINATION .
)
