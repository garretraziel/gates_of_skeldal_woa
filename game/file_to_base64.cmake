# file_to_base64.cmake - Convert a binary file to base64
# Usage: cmake -DIN_FILE=<input> -DOUT_FILE=<output> -P file_to_base64.cmake

file(READ "${IN_FILE}" FILE_CONTENT HEX)

# Convert hex to base64 via a temporary binary file approach
# CMake 3.18+ supports file(READ ... BASE64) but we target 3.16+
# Use string manipulation to convert hex pairs to base64

string(LENGTH "${FILE_CONTENT}" HEX_LEN)
math(EXPR BYTE_COUNT "${HEX_LEN} / 2")

set(BASE64_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
set(RESULT "")

# Process 3 bytes (6 hex chars) at a time
set(POS 0)
while(POS LESS HEX_LEN)
    # Read up to 3 bytes
    set(BYTES_IN_GROUP 0)
    set(ACCUM 0)

    foreach(B RANGE 0 2)
        math(EXPR BYTE_POS "${POS} + ${B} * 2")
        if(BYTE_POS LESS HEX_LEN)
            string(SUBSTRING "${FILE_CONTENT}" ${BYTE_POS} 2 HEX_BYTE)
            # Convert hex to decimal
            math(EXPR BYTE_VAL "0x${HEX_BYTE}" OUTPUT_FORMAT DECIMAL)
            math(EXPR SHIFT "(2 - ${B}) * 8")
            math(EXPR SHIFTED "${BYTE_VAL} << ${SHIFT}")
            math(EXPR ACCUM "${ACCUM} | ${SHIFTED}")
            math(EXPR BYTES_IN_GROUP "${BYTES_IN_GROUP} + 1")
        endif()
    endforeach()

    # Extract 6-bit groups and map to base64 chars
    if(BYTES_IN_GROUP GREATER_EQUAL 1)
        math(EXPR IDX "(${ACCUM} >> 18) & 63")
        string(SUBSTRING "${BASE64_CHARS}" ${IDX} 1 C)
        string(APPEND RESULT "${C}")

        math(EXPR IDX "(${ACCUM} >> 12) & 63")
        string(SUBSTRING "${BASE64_CHARS}" ${IDX} 1 C)
        string(APPEND RESULT "${C}")
    endif()

    if(BYTES_IN_GROUP GREATER_EQUAL 2)
        math(EXPR IDX "(${ACCUM} >> 6) & 63")
        string(SUBSTRING "${BASE64_CHARS}" ${IDX} 1 C)
        string(APPEND RESULT "${C}")
    else()
        string(APPEND RESULT "=")
    endif()

    if(BYTES_IN_GROUP GREATER_EQUAL 3)
        math(EXPR IDX "${ACCUM} & 63")
        string(SUBSTRING "${BASE64_CHARS}" ${IDX} 1 C)
        string(APPEND RESULT "${C}")
    else()
        string(APPEND RESULT "=")
    endif()

    math(EXPR POS "${POS} + 6")
endwhile()

file(WRITE "${OUT_FILE}" "${RESULT}")
