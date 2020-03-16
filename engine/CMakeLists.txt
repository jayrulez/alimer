# CMake version
cmake_minimum_required(VERSION 3.10)

# Project declaration
project (alimer LANGUAGES C CXX)

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
define_engine_source_files(
    .
    Core
    Diagnostics
    math
    window
    graphics
    Input
    Games
)

# Sources per backend/platform
if(NOT (EMSCRIPTEN OR ANDROID))
    define_engine_source_files(
        window/glfw
        Games/GLFW
    )
endif ()

if (ALIMER_D3D11)
    define_engine_source_files(graphics/d3d_common)
    define_engine_source_files(graphics/d3d11)
endif()
if (ALIMER_D3D12)
    define_engine_source_files(graphics/d3d_common)
    #define_engine_source_files(graphics/d3d12)
endif()
if (ALIMER_VULKAN)
    define_engine_source_files(graphics/vulkan)
endif()

group_sources()

if (ALIMER_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
    target_compile_definitions(${PROJECT_NAME} PUBLIC ALIMER_EXPORTS)
else ()
    add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT (EMSCRIPTEN OR ANDROID))
    target_compile_definitions(${PROJECT_NAME} PRIVATE ALIMER_GLFW)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
    )
endif ()

if (ALIMER_D3D12)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        D3D12MemoryAllocator
    )
endif()

if (ALIMER_VULKAN)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        volk
        vma
    )
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
    set(ALIMER_WARNINGS /W3)
else()
    set(ALIMER_WARNINGS
        -Wall -Wextra -Wno-unused-parameter
        -Wextra-semi -Wnewline-eof -Wdeprecated -Wundef
        -Wgnu-conditional-omitted-operand
        -Wweak-vtables -Wnon-virtual-dtor -Wclass-varargs -Wimplicit-fallthrough
        -Wover-aligned
    )
endif()


target_compile_options(${PROJECT_NAME} PRIVATE
        ${ALIMER_WARNINGS}
        $<$<CONFIG:Release>:${OPTIMIZATION_FLAGS}>
        $<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Release>>:${DARWIN_OPTIMIZATION_FLAGS}>
        $<$<PLATFORM_ID:Linux>:-fPIC>
)

if (WIN32)
    # Enable unicode strings
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE)
endif()

# SDK installation
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
