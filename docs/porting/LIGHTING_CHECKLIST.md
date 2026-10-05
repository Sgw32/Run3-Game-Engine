# Manual Step 9B lighting acceptance sheet

The owner runs these cases manually. **Do not launch this matrix automatically
or use a debugger.** Current changes are compile-checked; all runtime/visual
boxes below are deliberately blank. A successful build is not a successful
GPU shader compilation or a campaign pass.

## 1. Build and install (no game execution)

Use an x64 Visual Studio Developer Command Prompt in the repository:

```bat
cmake --build --preset windows-msvc-x64-debug --target run3_shell run3_step9b_tests
cmake --install build/windows-msvc-x64-debug --prefix build/install/windows-debug
```

For Release replace both occurrences of `debug` with `release`. Close your
existing game first so installation can replace the executable. Small CPU-only
checks, optional: `ctest --preset windows-msvc-x64-debug -R run3_step9b --output-on-failure`.
Do not run unrestricted CTest here: other entries launch graphical smokes.

## 2. Paste this helper in PowerShell at the repository root

This defines a command; it does **not** launch anything. Each subsequent
`Test-Lighting` call runs exactly one requested case. It uses separate user
directories, explicit quality/settings, and retains console output and exit code.

```powershell
$LightingRoot = $PWD.Path
$LightingRenderer = 'd3d11'
function Test-Lighting {
    param(
        [Parameter(Mandatory)][string]$Id,
        [Parameter(Mandatory)][string]$Map,
        [Parameter(Mandatory)]
        [ValidateSet('legacy-forward','deferred','pbr','fast-forward')][string]$Pipeline,
        [ValidateSet('d3d11','gl3plus')][string]$Renderer = $LightingRenderer,
        [ValidateSet('off','low','medium','high','ultra')][string]$Shadows = 'medium',
        [int]$Frames = 120,
        [string]$Resolution = '1280x720',
        [double]$Exposure = 1,
        [switch]$Fullscreen,
        [switch]$Reload,
        [string]$Overlay
    )
    $caseDir = Join-Path $LightingRoot "build/manual-lighting/$Id-$Renderer"
    New-Item -ItemType Directory -Force $caseDir | Out-Null
    $runArgs = @(
        '--renderer', $Renderer, '--map', $Map,
        '--content-root', (Join-Path $LightingRoot 'Games/The Long Way/TheLongWay'),
        '--content-variant', 'original', '--user-dir', $caseDir,
        '--lighting-pipeline', $Pipeline, '--shadow-quality', $Shadows,
        '--exposure', $Exposure.ToString([Globalization.CultureInfo]::InvariantCulture),
        '--frames', "$Frames", '--lighting-capture',
        '--texture-quality', 'high', '--model-quality', 'high', '--scene-quality', 'high',
        '--audio-backend', 'null', '--resolution', $Resolution, '--fov', '75'
    )
    $runArgs += $(if ($Fullscreen) { '--fullscreen' } else { '--windowed' })
    if ($Reload) { $runArgs += '--lighting-reload' }
    if ($Overlay) { $runArgs += @('--content-variant','nextgen','--content-overlay',$Overlay) }
    & "$LightingRoot/build/install/windows-debug/bin/run3_shell.exe" @runArgs 2>&1 |
        Tee-Object -FilePath "$caseDir/process.log"
    $exitCode = $LASTEXITCODE
    "Exit code: $exitCode" | Tee-Object -FilePath "$caseDir/result.txt"
    "Evidence: $caseDir"
}
```

Use a new ID suffix on retest (e.g. `1A-r2`) to retain previous evidence.
`--frames 120` counts **rendered frames**, not loading time. Initial loading may
take minutes. Exit 0 plus a visible world and capture is a **load** pass only.
For interactive examination use `-Frames 0`, which enables normal FPS mouse
capture; quit normally to save the final screenshot/report. Bounded tests
intentionally do not capture the player mouse.

## 3. Map/pipeline loading matrix

Start with tlwcao. Test other maps after it loads. Each cell contains
**load / visual** checkboxes; mark PASS, FAIL, or BLOCKED, not just checked.
Run one command at a time. First use `$LightingRenderer = 'd3d11'`; later set
`$LightingRenderer = 'gl3plus'` and repeat selected rows with the same settings.
Record build revision/date/GPU/driver alongside the results.

