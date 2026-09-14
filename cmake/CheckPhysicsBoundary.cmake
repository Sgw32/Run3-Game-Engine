cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED RUN3_SOURCE_ROOT OR RUN3_SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "RUN3_SOURCE_ROOT is required")
endif()

file(GLOB _run3_root_sources
     LIST_DIRECTORIES FALSE
     "${RUN3_SOURCE_ROOT}/*.h"
     "${RUN3_SOURCE_ROOT}/*.hpp"
     "${RUN3_SOURCE_ROOT}/*.cpp")
file(GLOB_RECURSE _run3_owned_sources
     LIST_DIRECTORIES FALSE
     "${RUN3_SOURCE_ROOT}/include/*.h"
     "${RUN3_SOURCE_ROOT}/include/*.hpp"
     "${RUN3_SOURCE_ROOT}/include/*.cpp"
     "${RUN3_SOURCE_ROOT}/source/*.h"
     "${RUN3_SOURCE_ROOT}/source/*.hpp"
     "${RUN3_SOURCE_ROOT}/source/*.cpp"
     "${RUN3_SOURCE_ROOT}/AIR3-System/*.h"
     "${RUN3_SOURCE_ROOT}/AIR3-System/*.hpp"
     "${RUN3_SOURCE_ROOT}/AIR3-System/*.cpp")
set(_run3_sources ${_run3_root_sources} ${_run3_owned_sources})
set(_violations)
foreach(_source IN LISTS _run3_sources)
    file(RELATIVE_PATH _relative "${RUN3_SOURCE_ROOT}" "${_source}")
    if(_relative STREQUAL "source/physics/BulletPhysicsBackend.cpp")
        continue()
    endif()
    file(STRINGS "${_source}" _bullet_includes
         REGEX "^[ \t]*#[ \t]*include[ \t]*<(bt[A-Z]|Bullet[^>]*/|LinearMath/)")
    if(_bullet_includes)
        list(APPEND _violations "${_relative}: ${_bullet_includes}")
    endif()
endforeach()

if(_violations)
    list(JOIN _violations "\n" _message)
    message(FATAL_ERROR
            "Bullet headers escaped the private backend implementation:\n${_message}")
endif()

message(STATUS "Bullet headers are confined to BulletPhysicsBackend.cpp")
