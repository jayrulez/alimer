# add_subdirectory(vgpu)
# set_property(TARGET vgpu PROPERTY FOLDER "ThirdParty")

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

# Header-only library for khrplatform.h
add_library(alimer_khronos_platform INTERFACE)
target_sources(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos/KHR/khrplatform.h")
target_include_directories(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos")

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