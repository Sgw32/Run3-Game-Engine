set(_roots
    "${RUN3_SOURCE_ROOT}/include"
    "${RUN3_SOURCE_ROOT}/source")
set(_forbidden
    "audiere[.]h"
    "([/\\]|<)alut[.]h"
    "([/\\]|<)al[/\\]"
    "oalufmod"
    "OpenAL32"
    "RUN3_LEGACY_ENABLE_LEGACY_AUDIO")

foreach(_root IN LISTS _roots)
    file(GLOB_RECURSE _files "${_root}/*.hpp" "${_root}/*.cpp")
    foreach(_file IN LISTS _files)
        file(READ "${_file}" _contents)
        foreach(_pattern IN LISTS _forbidden)
            if(_contents MATCHES "${_pattern}")
                message(FATAL_ERROR
                        "Live source contains retired audio dependency '${_pattern}': ${_file}")
            endif()
        endforeach()
    endforeach()
endforeach()

file(READ "${RUN3_SOURCE_ROOT}/CMakeLists.txt" _cmake)
foreach(_pattern IN LISTS _forbidden)
    if(_cmake MATCHES "${_pattern}")
        message(FATAL_ERROR
                "Live CMake contains retired audio dependency '${_pattern}'")
    endif()
endforeach()

message(STATUS "Run3 live audio boundary contains no OpenAL/ALUT/Audiere dependency")
