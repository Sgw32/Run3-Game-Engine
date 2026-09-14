foreach(_required
        RUN3_PROGRAM RUN3_RENDERER RUN3_WORKING_DIR RUN3_USER_DIR
        RUN3_CONTENT_ROOT RUN3_MANIFEST)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${RUN3_WORKING_DIR}" "${RUN3_USER_DIR}")
set(_report "${RUN3_USER_DIR}/logs/asset-report.json")
execute_process(
    COMMAND "${RUN3_PROGRAM}"
            --renderer "${RUN3_RENDERER}"
            --frames 1
            --user-dir "${RUN3_USER_DIR}"
            --content-root "${RUN3_CONTENT_ROOT}"
            --manifest "${RUN3_MANIFEST}"
            --report "${_report}"
    WORKING_DIRECTORY "${RUN3_WORKING_DIR}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 60)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR
            "run3_asset_check failed with ${_result}\n${_stdout}\n${_stderr}")
endif()
if(NOT EXISTS "${_report}")
    message(FATAL_ERROR "run3_asset_check did not create ${_report}")
endif()
file(READ "${_report}" _json)
string(JSON _passed ERROR_VARIABLE _json_error GET "${_json}" passed)
if(_json_error OR NOT _passed)
    message(FATAL_ERROR "Asset report is not a passing JSON report: ${_json_error}")
endif()
if(NOT _stdout MATCHES "Run3 asset check: PASS")
    message(FATAL_ERROR "Concise PASS report was not printed:\n${_stdout}")
endif()
