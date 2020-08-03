if (NOT ALIMER_BUILD_EDITOR)
    return()
endif ()

set(TARGET_NAME EditorFramework)
file (GLOB_RECURSE SOURCE_FILES *.cpp *.h *.hpp)

add_library(${TARGET_NAME} ${SOURCE_FILES})
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${TARGET_NAME} PUBLIC
    alimer
)

set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "Editor")
