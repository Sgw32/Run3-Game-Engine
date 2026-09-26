cmake_minimum_required(VERSION 3.28)

project(Run3MyGUIOgreDemo VERSION 1.0.0 LANGUAGES CXX)

set(RUN3_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(RUN3_MYGUI_SOURCE_DIR "${RUN3_REPOSITORY_ROOT}/mygui" CACHE PATH
    "Path to the pinned MyGUI submodule")
set(RUN3_MYGUI_REVISION "8629ea76896fba2d837cffde9fce9f32935a1d11")

if(NOT EXISTS "${RUN3_MYGUI_SOURCE_DIR}/MyGUIEngine/CMakeLists.txt")
    message(FATAL_ERROR
        "MyGUI is not initialized at '${RUN3_MYGUI_SOURCE_DIR}'. Run "
        "'git submodule update --init mygui' from the repository root.")
endif()
find_package(Git QUIET)
if(Git_FOUND AND EXISTS "${RUN3_MYGUI_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${RUN3_MYGUI_SOURCE_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE _run3_mygui_head
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _run3_mygui_revision_result)
    if(NOT _run3_mygui_revision_result EQUAL 0 OR
       NOT _run3_mygui_head STREQUAL RUN3_MYGUI_REVISION)
        message(FATAL_ERROR
            "MyGUI checkout must be ${RUN3_MYGUI_REVISION}; found '${_run3_mygui_head}'")
    endif()
endif()

find_package(OGRE 14.5.2 EXACT CONFIG REQUIRED
             COMPONENTS Bites Overlay RTShaderSystem)
find_package(pugixml CONFIG REQUIRED)

foreach(_target IN ITEMS OgreMain OgreBites OgreOverlay OgreRTShaderSystem
                         RenderSystem_GL3Plus Codec_STBI)
    if(NOT TARGET "${_target}")
        message(FATAL_ERROR "Pinned Ogre package does not export ${_target}")
    endif()
endforeach()
if(WIN32 AND NOT TARGET RenderSystem_Direct3D11)
    message(FATAL_ERROR
        "Pinned Windows Ogre package does not export RenderSystem_Direct3D11")
endif()

# Build only the two MyGUI libraries used by this demonstration. MyGUI pins
# its small utf8cpp FetchContent dependency to v4.1.1; updates are disabled
# after the first configure. pugixml comes from the pinned Run3 vcpkg graph.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static MyGUI libraries" FORCE)
set(MYGUI_RENDERSYSTEM 3 CACHE STRING "Use Ogre classic" FORCE)
set(MYGUI_BUILD_RENDERSYSTEMS 3 CACHE STRING "Build Ogre platform only" FORCE)
set(MYGUI_USE_FREETYPE OFF CACHE BOOL "Use generated demo font atlas" FORCE)
set(MYGUI_USE_SYSTEM_PUGIXML ON CACHE BOOL "Use pinned vcpkg pugixml" FORCE)
set(MYGUI_BUILD_DEMOS OFF CACHE BOOL "Do not build upstream demos" FORCE)
set(MYGUI_BUILD_ADVANCED_DEMOS OFF CACHE BOOL "Do not build upstream advanced demos" FORCE)
set(MYGUI_BUILD_TOOLS OFF CACHE BOOL "Do not build upstream tools" FORCE)
set(MYGUI_BUILD_UNITTESTS OFF CACHE BOOL "Do not build upstream tests" FORCE)
set(MYGUI_BUILD_TEST_APP OFF CACHE BOOL "Do not build upstream test app" FORCE)
set(MYGUI_BUILD_DOCS OFF CACHE BOOL "Do not build upstream docs" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL
    "Never update already-populated MyGUI dependencies" FORCE)
add_subdirectory("${RUN3_MYGUI_SOURCE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/mygui" EXCLUDE_FROM_ALL)

if(NOT TARGET MyGUI::MyGUI OR NOT TARGET MyGUI::OgrePlatform)
    message(FATAL_ERROR "Pinned MyGUI did not export its expected CMake targets")
endif()

add_executable(run3_mygui_ogre_demo main.cpp)
target_compile_features(run3_mygui_ogre_demo PRIVATE cxx_std_17)
target_link_libraries(
    run3_mygui_ogre_demo
    PRIVATE MyGUI::MyGUI MyGUI::OgrePlatform OgreBites OgreOverlay
            OgreRTShaderSystem)

if(MSVC)
    target_compile_options(run3_mygui_ogre_demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(run3_mygui_ogre_demo PRIVATE
                           -Wall -Wextra -Wpedantic)
endif()

set(_runtime_dir "${CMAKE_CURRENT_BINARY_DIR}/bin")
set_target_properties(run3_mygui_ogre_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_runtime_dir}")
if(UNIX AND NOT APPLE)
    set_property(TARGET run3_mygui_ogre_demo PROPERTY
                 BUILD_RPATH "$ORIGIN;$ORIGIN/../lib;$ORIGIN/plugins")
endif()

file(TO_CMAKE_PATH "${_runtime_dir}/media" _demo_media_dir)
target_compile_definitions(run3_mygui_ogre_demo PRIVATE
    RUN3_MYGUI_DEMO_MEDIA_DIR="${_demo_media_dir}")

if(WIN32)
    set(_plugins_text
        "PluginFolder=plugins\nPlugin=RenderSystem_Direct3D11\nPlugin=RenderSystem_GL3Plus\nPlugin=Codec_STBI\n")
    set(_plugin_targets RenderSystem_Direct3D11 RenderSystem_GL3Plus Codec_STBI)
else()
    set(_plugins_text
        "PluginFolder=plugins\nPlugin=RenderSystem_GL3Plus\nPlugin=Codec_STBI\n")
    set(_plugin_targets RenderSystem_GL3Plus Codec_STBI)
endif()
file(GENERATE OUTPUT "${_runtime_dir}/plugins.cfg"
     CONTENT "${_plugins_text}")

add_custom_command(TARGET run3_mygui_ogre_demo POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${_runtime_dir}/plugins" "${_runtime_dir}/media"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RUN3_MYGUI_SOURCE_DIR}/Media/MyGUI_Media"
            "${_runtime_dir}/media/MyGUI_Media"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${OGRE_MEDIA_DIR}/Main" "${_runtime_dir}/media/OgreMain"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${OGRE_MEDIA_DIR}/RTShaderLib"
            "${_runtime_dir}/media/RTShaderLib"
    VERBATIM)
foreach(_plugin IN LISTS _plugin_targets)
    add_custom_command(TARGET run3_mygui_ogre_demo POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:${_plugin}>" "${_runtime_dir}/plugins/"
        VERBATIM)
endforeach()

include(CTest)
if(BUILD_TESTING)
    if(WIN32)
        set(_demo_renderer d3d11)
    else()
        set(_demo_renderer gl3plus)
    endif()
    add_test(NAME run3_mygui_ogre_demo.smoke
             COMMAND run3_mygui_ogre_demo --smoke-test
                     --renderer ${_demo_renderer})
    set_tests_properties(run3_mygui_ogre_demo.smoke PROPERTIES
        WORKING_DIRECTORY "${_runtime_dir}"
        TIMEOUT 60)
    if(WIN32)
        add_test(NAME run3_mygui_ogre_demo.gl3plus-smoke
                 COMMAND run3_mygui_ogre_demo --smoke-test
                         --renderer gl3plus)
        set_tests_properties(run3_mygui_ogre_demo.gl3plus-smoke PROPERTIES
            WORKING_DIRECTORY "${_runtime_dir}"
            TIMEOUT 60)
    endif()
endif()
