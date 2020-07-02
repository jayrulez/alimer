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
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    option(VGPU_DRIVER_D3D12 "Include Direct3D12 backend" ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
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
    src/vgpu_driver_d3d12.cpp
)

target_include_directories(vgpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	#$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan/include>
)

if (VGPU_DRIVER_D3D12)
    target_sources(vgpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src/D3D12MemAlloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src/D3D12MemAlloc.natvis
    )

    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D12)
    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src>
    )
endif ()
