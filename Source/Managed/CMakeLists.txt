cmake_minimum_required(VERSION 3.10)

if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")    
    enable_language(CSharp)
endif ()

# specify C# framework
set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION v4.6.1)

if("${CMAKE_BUILD_TYPE}" AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (DOTNET_BUILD_CONFIGURATION Release)
else()
    set (DOTNET_BUILD_CONFIGURATION Debug)
endif()

function(add_target_csharp name files)
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
        add_library(${name} SHARED ${files})
        target_compile_options(${name} PRIVATE "/unsafe")
        target_compile_options(${name} PRIVATE "/langversion:6")
        set_property(TARGET ${name} PROPERTY FOLDER "Managed")

        # SDK installation
        install(TARGETS AlimerSharp
            EXPORT AlimerSharp
            LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
            RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
            ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
        )

        #include_external_msproject(AlimerSharp AlimerSharp.csproj TYPE 9A19103F-16F7-4668-BE54-9A1E7A4F7556)
        #add_dependencies(AlimerSharp Alimer)
    else()
        add_custom_target(${name} ALL
		    COMMAND dotnet build /p:Configuration=${DOTNET_BUILD_CONFIGURATION} /p:OutDir=$<TARGET_FILE_DIR:Alimer>
		    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		    SOURCES ${files}
        )

        install (FILES $<TARGET_FILE_DIR:Alimer>/${name}.dll DESTINATION ${DEST_LIBRARY_DIR_CONFIG})
    endif()

    add_dependencies(${name} Alimer)
endfunction()

add_subdirectory(AlimerSharp)
