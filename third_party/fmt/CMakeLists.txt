include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(_strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
else ()
  check_symbol_exists(strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
endif ()

file(GLOB FMT_HEADERS include/fmt/*.h)
set(FMT_SOURCES src/format.cc src/os.cc)
if (HAVE_OPEN)
  set(FMT_HEADERS ${FMT_HEADERS} fmt/posix.h)
  set(FMT_SOURCES ${FMT_SOURCES} fmt/posix.cc)
endif ()

add_library(fmt ${FMT_SOURCES} ${FMT_HEADERS} README.rst ChangeLog.rst)

if (HAVE_STRTOD_L)
    target_compile_definitions(fmt PUBLIC FMT_LOCALE)
endif ()

target_compile_definitions(fmt PUBLIC FMT_USE_RVALUE_REFERENCES=1 FMT_USE_VARIADIC_TEMPLATES=1)
target_include_directories(fmt PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
)

set_property(TARGET fmt PROPERTY FOLDER "ThirdParty")
