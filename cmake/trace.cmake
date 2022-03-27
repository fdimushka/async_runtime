set(USE_TRACE ON)

find_program(_FLATC flatc)

message(STATUS "Use tracing")
add_definitions(-DUSE_TRACE)

set(SRC_FBS_FILES "src/fbs/TraceSchema.fbs")
set(DST_FBS_FILES "${CMAKE_CURRENT_BINARY_DIR}/TraceSchema.h")


set(DST_FLATBUFFERS_DIR "${CMAKE_CURRENT_BINARY_DIR}")

list(LENGTH SRC_FBS_FILES LEN)
math(EXPR FBS_LIST_NUM "${LEN} - 1")
foreach (LIST_ITEM RANGE ${FBS_LIST_NUM})
    list(GET SRC_FBS_FILES ${LIST_ITEM} SRC_FBS_FILE)
    list(GET DST_FBS_FILES ${LIST_ITEM} DST_FBS_FILE)
    get_filename_component(DST_FBS_FILE_DIR ${DST_FBS_FILE} DIRECTORY)

    add_custom_command (OUTPUT "${DST_FBS_FILE}"
            COMMENT "Generating flatbuffers header ${DST_FBS_FILE} from ${SRC_FBS_FILE}"
            COMMAND ${_FLATC} --cpp "${CMAKE_SOURCE_DIR}/${SRC_FBS_FILE}"
            WORKING_DIRECTORY "${DST_FLATBUFFERS_DIR}"
            DEPENDS ${SRC_FLATBUFFERS_DIR}
            )

    add_custom_command (OUTPUT "${DST_FBS_FILE}.js"
            COMMENT "Generating flatbuffers header ${DST_FBS_FILE} from ${SRC_FBS_FILE}"
            COMMAND ${_FLATC} --js "${CMAKE_SOURCE_DIR}/${SRC_FBS_FILE}"
            WORKING_DIRECTORY "${DST_FLATBUFFERS_DIR}"
            DEPENDS ${SRC_FLATBUFFERS_DIR}
            )

    set(FLATBUFFERS_H_FILES ${FLATBUFFERS_H_FILES} ${DST_FBS_FILE})
    set(FLATBUFFERS_H_FILES ${FLATBUFFERS_H_FILES} ${DST_FBS_FILE}.js)
endforeach()