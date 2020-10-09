# vgpu
add_subdirectory(vgpu)
set_property(TARGET vgpu PROPERTY FOLDER "ThirdParty")

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

add_subdirectory(fmt)

# spirv-cross
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "")
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "")
set(SPIRV_CROSS_CLI OFF CACHE BOOL "")
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "")
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "")
add_subdirectory(spirv-cross EXCLUDE_FROM_ALL)
set_property(TARGET spirv-cross-core PROPERTY FOLDER "ThirdParty")
set_property(TARGET spirv-cross-glsl PROPERTY FOLDER "ThirdParty")
set_property(TARGET spirv-cross-hlsl PROPERTY FOLDER "ThirdParty")
set_property(TARGET spirv-cross-msl PROPERTY FOLDER "ThirdParty")
set_property(TARGET spirv-cross-util PROPERTY FOLDER "ThirdParty")

if (ALIMER_IMGUI)
    add_subdirectory(imgui)
endif ()

# GLFW
if(NOT (WINDOWS_STORE OR EMSCRIPTEN OR ANDROID))
    set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set (GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "")
    #set (GLFW_USE_WAYLAND ${VGPU_USE_WAYLAND} CACHE BOOL "" FORCE)
    add_subdirectory(glfw)
    set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
endif ()

# cxxopts
add_library(cxxopts INTERFACE)
set(CXXOPTS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts/include)
target_sources(cxxopts INTERFACE ${CXXOPTS_INCLUDE_DIR}/cxxopts.hpp)
target_include_directories(cxxopts INTERFACE ${CXXOPTS_INCLUDE_DIR})
