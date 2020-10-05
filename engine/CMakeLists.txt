project(Alimer)

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

        if (MSVC)
            # Get natvis files
            file (GLOB NATVIS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.natvis)
		    list (APPEND SOURCE_FILES ${NATVIS_FILES})
        endif ()
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AlimerConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/AlimerConfig.h)
set(SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/AlimerConfig.h)

define_engine_source_files(
    Core
    Platform
    Math
    IO
    Assets
    Graphics
    Scene
)

if(ALIMER_D3D11 OR ALIMER_D3D12)
    define_engine_source_files(Graphics/D3D)
endif()

if(ALIMER_D3D11)
    define_engine_source_files(Graphics/D3D11)
endif()

if(ALIMER_D3D12)
    #define_engine_source_files(Graphics/D3D12)
endif()

if (ALIMER_ENABLE_VULKAN)
    #define_engine_source_files(Graphics/Vulkan)
endif()

if (ALIMER_IMGUI)
    define_engine_source_files(UI)
endif ()

if (ALIMER_PHYSICS)
    define_engine_source_files(Physics)
endif()

# Include platform files.
define_engine_source_files(Platform/${ALIMER_NATIVE_PLATFORM})

if(NOT (WINDOWS_STORE OR EMSCRIPTEN OR ANDROID))
    define_engine_source_files(Platform/glfw)
endif()

# Add root-level files
file (GLOB TOP_HEADER_FILES *.h)
list (APPEND HEADER_FILES ${TOP_HEADER_FILES})

group_sources()

if (ALIMER_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
    target_compile_definitions(${PROJECT_NAME} PUBLIC ALIMER_SHARED_LIBRARY)
else ()
    add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif ()
target_compile_definitions(${PROJECT_NAME} PRIVATE ALIMER_COMPILE)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    fmt
    stb
)

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

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast)
else()
  set(ALIMER_COMPILE_WARNINGS
      -Wall -Wextra -Wno-unused-parameter
      -Wextra-semi -Wnewline-eof -Wdeprecated -Wundef
      -Wgnu-conditional-omitted-operand
      -Wweak-vtables -Wnon-virtual-dtor -Wclass-varargs -Wimplicit-fallthrough
      -Wover-aligned
  )

  target_compile_options(${PROJECT_NAME} PRIVATE
        ${ALIMER_COMPILE_WARNINGS}
        $<$<CONFIG:Release>:${OPTIMIZATION_FLAGS}>
        $<$<PLATFORM_ID:Linux>:-fPIC>
  )
endif()

# Warnings as Errors Enabled"
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
endif()

# Compiler flags
if(EMSCRIPTEN)
elseif(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE log android native_app_glue)
elseif(APPLE)
elseif(WINDOWS_STORE)
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "${ALIMER_UWP_VERSION_MIN}")

    target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJECT_NAME} PUBLIC "/Zc:__cplusplus")
    # we need this when using /permissive
    target_compile_options(${PROJECT_NAME} PUBLIC "/Zc:twoPhase-")
elseif (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_NONSTDC_NO_WARNINGS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE _UNICODE)

    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4244 /MP")
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
elseif(UNIX)
endif ()

if(NOT (WINDOWS_STORE OR EMSCRIPTEN OR ANDROID))
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_BACKEND)
endif ()

if (ALIMER_IMGUI)
    target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
endif ()

# SDK installation
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