| ID | Exact command | D3D11 load / visual | GL3+ load / visual | Notes / error |
|---|---|---|---|---|
| 1A | `Test-Lighting 1A tlwcao legacy-forward` | [ ] / [ ] | [ ] / D3D11: mostly OK. Specular component highlight cone(glossy ray) is too big and bright. !!!Important note: if the light comes out of the sight, the shadows on object from this light disappears!!! GL3Plus Lights are absent except the overall map lighting with shadows. GL3Plus far shadows have noticable artifacts even on high quality | no error |
| 1B | `Test-Lighting 1B tlwcao fast-forward` | [ ] / [ ] | [ ] / Similar to 1A, shadows are slightly better and smooth, artifacts are less in GL3Plus, similar in D3D11 - but there are fewer lights and scene looks a bit unlit.  The main issue is too few lights.  | no error |
| 1C | `Test-Lighting 1C tlwcao deferred` | [ ] / [ ] | [ ] / D3D11: Mostly OK, but too bright(controllable by exposure level - good). Very poor performance/FPS. Objects too glossy - if without normal/spec. GL3Plus: Everything is black, FPS and performance is very low. Few materials are sort of unlit, while others are completely black -sometimes with some lighting specular, but without any color|no errors |
| 1D | `Test-Lighting 1D tlwcao pbr` | [ ] / [ ] | [ ] / PBR was working (not very good, but still) before last corrections | Crash: [Run3 error] Ogre main-loop error: Ogre::RuntimeAssertionException::RuntimeAssertionException: mSemantic == OPS_OUT failed. invalid semantic in Ogre::RTShader::Out::Out at C:\dev\vcpkg\buildtrees\ogre\src\v14.5.2-a37f7415e5.clean\Components\RTShaderSystem\include\OgreShaderFunctionAtom.h (line 169) |
| 2A | `Test-Lighting 2A tlwhome02 legacy-forward` | [ ] / [ ] | [ ] / [ ] | |
| 2B | `Test-Lighting 2B tlwhome02 fast-forward` | [ ] / [ ] | [ ] / [ ] | |
| 2C | `Test-Lighting 2C tlwhome02 deferred` | [ ] / [ ] | [ ] / [ ] | |
| 2D | `Test-Lighting 2D tlwhome02 pbr` | [ ] / [ ] | [ ] / [ ] | |
| 3A | `Test-Lighting 3A tlwstations01 legacy-forward` | [ ] / [ ] | [ ] / [ ] | |
| 3B | `Test-Lighting 3B tlwstations01 fast-forward` | [ ] / [ ] | [ ] / [ ] | |
| 3C | `Test-Lighting 3C tlwstations01 deferred` | [ ] / [ ] | [ ] / Good enough, but transparent objects not shown + performance is poor. | |
| 3D | `Test-Lighting 3D tlwstations01 pbr` | [ ] / [ ] | [ ] / [ ] | |
| 4A | `Test-Lighting 4A tlwstations02 legacy-forward` | [ ] / [ ] | Good. Same notes as for 1A / [ ] | |
| 4B | `Test-Lighting 4B tlwstations02 fast-forward` | [ ] / [ ] | [ ] / [ ] | |
| 4C | `Test-Lighting 4C tlwstations02 deferred` | [ ] / [ ] | [ ] / Works on D3D11 - probably even better than legacy forward(still, performance props very significantly), looks good enough. Issue with shadows disappearing when camera moves and objects are out of the sight persists as in 1A. | |
| 4D | `Test-Lighting 4D tlwstations02 pbr` | [ ] / [ ] | [ ] / [ ] | |
| 5A | `Test-Lighting 5A tlwstations03 legacy-forward` | [ ] / [ ] | [ ] / [ ] | |
| 5B | `Test-Lighting 5B tlwstations03 fast-forward` | [ ] / [ ] | [ ] / [ ] | |
| 5C | `Test-Lighting 5C tlwstations03 deferred` | [ ] / [ ] | [ ] / [ ] | |
| 5D | `Test-Lighting 5D tlwstations03 pbr` | [ ] / [ ] | [ ] / [ ] | Crash: [Run3 error] Ogre main-loop error: Ogre::RuntimeAssertionException::RuntimeAssertionException: mSemantic == OPS_OUT failed. invalid semantic in Ogre::RTShader::Out::Out at C:\dev\vcpkg\buildtrees\ogre\src\v14.5.2-a37f7415e5.clean\Components\RTShaderSystem\include\OgreShaderFunctionAtom.h (line 169) |
| 6A | `Test-Lighting 6A tlwdelusion05 legacy-forward` | [ ] / [ ] | [ ] / [ ] | |
| 6B | `Test-Lighting 6B tlwdelusion05 fast-forward` | [ ] / [ ] | [ ] / [ ] | |
| 6C | `Test-Lighting 6C tlwdelusion05 deferred` | [ ] / [ ] | [ ] / [ ] | |
| 6D | `Test-Lighting 6D tlwdelusion05 pbr` | [ ] / [ ] | [ ] / [ ] | |

Do not infer that an authored map exists from its shorthand: if a name is not
present in your selected content variant, mark BLOCKED with that loader error.

## 4. Focused visual/regression cases

Run these individually; none is pre-marked as successful.

