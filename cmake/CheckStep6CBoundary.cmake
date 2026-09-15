if(NOT DEFINED RUN3_SOURCE_ROOT)
    message(FATAL_ERROR "RUN3_SOURCE_ROOT is required")
endif()

set(_roots
    "${RUN3_SOURCE_ROOT}/include/run3"
    "${RUN3_SOURCE_ROOT}/source/air3"
    "${RUN3_SOURCE_ROOT}/source/app"
    "${RUN3_SOURCE_ROOT}/source/core"
    "${RUN3_SOURCE_ROOT}/source/gameplay"
    "${RUN3_SOURCE_ROOT}/source/input"
    "${RUN3_SOURCE_ROOT}/source/legacy"
    "${RUN3_SOURCE_ROOT}/source/physics")
set(_files
    "${RUN3_SOURCE_ROOT}/CMakeLists.txt")
foreach(_root IN LISTS _roots)
    file(GLOB_RECURSE _found LIST_DIRECTORIES FALSE
         "${_root}/*.cpp" "${_root}/*.h" "${_root}/*.hpp")
    list(APPEND _files ${_found})
endforeach()

foreach(_file IN LISTS _files)
    file(READ "${_file}" _text)
    if(_text MATCHES "OgreNewt|Newton\\.h|newton\\.lib|RUN3_LEGACY_ENABLE_NEWTON")
        message(FATAL_ERROR "Live Newton dependency found: ${_file}")
    endif()
endforeach()

message(STATUS "Step 6C live source/build boundary contains no Newton dependency")
