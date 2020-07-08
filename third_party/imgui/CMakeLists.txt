
# imgui
set(IMGUI_FILES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imconfig.h
    imgui.h
    imgui_internal.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
)

set(IMGUI_IMPL_FILES
    imgui_impl_glfw.h
    imgui_impl_glfw.cpp
)

add_library(imgui STATIC ${IMGUI_FILES} ${IMGUI_IMPL_FILES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui PRIVATE glfw)
set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")
