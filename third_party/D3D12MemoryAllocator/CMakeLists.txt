
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12MemAlloc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12MemAlloc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12MemAlloc.natvis
)

add_library(D3D12MemAlloc STATIC ${SOURCE_FILES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_property(TARGET D3D12MemAlloc PROPERTY FOLDER "ThirdParty")
