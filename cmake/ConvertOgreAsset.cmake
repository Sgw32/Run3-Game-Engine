cmake_minimum_required(VERSION 3.28)

# Non-destructive wrapper for the Ogre 14.5.2 command-line conversion tools.
# Required inputs:
#   RUN3_CONVERTER       OgreMeshUpgrader or OgreXMLConverter executable
#   RUN3_CONVERTER_MODE  mesh-upgrader or xml-converter
#   RUN3_INPUT           source .mesh/.skeleton/.xml file
# Optional inputs:
#   RUN3_OUTPUT_ROOT     defaults to <repository>/converted-content
#   RUN3_CONVERTER_OPTIONS is a CMake list of additional tool arguments

foreach(_required RUN3_CONVERTER RUN3_CONVERTER_MODE RUN3_INPUT)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

if(NOT RUN3_CONVERTER_MODE STREQUAL "mesh-upgrader" AND
   NOT RUN3_CONVERTER_MODE STREQUAL "xml-converter")
    message(FATAL_ERROR
            "RUN3_CONVERTER_MODE must be mesh-upgrader or xml-converter")
endif()

cmake_path(ABSOLUTE_PATH RUN3_CONVERTER NORMALIZE
           OUTPUT_VARIABLE _converter)
cmake_path(ABSOLUTE_PATH RUN3_INPUT NORMALIZE OUTPUT_VARIABLE _input)
if(NOT EXISTS "${_converter}" OR IS_DIRECTORY "${_converter}")
    message(FATAL_ERROR "Converter executable does not exist: ${_converter}")
endif()
if(NOT EXISTS "${_input}" OR IS_DIRECTORY "${_input}")
    message(FATAL_ERROR "Input asset does not exist: ${_input}")
endif()

if(NOT DEFINED RUN3_OUTPUT_ROOT OR "${RUN3_OUTPUT_ROOT}" STREQUAL "")
    cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _repository_root)
    set(RUN3_OUTPUT_ROOT "${_repository_root}/converted-content")
endif()
cmake_path(ABSOLUTE_PATH RUN3_OUTPUT_ROOT NORMALIZE
           OUTPUT_VARIABLE _output_root)

file(SHA256 "${_input}" _input_hash_before)
file(SHA256 "${_converter}" _converter_hash)
string(JOIN ";" _options_text ${RUN3_CONVERTER_OPTIONS})
string(SHA256 _invocation_hash
       "ogre=14.5.2;mode=${RUN3_CONVERTER_MODE};options=${_options_text}")
string(SUBSTRING "${_converter_hash}" 0 16 _converter_id)
string(SUBSTRING "${_invocation_hash}" 0 16 _invocation_id)

cmake_path(GET _input FILENAME _input_name)
cmake_path(GET _input EXTENSION _input_extension)
string(TOLOWER "${_input_extension}" _input_extension)
if(RUN3_CONVERTER_MODE STREQUAL "mesh-upgrader")
    if(NOT _input_extension STREQUAL ".mesh")
        message(FATAL_ERROR "OgreMeshUpgrader input must end in .mesh")
    endif()
    set(_output_name "${_input_name}")
else()
    if(_input_extension STREQUAL ".mesh" OR
       _input_extension STREQUAL ".skeleton")
        set(_output_name "${_input_name}.xml")
    elseif(_input_extension STREQUAL ".xml")
        file(READ "${_input}" _xml LIMIT 65536)
        if(_xml MATCHES "<[ \t\r\n]*mesh([ \t\r\n>/])")
            cmake_path(GET _input STEM _input_stem)
            set(_output_name "${_input_stem}.mesh")
        elseif(_xml MATCHES "<[ \t\r\n]*skeleton([ \t\r\n>/])")
            cmake_path(GET _input STEM _input_stem)
            set(_output_name "${_input_stem}.skeleton")
        else()
            message(FATAL_ERROR
                    "XML input root must be <mesh> or <skeleton>")
        endif()
    else()
        message(FATAL_ERROR
                "OgreXMLConverter input must end in .mesh, .skeleton, or .xml")
    endif()
endif()

set(_bucket
    "${_output_root}/${RUN3_CONVERTER_MODE}/${_input_hash_before}/${_converter_id}-${_invocation_id}")
set(_output "${_bucket}/${_output_name}")
set(_receipt "${_bucket}/conversion.json")
set(_log "${_bucket}/converter.log")

# Completed outputs are immutable and reusable only when their hashes match.
# Incomplete output is preserved for diagnosis and is never overwritten.
if(EXISTS "${_receipt}")
    if(NOT EXISTS "${_output}")
        message(FATAL_ERROR
                "Receipt exists but converted output is missing: ${_output}")
    endif()
    file(READ "${_receipt}" _prior_receipt)
    string(JSON _recorded_input GET "${_prior_receipt}" input sha256)
    string(JSON _recorded_output GET "${_prior_receipt}" output sha256)
    file(SHA256 "${_output}" _actual_output)
    if(NOT _recorded_input STREQUAL _input_hash_before OR
       NOT _recorded_output STREQUAL _actual_output)
        message(FATAL_ERROR "Existing conversion receipt/hash verification failed")
    endif()
    message(STATUS "Verified existing deterministic output: ${_output}")
    return()
elseif(EXISTS "${_output}")
    message(FATAL_ERROR
            "Refusing to overwrite incomplete conversion output: ${_output}")
endif()

file(MAKE_DIRECTORY "${_bucket}")
execute_process(
    COMMAND "${_converter}" ${RUN3_CONVERTER_OPTIONS}
            -log "${_log}" "${_input}" "${_output}"
    WORKING_DIRECTORY "${_bucket}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr)
file(SHA256 "${_input}" _input_hash_after)
if(NOT _input_hash_before STREQUAL _input_hash_after)
    message(FATAL_ERROR
            "Source asset changed during conversion; stop and restore it: ${_input}")
endif()
if(NOT _result EQUAL 0)
    message(FATAL_ERROR
            "Converter failed with ${_result}; source hash is unchanged.\n${_stdout}\n${_stderr}")
endif()
if(NOT EXISTS "${_output}" OR IS_DIRECTORY "${_output}")
    message(FATAL_ERROR "Converter reported success but produced no output")
endif()
file(SHA256 "${_output}" _output_hash)
file(RELATIVE_PATH _output_relative "${_output_root}" "${_output}")
file(TO_CMAKE_PATH "${_input}" _input_json)
file(TO_CMAKE_PATH "${_converter}" _converter_json)
file(TO_CMAKE_PATH "${_output_relative}" _output_json)

set(_json "{}")
string(JSON _json SET "${_json}" schema_version 1)
string(JSON _json SET "${_json}" ogre_version "\"14.5.2\"")
string(JSON _json SET "${_json}" mode "\"${RUN3_CONVERTER_MODE}\"")
string(JSON _json SET "${_json}" tool
       "{\"path\":\"${_converter_json}\",\"sha256\":\"${_converter_hash}\"}")
string(JSON _json SET "${_json}" input
       "{\"path\":\"${_input_json}\",\"sha256\":\"${_input_hash_before}\"}")
string(JSON _json SET "${_json}" output
       "{\"path\":\"${_output_json}\",\"sha256\":\"${_output_hash}\"}")
string(JSON _json SET "${_json}" options "\"${_options_text}\"")
file(WRITE "${_receipt}" "${_json}\n")
message(STATUS "Converted without modifying the source: ${_output}")
message(STATUS "Input SHA-256:  ${_input_hash_before}")
message(STATUS "Output SHA-256: ${_output_hash}")
