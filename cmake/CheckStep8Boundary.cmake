cmake_minimum_required(VERSION 3.28)

file(GLOB_RECURSE _live_files
     "${RUN3_SOURCE_ROOT}/include/run3/*.h"
     "${RUN3_SOURCE_ROOT}/include/run3/*.hpp"
     "${RUN3_SOURCE_ROOT}/source/*.cpp")

set(_allowed
    "${RUN3_SOURCE_ROOT}/source/scripting/ScriptEngine.cpp"
    "${RUN3_SOURCE_ROOT}/source/content/XmlParser.cpp")

foreach(_file IN LISTS _live_files)
    if(_file IN_LIST _allowed)
        continue()
    endif()
    file(READ "${_file}" _text)
    if(_text MATCHES "tinyxml\\.h|tinyxml2\\.h|lua\\.hpp|lua\\.h|luabind")
        message(FATAL_ERROR
                "Step 8 dependency escaped its Run3 adapter: ${_file}")
    endif()
endforeach()

get_filename_component(_legacy_sources "${RUN3_SOURCE_ROOT}/cmake/Run3LegacySources.cmake" ABSOLUTE)
file(READ "${_legacy_sources}" _legacy_text)
string(REGEX MATCH "set\\(RUN3_LEGACY_REUSABLE_SOURCES[^)]*tinyxml" _legacy_tinyxml "${_legacy_text}")
if(_legacy_tinyxml)
    message(FATAL_ERROR "Bundled TinyXML is still in the live legacy target")
endif()

message(STATUS "Step 8 XML/Lua dependency boundary is clean")
