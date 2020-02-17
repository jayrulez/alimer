function(add_sample sample_name)
	file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${sample_name}/*.cpp")
	file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${sample_name}/*.cpp")

	add_executable(${sample_name} ${SOURCE_FILES} ${HEADER_FILES})
	target_link_libraries(${sample_name} alimer)

	if(WIN32)
		set_target_properties(${sample_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
	endif ()

	set_property(TARGET ${sample_name} PROPERTY FOLDER "Samples")
endfunction()

add_sample(01_hello)