# CMake version
cmake_minimum_required(VERSION 3.10)
project(vgpu)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Version
set(VGPU_VERSION_MAJOR 0)
set(VGPU_VERSION_MINOR 1)
set(VGPU_VERSION_PATCH 0)
set(VGPU_VERSION ${VGPU_VERSION_MAJOR}.${VGPU_VERSION_MINOR}.${VGPU_VERSION_PATCH})

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
	set(LOBJC "objc")
elseif(WIN32)
	# "agpu.dll", not "libagpu.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Library
add_library(vgpu
	include/vgpu.h
	src/vgpu_driver.h
	src/vgpu.c
    src/vgpu_driver_vk.c
    src/vgpu_driver_d3d_common.h
    #src/agpu_driver_d3d11.cpp
    src/vgpu_driver_d3d12.cpp
    #src/agpu_driver_opengl.cpp
    src/stb_ds.h
)

target_include_directories(vgpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
)

# Soname
set_target_properties(vgpu PROPERTIES OUTPUT_NAME "vgpu"
	VERSION ${VGPU_VERSION}
	SOVERSION ${VGPU_VERSION_MAJOR}
)

# Vulkan
if (WIN32 OR ANDROID OR LINUX)
    target_sources(vgpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/vma/src/vk_mem_alloc.cpp
    )
    target_compile_definitions(vgpu PRIVATE
        VK_NO_PROTOTYPES
        VGPU_DRIVER_VULKAN
        VK_ENABLE_BETA_EXTENSIONS
    )

    if(ANDROID)
        target_compile_definitions(vgpu PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    elseif(WIN32)
        target_compile_definitions(vgpu PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif(APPLE)
        target_compile_definitions(vgpu PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    elseif(UNIX)
        # See whether X11 is available. If not, fall back to direct-to-display mode.
        find_package(X11 QUIET)
        if (X11_FOUND)
            target_compile_definitions(vgpu PRIVATE INTERFACE VK_USE_PLATFORM_XCB_KHR)
        else()
            target_compile_definitions(vgpu PRIVATE VK_USE_PLATFORM_DISPLAY_KHR)
        endif()
    endif()

    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan/include>
        $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/external/vma/src>
    )
endif ()

# D3D12 and D3D11
if(WIN32 OR WINDOWS_STORE)
    target_sources(vgpu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/d3d12ma/src/D3D12MemAlloc.cpp
    )

    target_include_directories(vgpu PRIVATE
        $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/external/d3d12ma/src>
    )

    target_compile_definitions(vgpu PRIVATE NOMINMAX)
    #target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D11)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D12)
endif ()

if (MSVC)
  # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
    /W3
    /WX
  )
else()
  set_property(TARGET vgpu PROPERTY COMPILE_FLAGS "-Wall -Wno-strict-aliasing -pedantic")
endif ()
