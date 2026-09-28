if(NOT DEFINED RUN3_PROGRAM OR NOT DEFINED RUN3_USER_DIR)
    message(FATAL_ERROR "Lighting smoke requires installed program and isolated user directory")
endif()
execute_process(
    COMMAND "${RUN3_PROGRAM}" --renderer "${RUN3_RENDERER}"
            --lighting-lab --lighting-pipeline "${RUN3_PIPELINE}"
            --shadow-quality low --frames 12 --lighting-capture --lighting-resize
            --audio-backend null --user-dir "${RUN3_USER_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 90)
file(WRITE "${RUN3_USER_DIR}/lighting-process.log" "${output}\n${errors}")
if(NOT result EQUAL 0 OR "${output}${errors}" MATCHES "error [XC][0-9]+|Error compiling|compile error|failed to compile|\\[Run3 error\\]")
    message(FATAL_ERROR "Lighting smoke failed (${result}):\n${output}\n${errors}")
endif()
if(NOT EXISTS "${RUN3_USER_DIR}/logs/lighting.png" OR NOT EXISTS "${RUN3_USER_DIR}/logs/lighting.json")
    message(FATAL_ERROR "Lighting smoke did not produce its capture/report")
endif()
