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
project(vgpu VERSION 1.0.0 LANGUAGES C)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(VGPU_STANDALONE TRUE)
endif()

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(VGPU_BUILD_EXAMPLES "Build the vgpu example programs" ${VGPU_STANDALONE})
option(VGPU_INSTALL "Generate installation target" ON)

if (WIN32 OR WINDOWS_STORE)
	option(VGPU_BACKEND_D3D11 "Include Direct3D11 backend" ON)
    option(VGPU_BACKEND_D3D12 "Include Direct3D12 backend (experimental)" ON)
    if (WIN32)
        option(VGPU_BACKEND_OPENGL "Include OpenGL backend" ON)
        option(VGPU_BACKEND_VULKAN "Include Vulkan backend (experimental)" ON)
    endif ()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(VGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(VGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(VGPU_BACKEND_VULKAN ON CACHE BOOL "Include Vulkan renderer")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Durango")
	option(VGPU_BACKEND_D3D11 "Include Direct3D11 backend" ON)
    option(VGPU_BACKEND_D3D12 "Include Direct3D12 backend (experimental)" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
	set(VGPU_BACKEND_OPENGL ON CACHE BOOL "Include OpenGL renderer")
else()
	message(FATAL_ERROR "Unknown platform ${CMAKE_SYSTEM_NAME}")
endif ()

# Setup third party libs used
if (VGPU_BACKEND_VULKAN)
    # vulkan headers
    add_library(vulkan INTERFACE)
    set (VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/khronos)
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
    include/vgpu/vgpu.h
    src/vgpu_backend.h
    src/vgpu.c
    #src/agpu_backend_gl.c
    #src/agpu_vk.cpp
    #src/agpu_d3d12.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS OFF
)

# Public include paths
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>
)

if (VGPU_BACKEND_VULKAN)
    target_sources(${PROJECT_NAME} PRIVATE
        src/vk/vk.h
        src/vk/vk.c
        src/vk/vk_mem_alloc.h
        src/vk/vk_mem_alloc.cpp
        src/vgpu_vk.c
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE VGPU_VK_BACKEND)
    target_link_libraries(${PROJECT_NAME} PRIVATE vulkan)
endif ()

if (AGPU_BACKEND_D3D11 OR AGPU_BACKEND_D3D12)
    #target_link_libraries(agpu PRIVATE dxguid)
endif ()

if (AGPU_BACKEND_D3D12)
    #target_compile_definitions(agpu PRIVATE GPU_D3D12_BACKEND)
    #target_link_libraries(agpu PRIVATE D3D12MemoryAllocator)
endif ()

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
        /W3     # Set warning level 3
        /WX     # Treat all other warnings as errors
    )
endif ()

if (VGPU_INSTALL)
    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
