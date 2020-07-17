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
    set(VGPU_DRIVER_D3D11 ON CACHE BOOL "Include Direct3D11 backend")
    #set(VGPU_DRIVER_D3D12 ON CACHE BOOL "Include Direct3D12 backend")
    #option(VGPU_DRIVER_VULKAN "Include Vulkan backend" ON)
elseif (WINDOWS_STORE)
    set(VGPU_DRIVER_D3D11 ON CACHE INTERNAL "Include Direct3D11 backend" FORCE)
    #set(VGPU_DRIVER_D3D12 ON CACHE BOOL "Include Direct3D12 backend")
    #option(VGPU_DRIVER_D3D12 "Include Direct3D12 backend" ON)
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

# Third party
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

if (VGPU_DRIVER_VULKAN AND TODO_VK)
    # vulkan
    add_library(vulkan INTERFACE)
    set(VULKAN_INCLUDE_DIR ${THIRD_PARTY_DIR}/vulkan/include)
    target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
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

     # vma
    add_library(vma INTERFACE)
    set(VMA_DIR ${THIRD_PARTY_DIR}/vma)
    target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
    target_include_directories(vma INTERFACE ${VMA_DIR})
    target_link_libraries(vma INTERFACE vulkan)

    # volk
    set(VOLK_DIR "${THIRD_PARTY_DIR}/volk")
    set(VOLK_FILES
        "${VOLK_DIR}/volk.c"
        "${VOLK_DIR}/volk.h")

    add_library(volk STATIC ${VOLK_FILES})
    target_link_libraries(volk PUBLIC vulkan)
    target_include_directories(volk PUBLIC ${VOLK_DIR})
    set_property(TARGET volk PROPERTY FOLDER "ThirdParty")
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
    src/vgpu_driver_opengl.cpp
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
    #target_sources(vgpu PRIVATE
    #    ${THIRD_PARTY_DIR}/D3D12MemoryAllocator/D3D12MemAlloc.cpp
    #    ${THIRD_PARTY_DIR}/D3D12MemoryAllocator/D3D12MemAlloc.h
    #    ${THIRD_PARTY_DIR}/D3D12MemoryAllocator/D3D12MemAlloc.natvis
    #)

    #target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_D3D12)
    #target_include_directories(vgpu PRIVATE
    #    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator>
    #)
endif ()

# Vulkan
if (VGPU_DRIVER_VULKAN)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_VULKAN)
     target_link_libraries(vgpu PRIVATE
        volk
        vma
    )
endif ()

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
        /W3
        /WX
    )
endif ()
