file(GLOB SOURCE_FILES
    *.cs
)

add_library(AlimerSharp SHARED ${SOURCE_FILES})
target_compile_options(AlimerSharp PRIVATE "/unsafe")
target_compile_options(AlimerSharp PRIVATE "/langversion:6")
add_dependencies(AlimerSharp Alimer)
set_property(TARGET AlimerSharp PROPERTY FOLDER "Managed")

# SDK installation
install(TARGETS AlimerSharp
    EXPORT AlimerSharp
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
