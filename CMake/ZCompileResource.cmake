function(generate_header INPUTFILE OUTPUTFILE VARNAME)
    set(DONE FALSE)
    set(CURRENTPOS 0)

    file(READ ${INPUTFILE} FILEDATA HEX)
    string(LENGTH "${FILEDATA}" DATALEN)
    set(OUTPUT_DATA "")

    file(WRITE ${OUTPUTFILE} "#pragma once\n\nstatic const char ${VARNAME}[] = { ")

    foreach(BYTE_OFFSET RANGE 0 "${DATALEN}" 2)
        string(SUBSTRING "${FILEDATA}" "${BYTE_OFFSET}" 2 HEX_STRING)
        string(LENGTH "${HEX_STRING}" TEMPLEN)
        if ("${TEMPLEN}" GREATER 0)
            set(OUTPUT_DATA "${OUTPUT_DATA}0x${HEX_STRING}, ")
        endif()
    endforeach()

    file(APPEND ${OUTPUTFILE} "${OUTPUT_DATA}0")
    file(APPEND ${OUTPUTFILE} " };\n")
endfunction()

function(z_compile_resources RESOURCE_LIST)
    foreach(RESOURCE_NAME ${ARGN})
        set(RESOURCE_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_NAME}")

        get_filename_component(FILENAME ${RESOURCE_NAME} NAME_WE)
        get_filename_component(EXT ${RESOURCE_NAME} EXT)
        string(SUBSTRING ${EXT} 1 -1 EXT)
        set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}_${EXT}.h")
        set(VARIABLE_NAME "${FILENAME}_${EXT}")

        generate_header(${RESOURCE_FILENAME} ${OUTPUT_FILE} ${VARIABLE_NAME})
    endforeach()
endfunction()
