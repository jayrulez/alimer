
set(SOURCE_FILES
    D3D12MemAlloc.cpp
    D3D12MemAlloc.h
    D3D12MemAlloc.natvis
)

add_library(D3D12MemAlloc STATIC ${SOURCE_FILES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET D3D12MemAlloc PROPERTY FOLDER "ThirdParty")
