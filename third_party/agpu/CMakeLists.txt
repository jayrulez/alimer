#
# Copyright (c) 2019-2020 Amer Koleci and contributors.
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

cmake_minimum_required(VERSION 3.10)
project(agpu)

# Options
if (WIN32 OR WINDOWS_STORE)
	option(AGPU_BACKEND_D3D11 "Include Direct3D11 backend" ON)
    option(AGPU_BACKEND_D3D12 "Include Direct3D12 backend (experimental)" ON)
    if (WIN32)
        option(AGPU_BACKEND_OPENGL "Include OpenGL backend" ON)
        option(AGPU_BACKEND_VULKAN "Include Vulkan backend (experimental)" ON)
    endif ()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(AGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(AGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(AGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Durango")
	option(AGPU_BACKEND_D3D11 "Include Direct3D11 backend" ON)
    option(AGPU_BACKEND_D3D12 "Include Direct3D12 backend (experimental)" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
	set(AGPU_BACKEND_OPENGL ON CACHE BOOL "Include OpenGL renderer")
else()
	message(FATAL_ERROR "Unknown platform ${CMAKE_SYSTEM_NAME}")
endif ()

option(AGPU_INSTALL "Generate installation target" ON)

set(AGPU_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# third_party
if (AGPU_BACKEND_VULKAN)
    # vulkan
    add_library(vulkan INTERFACE)
    set (VULKAN_INCLUDE_DIR ${AGPU_THIRD_PARTY_DIR}/khronos)
    target_sources (vulkan INTERFACE 
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_platform.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_sdk_platform.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan_core.h
    )
    target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})
    target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)

    if(ANDROID)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_ANDROID_KHR)
    elseif(WIN32)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR)
    elseif(APPLE)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_MACOS_MVK)
    elseif(UNIX)
        # See whether X11 is available. If not, fall back to direct-to-display mode.
        find_package(X11 QUIET)
        if (X11_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XCB_KHR)
        else()
            set(DIRECT_TO_DISPLAY TRUE)
            set(DIRECT_TO_DISPLAY TRUE PARENT_SCOPE)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_DISPLAY_KHR)
        endif()
    endif()
endif ()

# agpu
set(SOURCE_FILES 
    include/agpu.h
    src/agpu.cpp
    src/agpu_internal.h
)

add_library(agpu STATIC ${SOURCE_FILES})

# Public include paths
target_include_directories(agpu PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>
)

if (AGPU_BACKEND_OPENGL)
    target_sources(agpu PRIVATE
        src/agpu_gl.cpp
    )

    target_include_directories(agpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/khronos
    )
endif ()

if (AGPU_BACKEND_VULKAN)
    set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma)
    target_sources(agpu PRIVATE
        src/agpu_vk.cpp
        "${VMA_DIR}/vk_mem_alloc.h"
    )

    target_include_directories(agpu PRIVATE
        ${VMA_DIR}
    )
endif ()

if (MSVC)
	target_compile_options(agpu PRIVATE
        /W3     # Set warning level 3
        /WX     # Treat all other warnings as errors
    )
endif ()

if (AGPU_INSTALL)
    install(TARGETS agpu
        EXPORT agpu
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
