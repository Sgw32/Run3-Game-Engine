cmake_minimum_required(VERSION 3.28)

set(_media "${RUN3_SOURCE_ROOT}/mygui/Media/MyGUI_Media")
set(_required
    MyGUI_DirectX11_VP.hlsl
    MyGUI_DirectX11_FP.hlsl
    MyGUI_OpenGL3_VP.glsl
    MyGUI_OpenGL3_FP.glsl)
foreach(_name IN LISTS _required)
    set(_path "${_media}/${_name}")
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR "Required maintained UI shader is missing: ${_path}")
    endif()
    file(READ "${_path}" _source)
    string(TOLOWER "${_source}" _lower)
    if(_lower MATCHES "(^|[^a-z0-9_])(cg|ps_2_0|vs_2_0)([^a-z0-9_]|$)")
        message(FATAL_ERROR "Retired shader language/profile in ${_path}")
    endif()
endforeach()
file(READ "${_media}/MyGUI_OpenGL3_VP.glsl" _gl_vertex)
file(READ "${_media}/MyGUI_OpenGL3_FP.glsl" _gl_fragment)
if(NOT _gl_vertex MATCHES "#version 1[5-9][0-9]" OR
   NOT _gl_fragment MATCHES "#version 1[5-9][0-9]")
    message(FATAL_ERROR "Required GL3+ UI shaders are not GLSL 1.50+")
endif()
if(WIN32)
    file(READ "${_media}/MyGUI_DirectX11_FP.hlsl" _d3d_fragment)
    if(NOT _d3d_fragment MATCHES "SV_TARGET")
        message(FATAL_ERROR "Required D3D11 UI shader lacks SM4+ semantics")
    endif()
endif()

file(GLOB_RECURSE _live_sources
     "${RUN3_SOURCE_ROOT}/source/*.cpp"
     "${RUN3_SOURCE_ROOT}/include/run3/*.hpp")
foreach(_path IN LISTS _live_sources)
    file(READ "${_path}" _source)
    if(_source MATCHES "#include[ \t]*[<\"](CEGUI|Cg|Hydrax|SkyX|dshow)")
        message(FATAL_ERROR "Retired visual dependency escaped into live code: ${_path}")
    endif()
endforeach()

message(STATUS "Step 9A maintained shader and visual dependency boundary is clean")
