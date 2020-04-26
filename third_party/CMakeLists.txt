# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# GLFW
if(NOT (EMSCRIPTEN OR ANDROID))
    set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set (GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    #set (GLFW_USE_WAYLAND ${VGPU_USE_WAYLAND} CACHE BOOL "" FORCE)
    add_subdirectory(glfw)
    set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
endif ()

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")

add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")

if (ALIMER_OPENGL)
    # Header-only library for khrplatform.h
    add_library(alimer_khronos_platform INTERFACE)
    target_sources(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos/KHR/khrplatform.h")
    target_include_directories(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos")
endif()

# Vulkan headers
add_library(vulkan INTERFACE)
set (VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/khronos)
target_sources (vulkan INTERFACE 
    ${VULKAN_INCLUDE_DIR}/vulkan/vk_platform.h
    ${VULKAN_INCLUDE_DIR}/vulkan/vk_sdk_platform.h
    ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h
    ${VULKAN_INCLUDE_DIR}/vulkan/vulkan_core.h
)
target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})