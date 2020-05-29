
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

add_library(vgpu
	# Public Headers
	include/vgpu.h
    # Internal Headers
	src/vgpu_driver.h
    # Source Files
	src/vgpu.c
    src/vgpu_d3d_common.h
    src/vgpu_driver_d3d11.c
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
endif ()

if (WIN32 OR ANDROID OR EMSCRIPTEN)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_OPENGL)
endif ()

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(vgpu PRIVATE
        /W3
        /WX
    )
endif ()
