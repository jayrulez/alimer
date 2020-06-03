if (WIN32 OR WINDOWS_STORE)
    add_subdirectory(Alimer.Direct3D11)
    add_subdirectory(Alimer.Direct3D12)
endif ()

if (WIN32)
    add_subdirectory(Alimer.Vulkan)
endif ()