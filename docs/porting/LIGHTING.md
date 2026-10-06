# Step 9B lighting workbench

Status: **in progress, not campaign-ready** (2026-10-05). Ogre **classic
14.5.2**, using the existing pinned vcpkg dependency. No Ogre-next or Cg.
The controls below are available for evaluation, not a claim of visual parity.

Owner-directed verification policy: compile plus small CPU-only unit checks;
no more automatic map launches/debugger sessions. Use
[LIGHTING_CHECKLIST.md](LIGHTING_CHECKLIST.md) for exact individual commands,
per-map/per-pipeline PASS/FAIL placeholders and error-report templates.

### Current material corrections (awaiting owner visual checks)

- Named `normalMap`/`normal_Map` and `specularMap` units and lowercase aliases
  now populate the adapter. Earlier parsing only recognized uppercase aliases.
- Lighting and opacity are independent. `lighting off` no longer erases
  `alpha_rejection greater 128` on Leaves/Fronds or glass alpha blending.
  Unlit tint/alpha is explicitly supplied to RTSS instead of relying on a
  lighting stage to apply material diffuse constants.
- Spherical reflection textures and `cubic_texture ... combinedUVW` use
  generated reflection coordinates, not ordinary UV0 albedo. Manual blend
  weights are retained; a layered cube reflection without a weight uses a
  documented 0.25 compatibility weight, reflection-only materials use 1.
  Skybox `separateUV` cube directions are distinct from reflective glass:
  they consume the sky geometry's float3 coordinates without requiring normals.
- `ANIM_SCREEN` authors `Random3D.dds 3d` and `NoiseVolume.dds 3d` for an old
  TV-effect shader. Treating its first noise volume as 2D albedo caused the
  texture-coordinate type mismatch. Volume/effect textures are now excluded
  from the surface adapter with a visible diagnostic; the TV distortion itself
  still needs its dedicated presentation adapter, not a fake diffuse texture.
- Unlit/reflection surfaces stay on deferred's forward path; depth casters
  explicitly disable lighting so RTSS does not demand `NORMAL0` solely to
  render depth. This addresses identified normal-less paths; the user's full
  map/particle coverage must still be verified manually.
- DotScene `colourAmbient` is now consumed. PBR's neutral IBL probe follows its
  colour in linear space, rather than adding a fixed bright probe regardless
  of the map. Ambient is not added a second time by the PBR material pass.
- Reused portable sky/water materials are immutable so map reload cannot leave
  RTSS pointing to deleted source techniques. Computer material replacement
  explicitly retires its RTSS state first.

## Running the preview

After building and installing Windows Debug, from the repository root:

```bat
run_tlw.bat --lighting-pipeline legacy-forward --shadow-quality low
run_tlw.bat --lighting-pipeline fast-forward --shadow-quality off
run_tlw.bat --lighting-pipeline pbr --shadow-quality high --exposure 1
run_tlw.bat --lighting-pipeline deferred --shadow-quality low
run_tlw.bat --renderer gl3plus --lighting-pipeline deferred --shadow-quality low
run_tlw.bat --map tlwhome02 --lighting-pipeline legacy-forward
```

`run3.bat` accepts the same overrides and starts at the menu. Both scripts
preserve their high texture/model/scene settings. CLI overrides user config,
which overrides content defaults. Config keys: `render.lighting_pipeline`,
`render.shadow_quality`, `render.shadow_update_interval`, `render.exposure`.
Pipeline values are exactly those
above; shadow values are `off`, `low`, `medium`, `high`, `ultra`. Defaults are
`legacy-forward`, `off`, exposure `1`. Legacy/fast-forward refresh their shadow
textures every two rendered frames by default; deferred/PBR and
`--shadow-update-interval 1` retain Ogre's every-frame behaviour. Valid refresh
intervals are 1..8. Invalid values fail with an error.

Asset-independent lab (do not combine `--lighting-lab` with `--map`):

```powershell
& .\build\install\windows-debug\bin\run3_shell.exe --lighting-lab --renderer d3d11 --lighting-pipeline pbr --shadow-quality low --frames 120 --lighting-capture --user-dir "$PWD/build/user/lab-pbr"
```

