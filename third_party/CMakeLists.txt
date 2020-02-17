if(NOT EMSCRIPTEN)
    add_subdirectory(SDL)
    set_property(TARGET SDL2 PROPERTY FOLDER "ThirdParty")
endif ()