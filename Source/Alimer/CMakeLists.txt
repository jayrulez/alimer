set(TARGET_NAME Alimer)

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set(SOURCE_FILES alimer.natvis)

define_engine_source_files(
    foundation
    Core
    Math
    os
    Graphics
    Input
    Application
)

if (ALIMER_GRAPHICS_D3D12)
    #define_engine_source_files(graphics/d3d)
    #define_engine_source_files(graphics/d3d12)
endif ()

group_sources()

if (ALIMER_BUILD_SHARED)
    add_library(${TARGET_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
    target_compile_definitions(${TARGET_NAME} PUBLIC ALIMER_SHARED_LIBRARY)
else ()
    add_library(${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif ()

target_compile_definitions(${TARGET_NAME} PRIVATE ALIMER_COMPILE)

target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${TARGET_NAME} PUBLIC
    stb
    imgui
)

if (ALIMER_GRAPHICS_VULKAN)
    target_link_libraries(${TARGET_NAME} PRIVATE vulkan)
    target_compile_definitions(${TARGET_NAME} PRIVATE GPU_DRIVER_VULKAN)
endif ()

# Link platform specific libraries
if(ANDROID)
    target_link_libraries(${TARGET_NAME} PRIVATE log android native_app_glue)
else()
    if(NOT (EMSCRIPTEN OR DIRECT_TO_DISPLAY))
        target_compile_definitions(${TARGET_NAME} PRIVATE GLFW_BACKEND)
        target_link_libraries(${TARGET_NAME} PRIVATE glfw)
    endif()
endif()

if (MSVC)
    set(OPTIMIZATION_FLAGS
        /fp:fast
    )
elseif(EMSCRIPTEN)
    # Avoid strict-vtable-pointers here, it is broken in WebAssembly.
    set(OPTIMIZATION_FLAGS -fvisibility=hidden -fvisibility-inlines-hidden)
else()
    set(OPTIMIZATION_FLAGS
        -ffast-math
        -ffp-contract=fast
        # TODO: aggressive vectorization is currently broken on Android
        #        -fslp-vectorize-aggressive
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -fstrict-vtable-pointers
    )
endif()

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
    set(ALIMER_WARNINGS /W3 /WX)
else()
    set(ALIMER_WARNINGS
        -Wall -Wextra -Wno-unused-parameter
        -Wextra-semi -Wnewline-eof -Wdeprecated -Wundef
        -Wgnu-conditional-omitted-operand
        -Wweak-vtables -Wnon-virtual-dtor -Wclass-varargs -Wimplicit-fallthrough
        -Wover-aligned
    )
endif()

target_compile_options(${TARGET_NAME} PRIVATE
        ${ALIMER_WARNINGS}
        $<$<CONFIG:Release>:${OPTIMIZATION_FLAGS}>
        $<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Release>>:${DARWIN_OPTIMIZATION_FLAGS}>
        $<$<PLATFORM_ID:Linux>:-fPIC>
)

if (WIN32)
    # Enable unicode strings
    target_compile_definitions(${TARGET_NAME} PRIVATE _UNICODE)
    target_compile_definitions(${TARGET_NAME} PRIVATE GPU_DRIVER_D3D11)
endif()

# SDK installation
install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)

