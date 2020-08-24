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

define_engine_source_files(
  core
  io
  platform
)

if (WIN32)
    define_engine_source_files(platform/windows)
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


if(ALIMER_WARNINGS_AS_ERRORS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${TARGET_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${TARGET_NAME} PRIVATE /W3 /WX)
    endif()
endif()

if (WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "/wd4244 /MP")
  target_compile_definitions(${TARGET_NAME} PUBLIC -D_CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(${TARGET_NAME} PUBLIC -D_CRT_NONSTDC_NO_WARNINGS)
  target_compile_definitions(${TARGET_NAME} PUBLIC -D_UNICODE)
elseif(EMSCRIPTEN)
endif ()

if(NOT (EMSCRIPTEN OR ANDROID))
  #target_link_libraries(${TARGET_NAME} glfw)
endif ()

# SDK installation
install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
