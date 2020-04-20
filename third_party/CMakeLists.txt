# vgpu
set(VGPU_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vgpu)
set_property(TARGET vgpu PROPERTY FOLDER "ThirdParty")

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