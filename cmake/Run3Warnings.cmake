include_guard(GLOBAL)

function(run3_create_warning_targets)
    if(TARGET run3_warnings)
        return()
    endif()

    add_library(run3_warnings INTERFACE)
    add_library(run3::warnings ALIAS run3_warnings)

    add_library(run3_warnings_as_errors INTERFACE)
    add_library(run3::warnings_as_errors ALIAS run3_warnings_as_errors)
    target_link_libraries(run3_warnings_as_errors INTERFACE run3_warnings)

    if(MSVC)
        target_compile_options(
            run3_warnings
            INTERFACE /W4 /permissive- /Zc:__cplusplus /Zc:preprocessor)
        target_compile_options(run3_warnings_as_errors INTERFACE /WX)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(
            run3_warnings
            INTERFACE -Wall -Wextra -Wpedantic)
        target_compile_options(run3_warnings_as_errors INTERFACE -Werror)
    else()
        message(WARNING
            "No Run3 warning policy is defined for "
            "${CMAKE_CXX_COMPILER_ID}; warning helper targets are empty.")
    endif()
endfunction()
