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

# Third party deps
set(AGPU_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Library
add_library(vgpu
	include/vgpu.h
	src/vgpu_driver.h
	src/vgpu.cpp
    src/vgpu_driver_vk.cpp
    #src/agpu_driver_d3d_common.h
    #src/agpu_driver_d3d11.cpp
    #src/agpu_driver_d3d12.cpp
    #src/agpu_driver_opengl.cpp
    src/vgpu_test.c
)

target_include_directories(vgpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Soname
set_target_properties(vgpu PROPERTIES OUTPUT_NAME "vgpu"
	VERSION ${VGPU_VERSION}
	SOVERSION ${VGPU_VERSION_MAJOR}
)

if (WIN32 OR ANDROID OR LINUX)
    target_compile_definitions(vgpu PRIVATE VGPU_DRIVER_VULKAN)
    #target_link_libraries(agpu PRIVATE alimer_vulkan_headers)
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
