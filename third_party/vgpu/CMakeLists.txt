
# CMake version
cmake_minimum_required(VERSION 3.10)
project(vgpu)

set(VGPU_VERSION_MAJOR 0)
set(VGPU_VERSION_MINOR 1)
set(VGPU_VERSION_PATCH 0)
set(VGPU_VERSION ${VGPU_VERSION_MAJOR}.${VGPU_VERSION_MINOR}.${VGPU_VERSION_PATCH})

# Enable folders in IDE (VisualStudio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
	set(LOBJC "objc")
elseif(WIN32)
	# "vgpu.dll", not "libvgpu.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# third_party
set(VGPU_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# OpenGL headers
if (WIN32 OR ANDROID OR EMSCRIPTEN)
    add_library(opengl INTERFACE)
    target_sources(opengl INTERFACE "${VGPU_THIRD_PARTY_DIR}/khronos/KHR/khrplatform.h")
    target_include_directories(opengl INTERFACE "${VGPU_THIRD_PARTY_DIR}/khronos")
endif ()

# Vulkan headers
if (WIN32 OR ANDROID OR LINUX)
    add_library(vulkan INTERFACE)
    set (VULKAN_INCLUDE_DIR ${VGPU_THIRD_PARTY_DIR}/khronos/vulkan/include)
    target_sources (vulkan INTERFACE
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_icd.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_layer.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_platform.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vk_sdk_platform.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h
        ${VULKAN_INCLUDE_DIR}/vulkan/vulkan_core.h
    )

    target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})
    target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)
    target_compile_definitions(vulkan INTERFACE VK_ENABLE_BETA_EXTENSIONS)

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

    add_library(vma STATIC
        ${VGPU_THIRD_PARTY_DIR}/vma/vk_mem_alloc.cpp
        ${VGPU_THIRD_PARTY_DIR}/vma/vk_mem_alloc.h
    )

    target_include_directories(vma PUBLIC ${VGPU_THIRD_PARTY_DIR}/vma)
    target_link_libraries(vma PUBLIC vulkan)
    set_property(TARGET vma PROPERTY FOLDER "ThirdParty")
endif ()

add_library(vgpu
	# Public Headers
	include/vgpu.h
    # Internal Headers
	src/vgpu_driver.h
    # Source Files
	src/vgpu.cpp
    src/vgpu_d3d_common.h
    src/vgpu_driver_d3d11.cpp
    src/vgpu_driver_d3d12.cpp
    src/vgpu_driver_opengl.c
    src/vgpu_driver_vulkan.c
)

target_include_directories(vgpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(WIN32 OR WINDOWS_STORE)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D11)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D12)
endif ()

if (WIN32 OR ANDROID OR LINUX)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_VULKAN)
    target_link_libraries(vgpu PRIVATE vma)
endif ()

if (WIN32 OR ANDROID OR EMSCRIPTEN)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_OPENGL)
    target_link_libraries(vgpu PRIVATE opengl)
endif ()

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
        /W3
        /WX
    )
endif ()
