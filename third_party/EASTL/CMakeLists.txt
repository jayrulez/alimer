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
    EASTL_EASTDC_VSNPRINTF=0
    EASTDC_THREADING_SUPPORTED=0
    EA_USE_CPP11_CONCURRENCY=1
    EASTDC_GLOBALPTR_SUPPORT_ENABLED=0
)

target_compile_definitions(EASTL PRIVATE 
    _CHAR16T
)

if (MSVC)
    target_compile_definitions(EASTL PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif (UNIX OR EMSCRIPTEN)
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

if (BUILD_SHARED_LIBS)
    target_compile_definitions(EASTL PUBLIC -DEASTL_DLL -DEASTDC_DLL PRIVATE -DEASTL_EXPORTS -DEASTDC_EXPORTS)
endif ()

# Install
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