The lab generates its meshes/textures and uses deterministic frame/60 animation
time. It includes opaque, normal/specular, metal-roughness/AO, cutout,
transparent, emissive and distant objects, with directional, moving point and
spot lighting. Captures are `logs/lighting.png` and `logs/lighting.json` under
the selected user directory. `--lighting-resize` exercises lab resize;
`--lighting-reload` unloads/reloads a selected map once at frame 3.

## Current capability matrix

| Pipeline | Implemented approach | Important limits |
|---|---|---|
| legacy-forward | RTSS per-pixel diffuse/specular, six local lights and integrated texture shadows | Old multipass look still needs comparisons |
| fast-forward | RTSS per-pixel forward, six local lights and integrated texture shadows | Fast budget is not yet a measured performance guarantee |
| pbr | RTSS Cook-Torrance metal-roughness, neutral IBL, floating-point HDR and tone mapping | Neutral procedural probe, not authored environment lighting; campaign shader compatibility under investigation |
| deferred | Five floating-point MRTs, directional/point/spot resolve, forward transparent/unlit pass, HDR tone mapping | Full-screen loop capped at 64 lights, **not yet light volumes**; forward composition and campaign coverage incomplete |

All paths preserve ordinary Ogre light types, colours, transforms, attenuation
and spotlight cones. The shared Ogre texture-shadow renderer allocates one map
per shadow-casting light, up to the six-light maximum found in the shipped map
definitions. This covers every authored spotlight in high-quality `tlwcao` and
`outro`; exceeding the budget is a hard load error instead of silently dropping
a light. Low/medium/high/ultra allocate 512/1024/2048/4096-square depth textures
with 2,500/5,000/10,000/20,000 game-unit ranges. Low/medium use PCF4 and
high/ultra use PCF16. Spotlights use their authored cone. Point-light
illumination is omnidirectional, but point lights do not cast texture shadows:
Ogre Classic's built-in point path is a camera-facing 120-degree projection,
not an omnidirectional shadow, and produces a moving clipping plane when the
viewer rotates. A proper six-face cubemap or dual-paraboloid implementation
remains required before point shadows can be enabled. Bias tuning and
device-capability fallback also remain required.

Run3's scene-manager overlay caches the completed spotlight shadow textures
between refreshes. It forces an immediate refresh whenever the active caster
light set, mask, type, position, or direction changes; otherwise the forward
pipelines use the configured frame interval. This reduces shadow-caster scene
batches without skipping the main world or compositor render. Lighting JSON
captures record refresh/skipped counts and rendered/estimated-avoided shadow
batches; window-target batch statistics still exclude off-screen passes.

Ogre 14.5.2 has an out-of-range projector lookup when a multi-light RTSS
receiver is rendered while no shadow-casting light intersects the camera
frustum. The renderer adapter keeps that transition valid with an internal
zero-power, zero-mask fallback light used only for Ogre's shadow-texture update.
It is not attached to map gameplay or presentation, does not replace an atlas
slot while an authored caster is present, and prevents stale projector pointers
as lights enter or leave the view.

Deferred buffers: diffuse RGB + shininess; view normal + normalized Euclidean
view distance; specular RGB + AO; and two RGBA local-shadow-factor buffers for
lights 0..5. The resolve writes depth for the subsequent forward pass. The GLSL full-screen varying uses the
same explicit `TEXCOORD0` location as its vertex shader; a previous mismatch
produced a blank image despite a successful exit.

## Material conventions and fallbacks

Run3 owns `MaterialDescription`; Ogre ownership stays in the renderer adapter.
Diffuse textures are sRGB in HDR pipelines, data maps are linear. Legacy and
fast retain the legacy display-space path pending matched-camera assessment.
Normal maps use tangent space and UV0; tangents are generated in memory only.
Absent normals/UVs or optional textures produce logged constant/unlit fallbacks,
never source-asset rewrites. Metal-roughness uses **green roughness, blue metal**;
AO uses red. Legacy shininess maps to `sqrt(2/(shininess+2))`, clamped to
0.045..1. A legacy specular map is not treated as metallic: its RGB mask modulates
PBR F0 before IBL/direct lighting, with gloss-derived roughness. PBR AO modulates indirect light;
forward AO is an artistic diffuse multiplier. Complete colour-space auditing,
including material constants and shared texture resources, remains open.

