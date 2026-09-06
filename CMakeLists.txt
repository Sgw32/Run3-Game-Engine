cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

project(
    Run3
    VERSION 0.1.0
    DESCRIPTION "Run3 game engine modernization"
    LANGUAGES CXX)

option(RUN3_BUILD_TESTS "Build the Run3 test suite" ON)
option(RUN3_BUILD_TOOLS "Build Run3 developer tools" ON)
option(RUN3_ENABLE_OPTIONAL_DEVICES
       "Enable optional hardware device backends" OFF)
option(RUN3_BUILD_LEGACY "Build the controlled legacy compatibility target" ON)

option(RUN3_LEGACY_ENABLE_NEWTON "Enable Newton/OgreNewt compatibility" OFF)
option(RUN3_LEGACY_ENABLE_OIS "Enable OIS compatibility" OFF)
option(RUN3_LEGACY_ENABLE_CEGUI "Enable CEGUI compatibility" OFF)
option(RUN3_LEGACY_ENABLE_HYDRAX "Enable Hydrax compatibility" OFF)
option(RUN3_LEGACY_ENABLE_SKYX "Enable SkyX compatibility" OFF)
option(RUN3_LEGACY_ENABLE_LEGACY_AUDIO "Enable Audiere/ALUT compatibility" OFF)
option(RUN3_LEGACY_ENABLE_DIRECTSHOW "Enable DirectShow compatibility" OFF)
option(RUN3_LEGACY_ENABLE_SERIAL "Enable legacy serial devices" OFF)
option(RUN3_LEGACY_ENABLE_NAMED_PIPES "Enable legacy named pipes" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Run3Warnings)
run3_create_warning_targets()

find_package(
    OGRE 14.5.2 EXACT CONFIG REQUIRED
    COMPONENTS Bites Overlay RTShaderSystem)

set(_run3_required_ogre_targets
    OgreMain
    OgreBites
    OgreOverlay
    OgreRTShaderSystem
    RenderSystem_GL3Plus
    Plugin_ParticleFX
    Codec_STBI)
if(WIN32)
    list(APPEND _run3_required_ogre_targets RenderSystem_Direct3D11)
endif()
foreach(_target IN LISTS _run3_required_ogre_targets)
    if(NOT TARGET "${_target}")
        message(FATAL_ERROR
                "Pinned Ogre 14.5.2 package does not export ${_target}")
    endif()
endforeach()