| Check | Command | Result | Notes/error |
|---|---|---|---|
| Glass reflection and foliage alpha, forward | `Test-Lighting glass-forward tlwcao legacy-forward -Frames 0` | [ ] | |
| Same glass/foliage, deferred | `Test-Lighting glass-deferred tlwcao deferred -Frames 0` | [ ] | |
| Same glass/foliage, PBR | `Test-Lighting glass-pbr tlwcao pbr -Frames 0` | [ ] | |
| Baseline without shadows | `Test-Lighting shadow-off tlwcao legacy-forward -Shadows off -Frames 0` | [ ] | |
| Directional shadow comparison | `Test-Lighting shadow-high tlwcao legacy-forward -Shadows high -Frames 0` | [ ] | |
| PBR brightness at standard exposure | `Test-Lighting exposure-one tlwcao pbr -Exposure 1 -Frames 0` | [ ] | |
| Compare earlier workaround, same view | `Test-Lighting exposure-tenth tlwcao pbr -Exposure 0.1 -Frames 0` | [ ] | |
| One unload/reload at frame 3 | `Test-Lighting reload-forward tlwcao legacy-forward -Reload` | [ ] | |
| Deferred unload/reload | `Test-Lighting reload-deferred tlwcao deferred -Reload` | [ ] | |
| PBR unload/reload | `Test-Lighting reload-pbr tlwcao pbr -Reload` | [ ] | |
| Fullscreen / alt-tab / return | `Test-Lighting fullscreen tlwcao pbr -Fullscreen -Resolution 1920x1080 -Frames 0` | [ ] | |
| Window resize and menu/computer return | `Test-Lighting resize tlwcao deferred -Frames 0` | [ ] | |
| Large outdoor shadow distance | `Test-Lighting distance tlwhome02 pbr -Shadows high -Frames 0` | [ ] | |
| Existing derived-content selection | `Test-Lighting overlay tlwcao pbr -Overlay "$LightingRoot/derived-content/nextgen-v0"` | [ ] | |

For each visual case check:

- Glass: background visible, reflection moves with the view, not a flat cubemap
  picture; opaque objects behind it remain correctly depth-tested.
- Trees/grilles: alpha holes show the scene, not black rectangles. Examine
  edge pixels and front/back faces.
- Normal maps: grazing light responds to surface detail, without flipped seams.
  Specular mask: highlights vary across the authored surface, not uniformly.
  Compare a material that actually authors these maps; flat normal/white masks
  are intentional no-detail defaults, not evidence of a failure.
- Shadows: moving casters and spotlight contact shadows; check off/high at the
  same camera. Point illumination must remain stable while rotating the camera;
  omnidirectional point shadows remain unsupported until a cubemap or
  dual-paraboloid path replaces Ogre's camera-facing single-map approximation.
- Brightness: same scene quality, time, camera, FOV and exposure. PBR should
  respect the scene ambient colour, but its BRDF/tone curve is not expected to
  pixel-match legacy-forward. Report clipping, loss of detail or required
  exposure workarounds.
- Exit/reload: no exception, no old-map objects/materials, no leftover computer
  surface/menu, and no sustained memory growth over your repeated transitions.

## 5. Optional asset-independent lab

No The Long Way content required. Change pipeline/renderer explicitly and use
different user directories. Inspect transparent/alpha-tested reference objects.

```powershell
& "$LightingRoot/build/install/windows-debug/bin/run3_shell.exe" --lighting-lab --renderer d3d11 --lighting-pipeline legacy-forward --shadow-quality high --frames 120 --lighting-capture --user-dir "$LightingRoot/build/manual-lighting/lab-forward"
& "$LightingRoot/build/install/windows-debug/bin/run3_shell.exe" --lighting-lab --renderer d3d11 --lighting-pipeline deferred --shadow-quality high --frames 120 --lighting-capture --user-dir "$LightingRoot/build/manual-lighting/lab-deferred"
& "$LightingRoot/build/install/windows-debug/bin/run3_shell.exe" --lighting-lab --renderer d3d11 --lighting-pipeline pbr --shadow-quality high --frames 120 --lighting-capture --user-dir "$LightingRoot/build/manual-lighting/lab-pbr"
& "$LightingRoot/build/install/windows-debug/bin/run3_shell.exe" --lighting-lab --renderer d3d11 --lighting-pipeline fast-forward --shadow-quality high --frames 120 --lighting-capture --user-dir "$LightingRoot/build/manual-lighting/lab-fast"
```

Results: forward [ ]; deferred [ ]; PBR [ ]; fast [ ]. Notes: __________.

## 6. Copy this failure/result record

- ID / date / build revision:
- GPU / driver / renderer:
- Exact command:
- Content quality / original or overlay / overlay manifest hash:
- Loaded? PASS / FAIL / BLOCKED:
- Visual checks passed? (glass / foliage / normal / specular / shadow / brightness):
- Exit code:
- Where/when it failed (loading, first frame, use, cutscene, reload, exit):
- Complete error message and preceding relevant lines:
- Evidence folder / screenshot / recording:
- Notes:

Attach `process.log`, `logs/Ogre.log`, and (if present)
`logs/lighting.json` / `logs/lighting.png`. Shader warnings are not necessarily
fatal; missing input semantics, shader compilation errors and exceptions are
failures. Do not edit game assets to suppress them. See [LIGHTING.md](LIGHTING.md)
for current limitations. Compilation alone does not close Step 9B.

