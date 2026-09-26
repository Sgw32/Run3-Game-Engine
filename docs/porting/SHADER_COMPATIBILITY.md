# Step 9A shader and material compatibility

Step 9A establishes a portable required set; it does not perform the Step 9B
lighting redesign. Source content remains byte-for-byte unchanged.

| Runtime path | D3D11 | GL3+ | Required implementation and fallback |
|---|---|---|---|
| ordinary opaque/alpha-tested map and dynamic materials | Ogre RTSS HLSL | Ogre RTSS GLSL | generated compatibility materials preserve diffuse texture, culling, alpha/depth state, and legacy `lighting off`; missing fancy programs fall back to a visible material |
| transparent materials and portable water | Ogre/RTSS | Ogre/RTSS | depth test stays on; transparent passes disable depth writes; simple water replaces the mandatory Hydrax path |
| ParticleFX and material aliases | Ogre/RTSS | Ogre/RTSS | required authored particle materials use the same catalogue; unlit billboard materials never request normals |
| MyGUI window/HUD/computer surface | `MyGUI_DirectX11_*.hlsl` | `MyGUI_OpenGL3_*.glsl` | maintained MyGUI SM4+/GLSL 1.50 shaders from the pinned submodule |
| computer screen material | Ogre unlit textured pass | Ogre unlit textured pass | samples the isolated MyGUI render texture and restores the authored submesh material on teardown |
| sky | Ogre skybox | Ogre skybox | requested material when available, otherwise `Run3/PortableSkyFallback`; SkyX is not linked |
| legacy Cg, `vs_2_0`, `ps_2_0` | not loaded | not loaded | retired from the required runtime; no Cg plugin is staged |
| legacy HDR/LSD compositor requests | portable no-op with diagnostic | portable no-op with diagnostic | typed gameplay state continues without loading obsolete shader programs; modern HDR belongs to Step 9B |
| Hydrax/SkyX rich effects | not linked | not linked | portable sky/water above; richer adapters may return only as optional, reproducible backends |
| DirectShow/WMV intro | not linked | not applicable | intro is disabled by default; `--intro` logs the portable skip and continues immediately |

`cmake/CheckStep9AShaders.cmake` fails when a required MyGUI shader is
missing, uses Cg/SM2 tokens, lacks the expected GL3+/D3D11 semantics, or when a
live first-party source includes Cg, Hydrax, SkyX, or DirectShow. Installed
shell smoke tests additionally scan `ogre.log` and fail on required shader
compile-error patterns.

Verified required-set results on 2026-09-26:

| Host/configuration | Renderer | Result |
|---|---|---|
| Windows MSVC Debug and Release installed shell | D3D11 | five-frame smoke passed; no required shader compile errors |
| Windows MSVC Debug and Release installed shell | GL3+ | five-frame smoke passed; no required shader compile errors |
| Windows standalone MyGUI/Ogre demo | D3D11 and GL3+ | rotating cube and foreground GUI smoke passed |
| WSL GCC Debug cached-dependency build | GL3+ through WSLg | installed shell and standalone demo smokes passed |

The legacy RTSS `X3205` narrowing warnings are compiler warnings in Ogre's
generated lighting code, not shader compilation failures. Step 9B owns modern
lighting, shadows, material maps, HDR/tone mapping, and any reviewed
replacement for visual-only legacy compositors.
