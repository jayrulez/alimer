# CMake version
cmake_minimum_required(VERSION 3.10)

# Project declaration
project (Alimer LANGUAGES C CXX)

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
    Core
    Diagnostics
    Math
    Graphics
    Input
    Games
)

# Sources per backend/platform
if(NOT (EMSCRIPTEN OR ANDROID))
    define_engine_source_files(
        Games/GLFW
    )
endif ()

# Graphics sources
if (ALIMER_GRAPHICS_VULKAN)
    define_engine_source_files(
        Graphics/Vulkan
    )
endif()

if (ALIMER_GRAPHICS_D3D12)
    define_engine_source_files(
        Graphics/D3D12
    )
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

target_link_libraries(${PROJECT_NAME}
    agpu
    EASTL
)

if(NOT (EMSCRIPTEN OR ANDROID))
    target_link_libraries(${PROJECT_NAME}
        glfw
    )
endif ()

if (ALIMER_GRAPHICS_VULKAN)
    target_link_libraries(${PROJECT_NAME}
        volk
        vma
    )
endif ()

if (ALIMER_GRAPHICS_D3D12)
    target_link_libraries(${PROJECT_NAME}
        D3D12MemoryAllocator
    )
endif ()

if (CLANG_CL OR MSVC)
     target_compile_options(${PROJECT_NAME} PUBLIC /MP)
endif ()

if (WIN32)
    # Enable unicode strings
    target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE)

	target_compile_options(${PROJECT_NAME} PUBLIC
        /W3         # Set warning level 3
        /WX         # Treat all other warnings as errors
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -ffast-math $<$<PLATFORM_ID:Linux>:-fPIC>)
endif()

# SDK installation
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
