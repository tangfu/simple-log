aux_source_directory(. SRC_LIST)
add_library(simplelog ${SRC_LIST})

include(FindDoxygen)

add_custom_command(TARGET simplelog
    POST_BUILD
    COMMENT "[ generate doc ]"
    COMMAND ${DOXYGEN_EXECUTABLE} 2>/dev/null
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    #VERBATIM
    )

add_custom_command(TARGET simplelog
    POST_BUILD
    COMMENT "[ astyle code ]"
    COMMAND astyle src/*.c src/*.h 1>/dev/null 2>/dev/null
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    #VERBATIM
    )

add_custom_command(TARGET simplelog
    POST_BUILD
    COMMENT "[generate sha1 file]"
    COMMAND rm -rf sha1sum.* 2>/dev/null
    COMMAND [ -z `which sha1sum 2>/dev/null` ] || [ -z `which touch 2>/dev/null` ] || touch sha1sum.`sha1sum src/libsimplelog.a`
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
