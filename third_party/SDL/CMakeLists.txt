cmake_minimum_required(VERSION 3.10)
project(SDL2 C)

set(TARGET          SDL2)
set(SRC_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADERS_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/include)

# General source files
file(GLOB HEADER_FILES
    ${HEADERS_DIR}/*.h
)

file(GLOB SOURCE_FILES
    ${SRC_DIR}/*.c
    ${SRC_DIR}/atomic/*.c
    ${SRC_DIR}/audio/*.c
    ${SRC_DIR}/audio/disk/*.c
    ${SRC_DIR}/audio/dsp/*.c
    ${SRC_DIR}/audio/dummy/*.c
    ${SRC_DIR}/cpuinfo/*.c
    ${SRC_DIR}/dynapi/*.c
    ${SRC_DIR}/events/*.c
    ${SRC_DIR}/file/*.c
    ${SRC_DIR}/joystick/*.c
    ${SRC_DIR}/joystick/steam/*.c
    ${SRC_DIR}/haptic/*.c
    ${SRC_DIR}/libm/*.c
    ${SRC_DIR}/power/*.c
    ${SRC_DIR}/render/*.c
    ${SRC_DIR}/render/*/*.c
    ${SRC_DIR}/stdlib/*.c
    ${SRC_DIR}/thread/*.c
    ${SRC_DIR}/timer/*.c
    ${SRC_DIR}/video/*.c
    ${SRC_DIR}/video/yuv2rgb/*.c
    ${SRC_DIR}/video/dummy/*.c
)

add_library(${TARGET} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories (${TARGET} 
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
