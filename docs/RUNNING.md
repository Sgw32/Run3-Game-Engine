# Running the Run3 first-person shell

`run3_shell` can now load the static geometry and collision for The Long Way's
`tlwcao` and `tlwhome02` maps and place the new capsule player at the map spawn.
Use `tlwcao` for the quickest check; `tlwhome02` (also accepted as
`tlwhome2`) is substantially larger. This is a first-person map viewer and
physics/gameplay prototype, not yet the complete game. Step 6C adds live Bullet motion
for main-scene `<phys>`/`<breakable>` objects and typed/tested physics behavior
for doors, trains, triggers, pickups, projectiles, NPC collision, ragdolls, and
AIR3. Step 7 provides the live miniaudio/null device layer plus the first
`tlwcao` audio compatibility slice: always-active 3D ambience, startup music,
and concrete footsteps. Step 8 will bind the remaining named/event-controlled
sequence and Lua sounds. Step 8D now constructs map NPCs, routes their Lua
events, and provides deterministic movement, collision, animation, voice,
parenting, damage/death, and generic ragdoll slices. NPC head-look/facial pose,
complete sound/combat parity, and final materials remain later work. Step 9A
uses pinned MyGUI for the main menu, HUD, subtitles, console, loading and
inventory presentation, and isolated virtual-computer render textures. CEGUI,
DirectShow, Hydrax, SkyX, Cg, and shader-model-2 programs are not required by
the portable runtime.
Legacy `physPosit` visual offsets and `physSize` collision multipliers are
honoured independently, and sequence-spawned doors, trains, buttons, and NPCs
use the same textured compatibility-material path as static geometry.

Build and test first with [BUILDING.md](BUILDING.md). Run the installed binary,
because `cmake --install` stages the Ogre plugins, runtime libraries,
`plugins.cfg`, and framework media required by the shell. The external game
content remains read-only and is not copied into the installation.

## Windows: build, install, and enter a map

Open an x64 **Developer PowerShell for Visual Studio**, change to the repository
root, and run:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug

$installRoot = Join-Path $PWD 'build\install\windows-debug'
$userRoot = Join-Path $PWD 'build\user\windows-debug'
$contentRoot = Join-Path $PWD 'Games\The Long Way\TheLongWay'
cmake --install build/windows-msvc-x64-debug --prefix $installRoot --config Debug

# Smaller map, recommended first:
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot
```

To load the large map, change only the final map argument:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwhome02 `
  --user-dir $userRoot
```

Add `--fullscreen` to either command to use Ogre's fullscreen window mode.
Use `--windowed` to override a persisted fullscreen setting:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot --fullscreen
```

For Release, use preset `windows-msvc-x64-release`, install from
`build/windows-msvc-x64-release` with `--config Release`, and choose distinct
install/user directories. `--renderer gl3plus` exercises the staged Windows
GL3+ renderer.

## Linux: build, install, and enter a map

In a graphical X11 or Wayland session, change to the repository root and run:

```bash
export VCPKG_ROOT="$HOME/dev/vcpkg"
cmake --preset linux-ninja-debug
cmake --build --preset linux-ninja-debug
ctest --preset linux-ninja-debug

install_root="$PWD/build/install/linux-debug"
user_root="$PWD/build/user/linux-debug"
content_root="$PWD/Games/The Long Way/TheLongWay"
cmake --install build/linux-ninja-debug --prefix "$install_root" --config Debug

# Smaller map, recommended first:
"$install_root/bin/run3_shell" \
  --renderer gl3plus --content-root "$content_root" --map tlwcao \
  --user-dir "$user_root"
```

Use `--map tlwhome02` for the large map. For Release, substitute
`linux-ninja-release`, use distinct Release install/user directories, and pass
`--config Release`. The same `--fullscreen` and `--windowed` switches are
available with GL3+.

## First-person controls

- Mouse: look
- `W`/`S` or Up/Down: forward/backward
- `A`/`D` or Left/Right: strafe
- Left Shift: run
- Space: jump
- Left or Right Ctrl: duck
- `E`: cast the use ray (the hit is logged)
- Left mouse button: cast the weapon ray (the hit is logged)
- `N`: toggle noclip
- `F3`: toggle collision-section bounds
- Escape: open/close the MyGUI menu; while connected to a computer or playing
  a cutscene, disconnect/skip first
- Window close or **Quit** in the menu: quit cleanly

For a playable map, the mouse is captured in SDL relative mode and the OS
cursor is hidden. Relative motion allows unlimited yaw in windowed and
fullscreen modes. Alt-Tab/focus loss releases the pointer; focusing the game
window captures it again. Clean exit also restores it. Bounded `--frames`
smoke runs deliberately do not capture the pointer.

