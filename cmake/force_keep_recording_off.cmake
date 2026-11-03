# Copies INPUT to OUTPUT and forces keepRecordingFiles=false.
# Used for the Android asset only. Does not modify INPUT.
# Existing on-device settings are not touched: startup copies an asset
# only when that file is not already there.
if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "INPUT and OUTPUT are required")
endif()

file(READ "${INPUT}" content)
string(REGEX REPLACE "(^|\r?\n)keepRecordingFiles=[^\r\n]*"
    "\\1keepRecordingFiles=false" patched "${content}")
if(NOT patched MATCHES "(^|\n|\r\n)keepRecordingFiles=false(\r?\n|$)")
    if(NOT patched MATCHES "(\n|\r)$")
        string(APPEND patched "\n")
    endif()
    string(APPEND patched "keepRecordingFiles=false\n")
endif()

if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" existing)
    if(existing STREQUAL patched)
        return()
    endif()
endif()

get_filename_component(_out_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")
file(WRITE "${OUTPUT}" "${patched}")
