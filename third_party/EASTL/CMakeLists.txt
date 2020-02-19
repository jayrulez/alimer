#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(EASTL CXX)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
file(GLOB EASTL_SOURCES
	include/EASTL/*.h
	include/EASTL/internal/*.h
	include/EASTL/bonus/*.h
	source/*.cpp
)
add_library(EASTL ${EASTL_SOURCES} doc/EASTL.natvis)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
target_compile_definitions(EASTL PUBLIC  
	EASTL_OPENSOURCE=1
	EASTL_RTTI_ENABLED=0
)

target_compile_definitions(EASTL PRIVATE 
    _CHAR16T
)

if (MSVC)
    target_compile_definitions(EASTL PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif (UNIX)
    target_compile_options(EASTL PUBLIC -Wno-literal-suffix)
endif ()

#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(EASTL PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

#-------------------------------------------------------------------------------------------
# Dependencies
#-------------------------------------------------------------------------------------------
target_link_libraries(EASTL EABase)

set_property(TARGET EASTL PROPERTY FOLDER "third_party")

# Install
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