Noclip can be enabled before the map opens by adding `--noclip` to either map
command. In noclip, `W/A/S/D` move horizontally, Space moves up, and Ctrl moves
down. Press `N` to return to collision/gravity at the current location. Add
`--physics-debug` to show collision bounds immediately instead of toggling them
with `F3`.

The map loader preserves legacy diffuse textures through generated
RTSS-compatible materials. It intentionally does not load the old D3D9 shader
programs. Opaque compatibility materials keep depth testing and depth writes;
legacy additive lighting passes are not mistaken for transparency. Advanced
effects, normal/specular maps, and exact shader parity remain a later rendering
task; a missing source texture is reported in `ogre.log` and only that material
falls back to white.

## Arguments and configuration

```text
run3_shell [--renderer d3d11|gl3plus] [--frames N]
           [--user-dir PATH] [--content-root PATH]
           [--map tlwcao|tlwhome02]
           [--scene-quality low|medium|high]
           [--texture-quality low|medium|high]
           [--model-quality low|medium|high]
           [--resource-profile FILE] [--player-height-cm N]
           [--fov 35..120] [--resolution WIDTHxHEIGHT]
           [--fullscreen|--windowed] [--noclip] [--physics-debug]
           [--ui-scale 0.75..3] [--new-game-map NAME]
           [--intro|--skip-intro]
           [--audio-backend auto|miniaudio|null]
           [--render-hz 30|60|144]
           [--validate-content] [--manifest PATH] [--report PATH]
```

`--frames N` exits after exactly N rendered frames. `--render-hz` supplies a
deterministic render schedule for bounded regression runs; omit it for normal
interactive play. `--map-quality` remains a backward-compatible alias for
`--scene-quality`.

For the best content detail currently available, add:

```text
--texture-quality high --model-quality high --scene-quality high
```

The Long Way names its resource profiles
`resources_<texture>_<scene>.cfg`; `medium` maps to the legacy filename token
`med`. Run3 now selects that profile automatically. Thus texture `high` plus
scene `low` selects `resources_high_low.cfg`, while high/high selects
`resources_high_high.cfg`. `--texture-quality` chooses the material tree and
`--scene-quality` chooses the authored map tree. The content has one shared
`run3/models` tree rather than separate low/medium/high mesh files, so
`--model-quality` controls Ogre mesh LOD bias: high retains detailed mesh LODs
farther away, medium preserves Ogre's normal behavior, and low switches sooner.
`--resource-profile FILE` is an expert override and takes precedence over the
automatically selected profile.

For example, run `tlwcao` at the highest available settings, 1920x1080, and a
modern wider view with:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot --resolution 1920x1080 --fov 75 `
  --texture-quality high --model-quality high --scene-quality high
```

`--fov` is vertical field of view in degrees and accepts 35 through 120; the
default is 75. `--resolution` uses `WIDTHxHEIGHT`, defaults to 1280x720, and
must be a mode exposed by the selected renderer/display. Both options work in
windowed and fullscreen modes.

## MyGUI menu, HUD, and computer controls

Start `run3_shell` without `--map` to open the MyGUI main menu immediately, or
press Escape while exploring a map. **Continue** returns to the current map;
**New game** opens `tlwintro` by default; **Chapters** accepts an explicit map
name; **Options** applies resolution and vertical FOV for the current session;
and **Quit** shuts down cleanly. Override the New game destination with
`--new-game-map NAME` or `new-game-map=NAME` in `run3.cfg`.

Use `--ui-scale 1.25` (valid range 0.75 through 3) or persist
`ui-scale=1.25` for high-DPI displays. Layout math is tested at 16:9, 16:10,
4:3, and 2x DPI. The `--intro` flag does not restore DirectShow/WMV: it logs
that the retired intro was skipped and continues. Intro playback remains
disabled by default; `--skip-intro` makes that choice explicit.

To use an authored virtual computer, aim at its use surface and press `E`.
Only that computer receives keyboard/mouse input; its MyGUI layer is rendered
to the attached screen without the main HUD. Press Escape to disconnect. The
engine restores the authored screen material, camera/player state, UI focus,
and first-person mouse capture on exit, failure, map transition, or unload.
Legacy `buttonGUI` computer scripts and the typed `mygui` Lua facade share this
single scoped backend. See [UI_RUNTIME.md](porting/UI_RUNTIME.md) for the API
and teardown contract.

