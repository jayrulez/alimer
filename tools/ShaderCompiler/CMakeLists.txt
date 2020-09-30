
# Library
add_library(ShaderCompilerLib
	include/ShaderCompiler.h
	src/ShaderCompiler.cpp
)

target_include_directories(ShaderCompilerLib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Executable
add_executable(ShaderCompiler src/main.cpp)

target_link_libraries(ShaderCompiler PRIVATE
    ShaderCompilerLib
    cxxopts
)

if (MSVC)
    # Set warning level 3 and treat all other warnings as errors
	target_compile_options(ShaderCompiler PRIVATE /W3 /WX)
else()
    set_property(TARGET ShaderCompiler PROPERTY COMPILE_FLAGS "-Wall -Wno-strict-aliasing -pedantic")
endif ()

set_property(TARGET ShaderCompilerLib PROPERTY FOLDER "Tools")
set_property(TARGET ShaderCompiler PROPERTY FOLDER "Tools")
