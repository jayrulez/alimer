# Third party deps
set(AGPU_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
add_library(khronos_platform INTERFACE)
target_sources(khronos_platform INTERFACE 
    "${AGPU_THIRD_PARTY_DIR}/khronos/KHR/khrplatform.h"
    "${AGPU_THIRD_PARTY_DIR}/khronos/GL/glcorearb.h"
    "${AGPU_THIRD_PARTY_DIR}/khronos/GL/glext.h"
)
target_include_directories(khronos_platform INTERFACE "${AGPU_THIRD_PARTY_DIR}/khronos")

# agpu
add_library(agpu
	include/agpu.h
  src/agpu_driver.h
  src/agpu.c
  src/agpu_driver_gl.c
)

target_include_directories(agpu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if(WIN32)
  target_compile_definitions(agpu PRIVATE AGPU_DRIVER_OPENGL)

  target_link_libraries(agpu PRIVATE
      khronos_platform
  )
endif()
