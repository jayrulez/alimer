
# CMake version
cmake_minimum_required(VERSION 3.10)
project(agpu)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Version
set(AGPU_VERSION_MAJOR 0)
set(AGPU_VERSION_MINOR 1)
set(AGPU_VERSION_PATCH 0)
set(AGPU_VERSION ${AGPU_VERSION_MAJOR}.${AGPU_VERSION_MINOR}.${AGPU_VERSION_PATCH})

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

if(NOT EMSCRIPTEN)
  # glad
  set(GLAD_DIR "${AGPU_THIRD_PARTY_DIR}/glad")
  set(GLAD_FILES
    "${GLAD_DIR}/include/KHR/khrplatform.h"
    "${GLAD_DIR}/include/glad/glad.h"
    "${GLAD_DIR}/src/glad.c"
  )

  add_library(glad STATIC ${GLAD_FILES})
  target_include_directories(glad PUBLIC "${GLAD_DIR}/include")
  set_property(TARGET glad PROPERTY FOLDER "ThirdParty")
endif ()

add_library(agpu
  # Public Headers
	include/agpu.h
    # Internal Headers
	src/agpu_driver.h
    # Source Files
	src/agpu.c
  src/agpu_driver_d3d_common.h
  src/agpu_driver_d3d11.c
  src/agpu_driver_opengl.c
)

target_include_directories(agpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Soname
set_target_properties(agpu PROPERTIES OUTPUT_NAME "agpu"
	VERSION ${AGPU_VERSION}
	SOVERSION ${AGPU_VERSION_MAJOR}
)

if(WIN32 OR WINDOWS_STORE)
    target_compile_definitions(agpu PRIVATE AGPU_DRIVER_D3D11)
    target_compile_definitions(agpu PRIVATE AGPU_DRIVER_D3D12)
endif ()

if (WIN32 OR ANDROID OR LINUX)
    target_compile_definitions(agpu PRIVATE AGPU_DRIVER_VULKAN)
    #target_link_libraries(agpu PRIVATE alimer_vulkan_headers)
endif ()

if (WIN32 OR ANDROID OR EMSCRIPTEN)
    target_compile_definitions(agpu PRIVATE AGPU_DRIVER_OPENGL)

    if(EMSCRIPTEN)
      target_compile_definitions(agpu PRIVATE -DAGPU_WEBGL)
    else()
      target_link_libraries(agpu PRIVATE glad)
    endif ()
endif ()

if (MSVC)
  # Set warning level 3 and treat all other warnings as errors
	target_compile_options(agpu PRIVATE
    /W3
    /WX
  )
else()
  set_property(TARGET agpu PROPERTY COMPILE_FLAGS "-Wall -Wno-strict-aliasing -pedantic")
endif ()