Audio defaults to `auto`: Run3 opens the pinned miniaudio backend and falls
back to the bounded null backend if device initialization fails. Force silent,
device-independent gameplay with:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot --audio-backend null
```

Use `--audio-backend miniaudio` to request the real backend explicitly; device
failure still logs the reason and falls back safely. Persist either choice as
`audio-backend=miniaudio` or `audio-backend=null` in
`<user-root>/config/run3.cfg`. The shell updates the listener position,
velocity, and orientation from the player camera.

## Audible `tlwcao` test

Use the normal `tlwcao` command above and add
`--audio-backend miniaudio`. At the initial spawn you should hear spatial
`indoor2.wav` ambience and `machining.mp3` background music. Walk with `W/A/S/D`
for at least half a second to hear the four concrete samples alternate. Run and
walk have different cadences. Footsteps stop while airborne or in noclip mode.
Move away from the spawn to verify that authored `distance`/`maxDistance`
attenuation changes the ambient mix.

The log file is `<user-root>/logs/ogre.log`. A successful startup contains a
line similar to:

```text
Map audio: ambient=9 failed=0 script-controlled-deferred=9 music=started
```

The deferred sources have `objname` and are controlled by chapter Lua or
sequence events (alarms, radios, and similar sounds). They are intentionally
not all started during map load; Step 8 will activate them through their real
events.

The small repository-only fixture in `tests/fixtures/audio_map` checks two 3D
sources, one deferred source, startup music, multiplier coordinates, footstep
cadence, noclip suppression, and map cleanup without redistributing game audio.
Run it and the attached-content inventory test with:

```powershell
ctest --test-dir build/windows-msvc-x64-debug -C Debug `
  -R "^run3_audio\.(legacy map audio fixture|attached tlwcao)" `
  --output-on-failure
```

```bash
ctest --test-dir build/linux-ninja-debug \
  -R '^run3_audio\.(legacy map audio fixture|attached tlwcao)' \
  --output-on-failure
```

The fixture tests behavior through the deterministic null backend; use the
interactive `tlwcao` launch for the listening and 3D-position check.

The game-facing player defaults to a 180 cm collision capsule with an
approximately 165 cm eye height. Set another physical height from 120 through
240 cm either on the command line:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot --player-height-cm 185
```

or persist it in `<user-root>/config/run3.cfg`:

```ini
player-height-cm=185
```

The crouching capsule and standing/crouching camera offsets are derived from
this value, so the physical body and viewpoint stay in proportion.

Windowed mode is the default. Persist fullscreen mode in
`<user-root>/config/run3.cfg` with:

```ini
fullscreen=true
```

`--fullscreen` and `--windowed` override that setting for the current run.

FOV, resolution, and quality can be persisted in the same file:

```ini
fov=75
resolution=1920x1080
texture-quality=high
model-quality=high
scene-quality=high
ui-scale=1.25
new-game-map=tlwintro
intro=false
```

Configuration uses `key=value` lines and this precedence:

1. `<content-root>/config/run3.cfg` (lowest)
2. `<user-root>/config/run3.cfg`
3. command-line arguments (highest)

The recognized runtime keys are `renderer`, `frames`, `content-root`,
`user-root`, `map`, `scene-quality` (and legacy `map-quality`),
`texture-quality`, `model-quality`, `resource-profile`, `player-height-cm`,
`fov`, `resolution`, `render-hz`, `audio-backend`, `fullscreen`, `noclip`,
`physics-debug`, `ui-scale`, `new-game-map`, and `intro`.
Relative paths resolve from the executable directory, not the process working
directory.

The user root is writable and receives `config/`, `logs/`, `saves/`, and
`cache/`. The content root stays read-only. Running from a different working
directory is therefore supported, for example:

```powershell
Push-Location $env:TEMP
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --content-root $contentRoot --map tlwcao `
  --user-dir $userRoot --noclip
Pop-Location
```

## Content validation and other programs

`run3_asset_check` remains the validation-only entry point. It enables
`--validate-content` and defaults to one frame. Exact report and safe
conversion instructions are in
[CONTENT_VALIDATION.md](porting/CONTENT_VALIDATION.md). Neither executable
modifies the content root.

The Step 1 probe can be launched directly from its build directory:

```text
build/windows-msvc-x64-debug/run3_build_probe.exe
build/linux-ninja-debug/run3_build_probe
```

Use `ctest --preset <preset> -R "^run3_player\." --output-on-failure` for the
focused player tests. If an Ogre plugin cannot be loaded, reinstall the shell
instead of copying individual DLL/shared-library files. On Linux, interactive
rendering requires a working display and OpenGL driver.

Use `ctest --preset <preset> -R "^run3_step6c\." --output-on-failure` for the
dynamic entities, contacts, constraints, AIR3, ragdoll lifetime, unload, and
representative-map fixtures.
