
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

add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")
