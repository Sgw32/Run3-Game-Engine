foreach(_required RUN3_CMAKE RUN3_WRAPPER RUN3_CONVERTER RUN3_TEST_ROOT)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${RUN3_TEST_ROOT}")
set(_source "${RUN3_TEST_ROOT}/source.mesh")
set(_output_root "${RUN3_TEST_ROOT}/outputs")
file(WRITE "${_source}" "immutable miniature mesh fixture\n")
file(SHA256 "${_source}" _before)

set(_command
    "${RUN3_CMAKE}"
    "-DRUN3_CONVERTER=${RUN3_CONVERTER}"
    "-DRUN3_CONVERTER_MODE=mesh-upgrader"
    "-DRUN3_INPUT=${_source}"
    "-DRUN3_OUTPUT_ROOT=${_output_root}"
    -P "${RUN3_WRAPPER}")
execute_process(COMMAND ${_command}
                RESULT_VARIABLE _first_result
                OUTPUT_VARIABLE _first_output
                ERROR_VARIABLE _first_error)
if(NOT _first_result EQUAL 0)
    message(FATAL_ERROR
            "First conversion failed: ${_first_output}\n${_first_error}")
endif()
file(SHA256 "${_source}" _after_first)
if(NOT _before STREQUAL _after_first)
    message(FATAL_ERROR "Conversion wrapper modified its source")
endif()

execute_process(COMMAND ${_command}
                RESULT_VARIABLE _second_result
                OUTPUT_VARIABLE _second_output
                ERROR_VARIABLE _second_error)
if(NOT _second_result EQUAL 0 OR
   NOT _second_output MATCHES "Verified existing deterministic output")
    message(FATAL_ERROR
            "Second conversion did not verify/reuse output: "
            "${_second_output}\n${_second_error}")
endif()
file(SHA256 "${_source}" _after_second)
if(NOT _before STREQUAL _after_second)
    message(FATAL_ERROR "Repeat conversion wrapper modified its source")
endif()
file(SHA256 "${RUN3_CONVERTER}" _converter_hash)
string(SHA256 _invocation_hash "ogre=14.5.2;mode=mesh-upgrader;options=")
string(SUBSTRING "${_converter_hash}" 0 16 _converter_id)
string(SUBSTRING "${_invocation_hash}" 0 16 _invocation_id)
set(_receipt
    "${_output_root}/mesh-upgrader/${_before}/${_converter_id}-${_invocation_id}/conversion.json")
if(NOT EXISTS "${_receipt}")
    message(FATAL_ERROR "Deterministic conversion receipt is missing")
endif()
file(READ "${_receipt}" _receipt_json)
string(JSON _schema GET "${_receipt_json}" schema_version)
string(JSON _input_hash GET "${_receipt_json}" input sha256)
if(NOT _schema EQUAL 1 OR NOT _input_hash STREQUAL _before)
    message(FATAL_ERROR "Conversion receipt is invalid")
endif()
