#
# Copyright (c) 2019 Amer Koleci.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# CMake version
cmake_minimum_required(VERSION 3.10)
project(vgpu)

set(VGPU_VERSION_MAJOR 0)
set(VGPU_VERSION_MINOR 1)
set(VGPU_VERSION_PATCH 0)
set(VGPU_VERSION ${VGPU_VERSION_MAJOR}.${VGPU_VERSION_MINOR}.${VGPU_VERSION_PATCH})

# Enable folders in IDE (VisualStudio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output paths
set(VGPU_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VGPU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
mark_as_advanced(VGPU_ROOT_DIR)

set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VGPU_BUILD_DIR}/bin")
set (CMAKE_PDB_OUTPUT_DIRECTORY "${VGPU_BUILD_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${VGPU_BUILD_DIR}/lib")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${VGPU_BUILD_DIR}/lib")

# Options
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    option(VGPU_BACKEND_D3D11 "Include Direct3D12 backend" ON)
    option(VGPU_BACKEND_D3D12 "Include Direct3D12 backend" ON)
    option(VGPU_BACKEND_OPENGL "Include OpenGL backend" ON)
    option(VGPU_BACKEND_VULKAN "Include Vulkan backend" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
    option(VGPU_BACKEND_D3D11 "Include Direct3D12 backend" ON)
    option(VGPU_BACKEND_D3D12 "Include Direct3D12 backend" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Durango")
    # XboxOne
    option(VGPU_BACKEND_D3D11 "Include Direct3D12 backend" ON)
    option(VGPU_BACKEND_D3D12 "Include Direct3D12 backend" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(VGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
else()
    set(VGPU_BACKEND_OPENGL ON CACHE BOOL "Include OpenGL renderer")
    set(VGPU_BACKEND_OPENGL ON CACHE BOOL "Include OpenGL renderer")
endif ()

option(VGPU_BUILD_SAMPLES "Build the vgpu samples" ON)
option(VGPU_BUILD_SHARED  "Build as shared library" OFF)

# third_party

# GLFW
if (VGPU_BUILD_SAMPLES AND NOT (EMSCRIPTEN OR ANDROID))
    set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set (GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
    #set (GLFW_USE_WAYLAND ${VGPU_USE_WAYLAND} CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw)
    set_property(TARGET "glfw" PROPERTY FOLDER "third_party")
    unset (LIB_SUFFIX CACHE)
endif ()


# vgpu
set(SOURCE_FILES 
    include/vgpu/vgpu.h
    src/stb_ds.h
    src/vgpu_backend.h
    src/vgpu.c
    src/vgpu_gl.c
    src/vgpu_vk.cpp
    #src/vgpu_d3d11.cpp
    #src/vgpu_d3d12.cpp
    #src/vgpu_d3d.h
)

if (VGPU_BUILD_SHARED)
    add_library (vgpu SHARED ${SOURCE_FILES})
    target_compile_definitions(vgpu PRIVATE VGPU_SHARED_LIBRARY)
else ()
    add_library (vgpu ${SOURCE_FILES})
endif ()
target_compile_definitions(vgpu PRIVATE VGPU_IMPLEMENTATION)

if (VGPU_BACKEND_D3D11)
    #target_compile_definitions(vgpu PRIVATE VGPU_BACKEND_D3D11)
endif ()

if (VGPU_BACKEND_VULKAN)
    target_sources(vgpu PRIVATE
        src/lib/vkbind/vkbind.h
        src/lib/vma/vk_mem_alloc.h
    )

    if (MSVC)
        target_sources(vgpu PRIVATE
            src/lib/vma/vk_mem_alloc.natvis
        )
    endif ()

    target_compile_definitions(vgpu PRIVATE VGPU_BACKEND_VULKAN)
endif ()

# OpenGL files
if (VGPU_BACKEND_OPENGL)
    target_sources(vgpu PRIVATE
        src/lib/glad/include/KHR/khrplatform.h
        src/lib/glad/include/glad/glad.h
        src/lib/glad/src/glad.c
    )

    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/lib/glad/include>
    )

    target_compile_definitions(vgpu PRIVATE VGPU_BACKEND_OPENGL)
endif ()

set_target_properties(vgpu PROPERTIES
    VERSION ${VGPU_VERSION}
    SOVERSION ${VGPU_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(vgpu PUBLIC
    "$<BUILD_INTERFACE:${VGPU_ROOT_DIR}/include>"
)

target_include_directories(vgpu PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/lib>"
)

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
        /W3
        /WX
    )
endif ()

# samples
if (VGPU_BUILD_SAMPLES)
    add_subdirectory(samples)
endif ()

# Print current build configuration
message(STATUS "VGPU Build Configuration:")
if (VGPU_BUILD_SHARED)
    message(STATUS "  Library         SHARED")
else ()
    message(STATUS "  Library         STATIC")
endif ()

if (VGPU_BACKEND_D3D11)
    message (STATUS "Build Renderer:        Direct3D11")
endif()
if (VGPU_BACKEND_D3D12)
    message (STATUS "Build Renderer:        Direct3D12")
endif()
if (VGPU_BACKEND_VULKAN)
    message (STATUS "Build Renderer:        Vulkan")
endif()
if (VGPU_BACKEND_OPENGL)
    message (STATUS "Build Renderer:        OpenGL")
endif()
