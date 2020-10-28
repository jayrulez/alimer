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

# dxc
add_library(dxc INTERFACE)
set(DXC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dxc/include)
target_sources(dxc INTERFACE 
    ${DXC_INCLUDE_DIR}/dxcapi.h
    ${DXC_INCLUDE_DIR}/d3d12shader.h
    ${DXC_INCLUDE_DIR}/WinAdapter.h
    )
target_include_directories(dxc INTERFACE ${DXC_INCLUDE_DIR})

if(WIN32)
    set(dxil_name "dxil.dll")
    set(dxcompiler_name "dxcompiler.dll")
else()
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(dxcompiler_name "libdxcompiler.dylib")
    else()
        set(dxcompiler_name "libdxcompiler.so")
    endif()
endif()

if (dxil_name)
    add_custom_target(CopyDxcompiler ALL
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/${dxil_name}" ${ALIMER_BIN_DIR}/${dxil_name}
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/${dxcompiler_name}" ${ALIMER_BIN_DIR}/${dxcompiler_name}
        COMMENT "Copying dxil and dxcompiler binaries..."
    )
else()
    set(dxcompiler_source_name "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/${dxcompiler_name}")
    add_custom_target(CopyDxcompiler ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        ${dxcompiler_source_name}
        ${ALIMER_BIN_DIR}/${dxcompiler_name}
        COMMENT "Copying dxil and dxcompiler binaries..."
    )
endif ()

set_target_properties(CopyDxcompiler PROPERTIES FOLDER "ThirdParty")

if (ALIMER_VULKAN)
    # vulkan
    add_library(vulkan INTERFACE)
    set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
    target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
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
endif()

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