# This header-only target proves that new first-party code can be consumed
# without pulling any legacy engine source or local binary SDK into the graph.
add_library(run3_build_info INTERFACE)
add_library(run3::build_info ALIAS run3_build_info)
target_compile_features(run3_build_info INTERFACE cxx_std_17)
target_include_directories(
    run3_build_info
    INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(run3_build_info INTERFACE run3::warnings)

if(RUN3_BUILD_LEGACY)
    include(Run3LegacySources)

    set(_run3_unimplemented_legacy_features
        RUN3_LEGACY_ENABLE_NEWTON
        RUN3_LEGACY_ENABLE_OIS
        RUN3_LEGACY_ENABLE_CEGUI
        RUN3_LEGACY_ENABLE_HYDRAX
        RUN3_LEGACY_ENABLE_SKYX
        RUN3_LEGACY_ENABLE_LEGACY_AUDIO
        RUN3_LEGACY_ENABLE_DIRECTSHOW
        RUN3_LEGACY_ENABLE_SERIAL
        RUN3_LEGACY_ENABLE_NAMED_PIPES)
    foreach(_feature IN LISTS _run3_unimplemented_legacy_features)
        if(${_feature})
            message(FATAL_ERROR
                    "${_feature}=ON requested, but Step 3 provides only a "
                    "visible-failure null backend for this subsystem")
        endif()
    endforeach()

    add_library(run3_legacy STATIC
                ${RUN3_LEGACY_REUSABLE_SOURCES}
                source/legacy/LegacyFeatures.cpp
                source/legacy/LegacySmoke.cpp)
    add_library(run3::legacy ALIAS run3_legacy)
    target_compile_features(run3_legacy PUBLIC cxx_std_17)
    target_include_directories(
        run3_legacy
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_compile_definitions(
        run3_legacy
        PUBLIC RUN3_LEGACY_ENABLE_NEWTON=0
               RUN3_LEGACY_ENABLE_OIS=0
               RUN3_LEGACY_ENABLE_CEGUI=0
               RUN3_LEGACY_ENABLE_HYDRAX=0
               RUN3_LEGACY_ENABLE_SKYX=0
               RUN3_LEGACY_ENABLE_LEGACY_AUDIO=0
               RUN3_LEGACY_ENABLE_DIRECTSHOW=0
               RUN3_LEGACY_ENABLE_SERIAL=0
               RUN3_LEGACY_ENABLE_NAMED_PIPES=0
        PRIVATE TIXML_USE_STL NOMINMAX)
    target_link_libraries(run3_legacy PUBLIC OgreMain OgreOverlay)

    # Keep concessions local to untouched legacy translation units. New glue
    # and all other first-party targets retain the normal warning policy.
    if(MSVC)
        target_compile_options(run3_legacy PRIVATE /W3 /permissive
                               /wd4244 /wd4267 /wd4996)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(
            run3_legacy
            PRIVATE -Wall -Wextra -Wpedantic
                    -Wno-deprecated-declarations
                    -Wno-sign-compare
                    -Wno-unused-parameter)
    endif()

    message(STATUS
            "Run3 legacy compatibility subset: "
            "${RUN3_LEGACY_REUSABLE_SOURCE_COUNT}/"
            "${RUN3_LEGACY_VCPROJ_SOURCE_COUNT} vcproj translation units")
endif()

add_executable(run3_shell source/shell_main.cpp)
target_compile_features(run3_shell PRIVATE cxx_std_17)
target_link_libraries(
    run3_shell
    PRIVATE run3::warnings
            OgreMain
            OgreBites
            OgreOverlay
            OgreRTShaderSystem)
if(UNIX AND NOT APPLE)
    set_property(TARGET run3_shell PROPERTY INSTALL_RPATH "$ORIGIN/../lib")
endif()

if(RUN3_BUILD_TOOLS)
    add_executable(run3_build_probe source/build_probe_main.cpp)
    target_link_libraries(run3_build_probe PRIVATE run3::build_info)
endif()

# Keep CMake's conventional BUILD_TESTING cache entry synchronized with the
# Run3-owned option so downstream tooling and IDEs still understand the build.
set(BUILD_TESTING "${RUN3_BUILD_TESTS}" CACHE BOOL "Enable CTest targets" FORCE)
include(CTest)

if(RUN3_BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(run3_build_probe_tests tests/build_probe_tests.cpp)
    target_link_libraries(
        run3_build_probe_tests
        PRIVATE run3::build_info Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(run3_build_probe_tests)

    if(RUN3_BUILD_LEGACY)
        add_executable(run3_legacy_compile_tests
                       tests/legacy_compile_tests.cpp)
        target_link_libraries(
            run3_legacy_compile_tests
            PRIVATE run3::warnings run3::legacy Catch2::Catch2WithMain)
        catch_discover_tests(run3_legacy_compile_tests
                             TEST_PREFIX "run3_legacy."
                             PROPERTIES LABELS "legacy-compile-smoke")
    endif()

    set(_run3_can_smoke_test FALSE)
    if(WIN32 OR (UNIX AND DEFINED ENV{DISPLAY} AND NOT "$ENV{DISPLAY}" STREQUAL ""))
        set(_run3_can_smoke_test TRUE)
    endif()

    if(_run3_can_smoke_test)
        set(_run3_test_install "${CMAKE_BINARY_DIR}/test-install")
        add_test(
            NAME run3_shell.install
            COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
                    --prefix "${_run3_test_install}" --config "$<CONFIG>")
        set_tests_properties(run3_shell.install
                             PROPERTIES FIXTURES_SETUP run3_shell_installed)

        if(WIN32)
            set(_run3_smoke_renderer d3d11)
        else()
            set(_run3_smoke_renderer gl3plus)
        endif()
        add_test(
            NAME run3_shell.smoke
            COMMAND
                "${CMAKE_COMMAND}"
                "-DRUN3_PROGRAM=${_run3_test_install}/bin/run3_shell${CMAKE_EXECUTABLE_SUFFIX}"
                "-DRUN3_RENDERER=${_run3_smoke_renderer}"
                "-DRUN3_WORKING_DIR=${CMAKE_BINARY_DIR}/test-run-cwd"
                "-DRUN3_USER_DIR=${CMAKE_BINARY_DIR}/test-user"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunShellSmoke.cmake")
        set_tests_properties(
            run3_shell.smoke
            PROPERTIES FIXTURES_REQUIRED run3_shell_installed
                       TIMEOUT 60)
    else()
        message(STATUS
                "run3_shell smoke test disabled: this host has no graphical display")
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS run3_shell
        RUNTIME_DEPENDENCY_SET run3_runtime_dependencies
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

set(_run3_plugin_targets
    RenderSystem_GL3Plus
    Plugin_ParticleFX
    Codec_STBI)
if(WIN32)
    list(PREPEND _run3_plugin_targets RenderSystem_Direct3D11)
    set(_run3_plugin_destination "${CMAKE_INSTALL_BINDIR}/plugins")
    set(_run3_plugin_folder "./plugins")
else()
    set(_run3_plugin_destination "${CMAKE_INSTALL_LIBDIR}/run3/plugins")
    set(_run3_plugin_folder "../${CMAKE_INSTALL_LIBDIR}/run3/plugins")
endif()

install(
    IMPORTED_RUNTIME_ARTIFACTS ${_run3_plugin_targets}
    RUNTIME_DEPENDENCY_SET run3_runtime_dependencies
    RUNTIME DESTINATION "${_run3_plugin_destination}"
    LIBRARY DESTINATION "${_run3_plugin_destination}")

if(WIN32)
    set(_run3_system_dependency_excludes
        ".*[\\/][Ss]ystem32[\\/].*"
        ".*[\\/][Ss]ys[Ww][Oo][Ww]64[\\/].*")
    # vcpkg's Windows app-local deployment places the complete DLL closure
    # beside the build-tree executable. Searching that one directory avoids
    # ambiguous duplicates between app-local and vcpkg's package bin folder.
    set(_run3_dependency_search_args
        DIRECTORIES "$<TARGET_FILE_DIR:run3_shell>")
else()
    set(_run3_system_dependency_excludes
        "^/lib/.*"
        "^/lib64/.*"
        "^/usr/lib/.*"
        "^/usr/lib64/.*")
    set(_run3_dependency_search_args
        DIRECTORIES "$<TARGET_FILE_DIR:OgreMain>"
                    "$<TARGET_FILE_DIR:OgreBites>")
endif()
install(
    RUNTIME_DEPENDENCY_SET run3_runtime_dependencies
    ${_run3_dependency_search_args}
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ${_run3_system_dependency_excludes}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")

string(CONCAT _run3_plugins_config
    "# Generated for the installed Run3 renderer shell.\n"
    "PluginFolder=${_run3_plugin_folder}\n")
if(WIN32)
    string(APPEND _run3_plugins_config
           "Plugin=RenderSystem_Direct3D11\n")
endif()
string(APPEND _run3_plugins_config
       "Plugin=RenderSystem_GL3Plus\n"
       "Plugin=Plugin_ParticleFX\n"
       "Plugin=Codec_STBI\n")
file(GENERATE
     OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/plugins.cfg"
     CONTENT "${_run3_plugins_config}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/plugins.cfg"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(DIRECTORY "${OGRE_MEDIA_DIR}/Main"
                  "${OGRE_MEDIA_DIR}/RTShaderLib"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/run3/Media")

message(STATUS "Run3 build tests: ${RUN3_BUILD_TESTS}")
message(STATUS "Run3 developer tools: ${RUN3_BUILD_TOOLS}")
message(STATUS "Run3 optional devices: ${RUN3_ENABLE_OPTIONAL_DEVICES}")
message(STATUS "Run3 Ogre: ${OGRE_VERSION} (${OGRE_DIR})")
