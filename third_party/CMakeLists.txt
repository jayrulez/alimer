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

if (ALIMER_D3D12)
    # D3D12MemoryAllocator
    set(D3D12_MA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator")
    set(D3D12_MA_FILES
        "${D3D12_MA_DIR}/D3D12MemAlloc.cpp"
        "${D3D12_MA_DIR}/D3D12MemAlloc.h"
        "${D3D12_MA_DIR}/D3D12MemAlloc.natvis"
    )

    add_library(D3D12MemoryAllocator STATIC ${D3D12_MA_FILES})
    target_include_directories(D3D12MemoryAllocator PUBLIC ${D3D12_MA_DIR})
    set_property(TARGET D3D12MemoryAllocator PROPERTY FOLDER "ThirdParty")
endif ()

if (ALIMER_OPENGL)
    # Header-only library for khrplatform.h
    add_library(alimer_khronos_platform INTERFACE)
    target_sources(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos/KHR/khrplatform.h")
    target_include_directories(alimer_khronos_platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/khronos")
endif()

if (ALIMER_VULKAN)
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

     # vma
    add_library(vma INTERFACE)
    set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator)
    target_sources(vma INTERFACE 
        ${VMA_DIR}/vk_mem_alloc.h
        ${VMA_DIR}/vk_mem_alloc.natvis
    )
    target_include_directories(vma INTERFACE ${VMA_DIR})
    target_link_libraries(vma INTERFACE vulkan)

    # volk
    set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
    set(VOLK_FILES
        "${VOLK_DIR}/volk.c"
        "${VOLK_DIR}/volk.h"
    )

    add_library(volk STATIC ${VOLK_FILES})
    target_link_libraries(volk PUBLIC vulkan)
    target_include_directories(volk PUBLIC ${VOLK_DIR})
    set_property(TARGET volk PROPERTY FOLDER "ThirdParty")
endif()