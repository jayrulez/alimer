cmake_minimum_required(VERSION 3.10)
include(alimer_macros)

# Compiler check
set(MIN_CLANG_VERSION "6.0")

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    if (CMAKE_C_COMPILER_VERSION VERSION_LESS MIN_CLANG_VERSION)
        message(FATAL_ERROR "Detected C compiler Clang ${CMAKE_C_COMPILER_VERSION} < ${MIN_CLANG_VERSION}")
    endif()
elseif (NOT MSVC)
    message(FATAL_ERROR "Detected C compiler ${CMAKE_C_COMPILER_ID} is unsupported")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_CLANG_VERSION)
        message(FATAL_ERROR "Detected CXX compiler Clang ${CMAKE_CXX_COMPILER_VERSION} < ${MIN_CLANG_VERSION}")
    endif()
elseif (NOT MSVC)
    message(FATAL_ERROR "Detected CXX compiler ${CMAKE_CXX_COMPILER_ID} is unsupported")
endif()

# Detect use of the clang-cl.exe frontend, which does not support all of clangs normal options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(CLANG_CL ON CACHE BOOL "" FORCE)
    endif()
endif()

# Define standard configurations
if( CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_CONFIGURATION_TYPES MATCHES "Debug;Dev;Release" )
    set(CMAKE_CONFIGURATION_TYPES "Debug;Dev;Release" CACHE STRING "List of supported configurations." FORCE)
endif ()

# Initialize the development configuration using release configuration
set(CMAKE_C_FLAGS_DEV "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEV "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_STATIC_LINKER_FLAGS_DEV "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_DEV "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_MODULE_LINKER_FLAGS_DEV "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_DEV "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# Set global defines
add_compile_options(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
add_compile_options($<$<CONFIG:DEBUG>:-D_SECURE_SCL_THROWS=0> $<$<CONFIG:DEBUG>:-D_SILENCE_DEPRECATION_OF_SECURE_SCL_THROWS>)
add_compile_options(-D_HAS_ITERATOR_DEBUGGING=$<CONFIG:DEBUG> -D_SECURE_SCL=$<CONFIG:DEBUG>)
add_compile_options(-D_HAS_EXCEPTIONS=0)

if (WIN32 OR WINDOWS_STORE)
    # Disable C++ exceptions
    replace_compile_flags("/EHsc" "")

    # Select static/dynamic runtime library
	if (WIN32)
        replace_compile_flags("/MDd" "/MTd" Debug)
        replace_compile_flags("/MD" "/MT" Dev Release)
	elseif (WINDOWS_STORE)
        replace_compile_flags("/MTd" "/MDd" Debug)
        replace_compile_flags("/MT" "/MD" Dev Release)
	endif()

    # Use security checks only in debug
	if ( WINDOWS_STORE)
        add_compile_options($<$<CONFIG:DEBUG>:/sdl> $<$<NOT:$<CONFIG:DEBUG>>:/sdl->)
	else()
		add_compile_options($<$<CONFIG:DEBUG>:/GS> $<$<NOT:$<CONFIG:DEBUG>>:/GS->)
	endif()

    # Enable function-level linking
	add_compile_options(/Gy)

    # Use fast floating point model
	add_compile_options(/fp:fast)

	# Disable run-time type information (RTTI)
	replace_compile_flags("/GR" "/GR-")

    # Enable multi-processor compilation for Visual Studio 2012 and above
	if (CLANG_CL OR MSVC)
		add_compile_options(/MP)
	endif()
endif()