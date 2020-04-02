set(VGPU_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VGPU_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vgpu)
set_property(TARGET vgpu PROPERTY FOLDER "ThirdParty")

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
