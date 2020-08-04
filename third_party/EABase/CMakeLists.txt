#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(EABase CXX)

#-------------------------------------------------------------------------------------------
# Header only library 
#-------------------------------------------------------------------------------------------
add_library(EABase INTERFACE) 

# Defines
target_compile_definitions(EABase INTERFACE 
    _CHAR16T
)

#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(EABase INTERFACE include/Common)

# Install
install (DIRECTORY include/Common/EABase DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR} FILES_MATCHING PATTERN *.h)