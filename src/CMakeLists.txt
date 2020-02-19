macro (define_engine_source_files)
    cmake_parse_arguments(DEFINE_SRC_FILES "" "" "" ${ARGN})
	foreach (path ${DEFINE_SRC_FILES_UNPARSED_ARGUMENTS})
		# Get header files
		file (GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.hpp ${path}/*.h ${path}/*.inl)

		list (APPEND HEADER_FILES ${_files})

		# Install them
        install (FILES ${_files} DESTINATION ${DEST_INCLUDE_DIR}/${path})

		# Get source files
		file (GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.c ${path}/*.cpp)
		list (APPEND SOURCE_FILES ${_files})
	endforeach ()
endmacro()

# Groups sources into subfolders.
macro(group_sources)
    file (GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/**)
    foreach (child ${children})
        if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
            string(REPLACE "/" "\\" groupname "${child}")
            file (GLOB files LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*)
            source_group(${groupname} FILES ${files})
        endif ()
    endforeach ()
endmacro()

define_engine_source_files(
    Core
    Application
    Graphics
)

if(WIN32)
    define_engine_source_files(
        Application/Windows
    )
endif ()

group_sources()

add_library(Alimer ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories (Alimer
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(Alimer
    EASTL
)

if(NOT (WIN32 OR EMSCRIPTEN OR ANDROID))
    target_link_libraries(Alimer
        glfw
    )
endif ()

if (WIN32)
    target_compile_definitions(Alimer PRIVATE _UNICODE)
    target_compile_definitions(Alimer PUBLIC _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)
    target_compile_options(Alimer PUBLIC $<$<CONFIG:DEBUG>:-D_SECURE_SCL_THROWS=0> $<$<CONFIG:DEBUG>:-D_SILENCE_DEPRECATION_OF_SECURE_SCL_THROWS>)
    target_compile_options(Alimer PRIVATE /fp:fast)
    target_compile_options(Alimer PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else()
    target_compile_options(Alimer PRIVATE -ffast-math $<$<PLATFORM_ID:Linux>:-fPIC>)
endif()

