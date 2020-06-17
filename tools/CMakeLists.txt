if (NOT ALIMER_BUILD_TOOLS AND NOT ALIMER_BUILD_EDITOR)
    return()
endif ()

if (ALIMER_BUILD_EDITOR)
    add_subdirectory(EditorFramework)
    add_subdirectory(Editor)
endif ()