set(HEADER_FILES
)

set(SOURCE_FILES
    main.c
)

if(ALIMER_BUILD_SHARED)
    add_library(alimer SHARED ${SOURCE_FILES} ${HEADER_FILES})
else()
    add_executable(alimer ${SOURCE_FILES} ${HEADER_FILES})
endif()