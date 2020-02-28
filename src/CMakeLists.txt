# Engine
add_subdirectory(Alimer)

if (ALIMER_BUILD_TOOLS OR ALIMER_BUILD_EDITOR)
    add_subdirectory(Tools)
endif ()