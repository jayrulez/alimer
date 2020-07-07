# CMake version
cmake_minimum_required(VERSION 3.10)
project(vgpu)

# Enable folders in IDE (VisualStudio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(VGPU_VERSION_MAJOR 0)
set(VGPU_VERSION_MINOR 1)
set(VGPU_VERSION_PATCH 0)
set(VGPU_VERSION ${VGPU_VERSION_MAJOR}.${VGPU_VERSION_MINOR}.${VGPU_VERSION_PATCH})

# Options
option(VGPU_INSTALL "Generate installation target" ON)
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Driver Options
if (WIN32)
    option(VGPU_DRIVER_D3D12 "Include Direct3D12 backend" ON)
    option(VGPU_DRIVER_VULKAN "Include Vulkan backend" ON)
elseif (WINDOWS_STORE)
    option(VGPU_DRIVER_D3D12 "Include Direct3D12 backend" ON)
else()
endif ()

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
	set(LOBJC "objc")
elseif(WIN32)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Source lists
add_library(vgpu
	# Public Headers
	include/vgpu.h
    # Internal Headers
	src/vgpu_driver.h
    src/vgpu_d3d_common.h
    # Source Files
	src/vgpu.cpp
    src/vgpu_driver_d3d11.cpp
    src/vgpu_driver_d3d12.cpp
    src/vgpu_driver_vulkan.cpp
)

target_include_directories(vgpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Direct3D11
if (WIN32 OR WINDOWS_STORE)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D11)
endif ()

# Direct3D12
if (VGPU_DRIVER_D3D12)
    target_sources(vgpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/D3D12MemAlloc.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/D3D12MemAlloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/D3D12MemAlloc.natvis
    )

    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D12)
    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator>
    )
endif ()

# Vulkan
if (VGPU_DRIVER_VULKAN)
    target_sources(vgpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma/vk_mem_alloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma/vk_mem_alloc.natvis
    )

    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_VULKAN)
    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/vma>
    )
endif ()

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
        /W3
        /WX
    )
endif ()
