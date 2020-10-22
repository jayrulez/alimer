# Replace compilation flags, configuration type is optional
macro(replace_compile_flags pSearch pReplace)
	set(MacroArgs "${ARGN}")
	if( NOT MacroArgs )
		string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		foreach(MacroArg IN LISTS MacroArgs)
			if( MacroArg STREQUAL "Debug" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			elseif( MacroArg STREQUAL "Dev" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_C_FLAGS_DEV "${CMAKE_C_FLAGS_DEV}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_CXX_FLAGS_DEV "${CMAKE_CXX_FLAGS_DEV}")
			elseif( MacroArg STREQUAL "Release" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			else()
				message(FATAL_ERROR "Unknown configuration, cannot replace compile flags!")
			endif()
		endforeach()
	endif()
endmacro()

# Add exe linker flags, configuration type is optional
macro(add_exe_linker_flags pFlags)
	set(MacroArgs "${ARGN}")
	if( NOT MacroArgs )
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${pFlags}")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${pFlags}")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${pFlags}")
	else()
		foreach(MacroArg IN LISTS MacroArgs)
			if( MacroArg STREQUAL "debug" )
				set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${pFlags}")
				set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${pFlags}")
				set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} ${pFlags}")
			elseif( MacroArg STREQUAL "dev" )
				set(CMAKE_EXE_LINKER_FLAGS_DEV "${CMAKE_EXE_LINKER_FLAGS_DEV} ${pFlags}")
				set(CMAKE_SHARED_LINKER_FLAGS_DEV "${CMAKE_SHARED_LINKER_FLAGS_DEV} ${pFlags}")
				set(CMAKE_MODULE_LINKER_FLAGS_DEV "${CMAKE_MODULE_LINKER_FLAGS_DEV} ${pFlags}")
			elseif( MacroArg STREQUAL "release" )
				set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${pFlags}")
				set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${pFlags}")
				set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${pFlags}")
			else()
				message(FATAL_ERROR "Unknown configuration, cannot add linker flags!")
			endif()
		endforeach()
	endif()
endmacro()

# Replace linker flags, configuration type is optional
macro(replace_linker_flags pSearch pReplace)
	set(MacroArgs "${ARGN}")
	if( NOT MacroArgs )
		string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
		string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
		string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
	else()
		foreach(MacroArg IN LISTS MacroArgs)
			if( MacroArg STREQUAL "debug" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG}")
			elseif( MacroArg STREQUAL "dev" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_EXE_LINKER_FLAGS_DEV "${CMAKE_EXE_LINKER_FLAGS_DEV}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_SHARED_LINKER_FLAGS_DEV "${CMAKE_SHARED_LINKER_FLAGS_DEV}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_MODULE_LINKER_FLAGS_DEV "${CMAKE_MODULE_LINKER_FLAGS_DEV}")
			elseif( MacroArg STREQUAL "release" )
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
				string(REGEX REPLACE "${pSearch}" "${pReplace}" CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
			else()
				message(FATAL_ERROR "Unknown configuration, cannot replace linker flags!")
			endif()
		endforeach()
	endif()
endmacro()

function(alimer_add_plugin name files)
	if (ALIMER_STATIC_PLUGINS)
        add_library(${name} STATIC ${files})
    else()
        add_library(${name} SHARED ${files})
    endif()

	target_link_libraries(${name} PRIVATE Alimer)
	set_target_properties(${name} PROPERTIES FOLDER Plugins)
endfunction()

function(alimer_copy_required_dlls TARGET_NAME)
	if(WIN32 OR WINDOWS_STORE)
		# Copy DXIL
    	set(VS_WINDOWS_SDK_BIN_DIR "$(WindowsSdkDir)/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")
    	set(VS_DXIL_SIGNER_PATH  "\"${VS_WINDOWS_SDK_BIN_DIR}/dxil.dll\"" CACHE INTERNAL "dxil.dll path")
		set(DXC_COMPILER_PATH  "\"${ALIMER_THIRD_PARTY_DIR}/dxc/bin/dxcompiler.dll\"" CACHE INTERNAL "dxcompiler.dll path")

		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${VS_DXIL_SIGNER_PATH}
			"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\""
		)

		# Copy dxcompiler (third_party/dxc/bin)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${DXC_COMPILER_PATH}
			"\"$<TARGET_FILE_DIR:${TARGET_NAME}>\""
		)
	endif ()
endfunction()