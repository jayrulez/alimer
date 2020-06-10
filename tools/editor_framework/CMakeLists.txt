if (NOT ALIMER_BUILD_EDITOR)
    return()
endif ()

set(TARGET_NAME editor_framework)
file (GLOB_RECURSE SOURCE_FILES *.cpp *.h *.hpp)

add_library(${TARGET_NAME} ${SOURCE_FILES})
target_link_libraries(${TARGET_NAME} PUBLIC
    imgui
    Alimer
)
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "Editor")