Opaque/cutout surfaces write depth; transparent surfaces alpha-blend without
depth writes. Authored double-sided state is retained. Cutouts use an explicit
alpha threshold. Emissive values are unclamped in HDR. Tone mapping uses an
exposure multiplier and a filmic curve followed by linear-to-sRGB output;
exposure is not a physical camera calibration. Authored attenuation remains in
game units; no gameplay/physics scale is changed.

## Inventory evidence

`tools/lighting_content.py inventory` reads content without rewriting it. The
initial workspace inventory found 2,595 material names, 54 map variants and
74 strict-XML exceptions. It separates mesh presence, map-declared mesh/material
use and inheritance; candidate matches across quality folders are not proof
that every duplicate definition executes.

Used candidate ancestors include cap-3 `Run3/highDetailMaterial` and several
cap-8 Full/Glow/transparent variants. The fixed-two `Run3/highDetail2Limited`
also has candidate evidence. Fixed-four, cap-two override and fixed-three
parallax definitions exist, but have not been proven live by this inventory;
they are parser fixtures, not a reason to restore unused Cg. There is no global
"three lights" rule. High tlwcao authors six spotlights; high tlwhome02 five.
Their low variants contain no DotScene lights, so quality selection affects
lighting as well as texture resolution.

## Derived content (opt-in, originals preserved)

```powershell
python tools/lighting_content.py inventory --content-root "Games/The Long Way/TheLongWay" --output build/lighting/inventory.json
python tools/lighting_content.py derive --content-root "Games/The Long Way/TheLongWay" --output derived-content/nextgen-v0
.\run_tlw.bat --lighting-pipeline pbr --content-variant nextgen --content-overlay "$PWD/derived-content/nextgen-v0"
```

Derivation refuses an existing output and any output inside/above the source
tree. It copies **only run3/core and run3/maps**, rejects escaped/symlink source
files and records input/output SHA-256 plus changes in `remaster-manifest.json`.
The current 211-file `lighting-v0-byte-identical-staging` recipe does **not**
retune a map yet. Outputs are Git-ignored. Unchanged models/textures/scripts
resolve from the original content root; overlay lookup is restricted to core
and maps. An explicit overlay path is recommended; without one, `nextgen`
looks under the selected user directory at `derived-content/nextgen`.
Original mode: omit the overlay and select `--content-variant original`.

## Verification and remaining exit criteria

The commands below describe the retained opt-in automated runner, **not the
current owner-directed workflow**. Do not launch it during compile-only work;
use the individual cases and blank results in LIGHTING_CHECKLIST.md instead.

```powershell
python tests/test_lighting_content.py
python tools/lighting_smoke.py --executable build/install/windows-debug/bin/run3_shell.exe --output build/lighting/my-comparison --frames 60 --shadow-quality low --resize
```

The Python smoke runner requires Pillow, refuses reused case directories,
captures process/shader errors, enforces a timeout, hashes screenshots and
rejects near-uniform images. This sanity gate is **not** an image-parity test.
JSON reports include renderer, dimensions, shader counts, shadow budget,
window draw/triangle counts and off-screen counts. GPU timing is explicitly
null; wall time includes startup, so it must not be reported as GPU time or
used to claim a performance win. Warmed timings and reviewed image-regression
tolerances are still needed.

Open: owner confirmation of campaign reload/PBR repairs; complete transparent and cutout-shadow
comparisons; omnidirectional point-light cubemap shadows; light volumes; capability fallback;
exception-path lifetime checks; warmed performance measurements; reviewed
tlwcao then indoor/outdoor/tlwhome02 overlay tuning; matched original/derived
captures; Linux Release build verification. Windows Debug/Release and WSL GCC
Debug compile checks passed for the current repair slice; Windows focused
Step 9B CPU tests passed 7/7 in each configuration. Windows Debug and Release
were installed after the owner's game closed; see STATUS.md for details. Earlier
exit-code-only GL3+ results are superseded: they missed the blank deferred
image. See STATUS.md for dated actual results. Step 9B is not complete.
