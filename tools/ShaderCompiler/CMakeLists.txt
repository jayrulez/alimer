
# CMake version
cmake_minimum_required(VERSION 3.10)
project(ShaderCompiler)

# Library
add_library(ShaderCompilerLib
	include/ShaderCompiler.h
	src/ShaderCompiler.cpp
)

target_include_directories(ShaderCompilerLib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Library
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ShaderCompilerLib
    cxxopts
)

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else()
    set_property(TARGET ${PROJECT_NAME} PROPERTY COMPILE_FLAGS "-Wall -Wno-strict-aliasing -pedantic")
endif ()

set_property(TARGET ShaderCompilerLib PROPERTY FOLDER "Tools")
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Tools")
