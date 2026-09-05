cmake_minimum_required(VERSION 3.28)

project(Run3OgreMinimal VERSION 1.0.0 LANGUAGES CXX)

set(RUN3_OGRE_SDK_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../OgreSDK"
    CACHE PATH "Path to the Ogre SDK root")
set(RUN3_TLW_MODEL
    "${CMAKE_CURRENT_LIST_DIR}/../../Games/The Long Way/TheLongWay/media/models/cube.mesh"
    CACHE FILEPATH "The Long Way mesh used by the demo")
set(RUN3_OGRE_MESH_UPGRADER
    "${RUN3_OGRE_SDK_ROOT}/bin/OgreMeshUpgrader.exe"
    CACHE FILEPATH "Ogre tool used to convert legacy meshes without changing their source files")

if(NOT EXISTS "${RUN3_OGRE_SDK_ROOT}/CMake/OGREConfig.cmake")
    message(FATAL_ERROR
        "Ogre CMake package not found at '${RUN3_OGRE_SDK_ROOT}'. "
        "Set RUN3_OGRE_SDK_ROOT to the Ogre SDK root.")
endif()

# The SDK currently checked out in this workspace contains only a
# RelWithDebInfo import configuration. Avoid mixing its release CRT with a
# Debug application CRT.
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(FATAL_ERROR
            "This Ogre SDK provides RelWithDebInfo binaries only. "
            "Configure with -DCMAKE_BUILD_TYPE=RelWithDebInfo.")
    endif()
endif()

list(PREPEND CMAKE_PREFIX_PATH "${RUN3_OGRE_SDK_ROOT}")
find_package(OGRE 14.6.0 EXACT REQUIRED COMPONENTS Bites Overlay RTShaderSystem CONFIG)

add_executable(Run3OgreMinimal main.cpp)
target_compile_features(Run3OgreMinimal PRIVATE cxx_std_17)
target_link_libraries(Run3OgreMinimal PRIVATE OgreBites)

if(MSVC)
    target_compile_options(Run3OgreMinimal PRIVATE /W4 /permissive-)
else()
    target_compile_options(Run3OgreMinimal PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(RUN3_DEMO_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin")
set_target_properties(Run3OgreMinimal PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${RUN3_DEMO_RUNTIME_DIR}")

file(TO_CMAKE_PATH "${RUN3_DEMO_RUNTIME_DIR}/assets" RUN3_DEMO_ASSET_DIR)
target_compile_definitions(Run3OgreMinimal PRIVATE
    RUN3_DEMO_DEFAULT_ASSET_DIR="${RUN3_DEMO_ASSET_DIR}")

if(EXISTS "${RUN3_TLW_MODEL}" AND EXISTS "${RUN3_OGRE_MESH_UPGRADER}")
    set(RUN3_DEMO_STAGE_TLW_MODEL TRUE)
    target_compile_definitions(Run3OgreMinimal PRIVATE RUN3_DEMO_HAS_TLW_MODEL=1)
elseif(EXISTS "${RUN3_TLW_MODEL}")
    message(WARNING
        "The Long Way cube.mesh exists, but OgreMeshUpgrader was not found at "
        "'${RUN3_OGRE_MESH_UPGRADER}'. The demo will use Ogre's generated cube.")
else()
    message(WARNING
        "The Long Way cube.mesh was not found at '${RUN3_TLW_MODEL}'. "
        "The demo will use Ogre's generated cube instead.")
endif()

set(RUN3_RUNTIME_FILES
    "$<TARGET_FILE:OgreMain>"
    "$<TARGET_FILE:OgreBites>"
    "$<TARGET_FILE:OgreOverlay>"
    "$<TARGET_FILE:OgreRTShaderSystem>"
    "$<TARGET_FILE:RenderSystem_Direct3D11>")

add_custom_command(TARGET Run3OgreMinimal POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RUN3_DEMO_RUNTIME_DIR}/assets/models"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${RUN3_RUNTIME_FILES}
            "${RUN3_DEMO_RUNTIME_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/plugins.cfg"
            "${RUN3_DEMO_RUNTIME_DIR}/plugins.cfg"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RUN3_OGRE_SDK_ROOT}/Media/Main"
            "${RUN3_DEMO_RUNTIME_DIR}/assets/Main"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RUN3_OGRE_SDK_ROOT}/Media/RTShaderLib"
            "${RUN3_DEMO_RUNTIME_DIR}/assets/RTShaderLib"
    VERBATIM)

foreach(runtime_file SDL2.dll z.dll)
    if(EXISTS "${RUN3_OGRE_SDK_ROOT}/bin/${runtime_file}")
        add_custom_command(TARGET Run3OgreMinimal POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${RUN3_OGRE_SDK_ROOT}/bin/${runtime_file}"
                    "${RUN3_DEMO_RUNTIME_DIR}/${runtime_file}"
            VERBATIM)
    endif()
endforeach()

if(RUN3_DEMO_STAGE_TLW_MODEL)
    add_custom_command(TARGET Run3OgreMinimal POST_BUILD
        # The game asset uses MeshSerializer_v1.40, which Ogre 14 cannot load
        # directly. Convert only the staged copy and preserve the source asset.
        COMMAND "${RUN3_OGRE_MESH_UPGRADER}" -V 1.10
                "${RUN3_TLW_MODEL}"
                "${RUN3_DEMO_RUNTIME_DIR}/assets/models/cube.mesh"
        VERBATIM)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME Run3OgreMinimal.smoke
        COMMAND Run3OgreMinimal --smoke-test)
    set_tests_properties(Run3OgreMinimal.smoke PROPERTIES
        WORKING_DIRECTORY "${RUN3_DEMO_RUNTIME_DIR}"
        TIMEOUT 30)
endif()
