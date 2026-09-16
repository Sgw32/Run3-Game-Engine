# Running the Step 6C first-person shell

`run3_shell` can now load the static geometry and collision for The Long Way's
`tlwcao` and `tlwhome02` maps and place the new capsule player at the map spawn.
Use `tlwcao` for the quickest check; `tlwhome02` (also accepted as
`tlwhome2`) is substantially larger. This is a first-person map viewer and
physics prototype, not yet the complete game. Step 6C adds live Bullet motion
for main-scene `<phys>`/`<breakable>` objects and typed/tested physics behavior
for doors, trains, triggers, pickups, projectiles, NPC collision, ragdolls, and
AIR3. Step 7 provides the live miniaudio/null device layer; Step 8 binds
sequence XML/Lua events to the new audio API. Final materials are a later step.

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
- Escape or window close: quit cleanly

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
           [--map tlwcao|tlwhome02] [--map-quality low|medium|high]
           [--resource-profile FILE] [--player-height-cm N]
           [--fullscreen|--windowed] [--noclip] [--physics-debug]
           [--audio-backend auto|miniaudio|null]
           [--render-hz 30|60|144]
           [--validate-content] [--manifest PATH] [--report PATH]
```

`--frames N` exits after exactly N rendered frames. `--render-hz` supplies a
deterministic render schedule for bounded regression runs; omit it for normal
interactive play. Map quality defaults to `low`, paired with
`resources_low_low.cfg`. When selecting another quality, explicitly select the
matching resource profile present in the game root.

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
`<user-root>/config/run3.cfg`. The shell updates the listener from the camera,
but campaign music/effect events remain quiet until Step 8 connects the legacy
sequence and Lua commands.

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

Configuration uses `key=value` lines and this precedence:

1. `<content-root>/config/run3.cfg` (lowest)
2. `<user-root>/config/run3.cfg`
3. command-line arguments (highest)

The recognized runtime keys are `renderer`, `frames`, `content-root`,
`user-root`, `map`, `map-quality`, `resource-profile`, `player-height-cm`,
`render-hz`, `audio-backend`, `fullscreen`, `noclip`, and `physics-debug`.
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
