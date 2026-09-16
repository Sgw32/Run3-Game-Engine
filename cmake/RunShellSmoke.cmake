if(NOT DEFINED RUN3_PROGRAM OR NOT DEFINED RUN3_RENDERER OR
   NOT DEFINED RUN3_WORKING_DIR OR NOT DEFINED RUN3_USER_DIR)
    message(FATAL_ERROR "RunShellSmoke.cmake is missing a required -D argument")
endif()

file(MAKE_DIRECTORY "${RUN3_WORKING_DIR}" "${RUN3_USER_DIR}")
set(_run3_audio_arguments)
if(DEFINED RUN3_AUDIO_BACKEND AND NOT RUN3_AUDIO_BACKEND STREQUAL "")
    list(APPEND _run3_audio_arguments --audio-backend "${RUN3_AUDIO_BACKEND}")
endif()
execute_process(
    COMMAND "${RUN3_PROGRAM}"
            --renderer "${RUN3_RENDERER}"
            --frames 5
            --user-dir "${RUN3_USER_DIR}"
            ${_run3_audio_arguments}
    WORKING_DIRECTORY "${RUN3_WORKING_DIR}"
    RESULT_VARIABLE _run3_result
    OUTPUT_VARIABLE _run3_stdout
    ERROR_VARIABLE _run3_stderr
    TIMEOUT 45)
if(NOT _run3_result EQUAL 0)
    message(FATAL_ERROR
            "run3_shell smoke failed (${_run3_result})\n"
            "stdout:\n${_run3_stdout}\n"
            "stderr:\n${_run3_stderr}")
endif()

set(_run3_log "${RUN3_USER_DIR}/logs/ogre.log")
if(NOT EXISTS "${_run3_log}")
    message(FATAL_ERROR "run3_shell did not create ${_run3_log}")
endif()
file(READ "${_run3_log}" _run3_log_text)
if(NOT _run3_log_text MATCHES "Run3 shell pinned Ogre version: 14\\.5\\.2")
    message(FATAL_ERROR "run3_shell log does not identify pinned Ogre 14.5.2")
endif()
if(NOT _run3_log_text MATCHES "Run3 shell selected render system:")
    message(FATAL_ERROR "run3_shell log does not identify its render system")
endif()
if(DEFINED RUN3_AUDIO_BACKEND AND RUN3_AUDIO_BACKEND STREQUAL "null" AND
   NOT _run3_log_text MATCHES "Run3 audio backend: null")
    message(FATAL_ERROR "run3_shell did not select the requested null audio backend")
endif()
