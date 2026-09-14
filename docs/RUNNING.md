# Running the Step 6B first-person shell

`run3_shell` can now load the static geometry and collision for The Long Way's
`tlwcao` and `tlwhome02` maps and place the new capsule player at the map spawn.
Use `tlwcao` for the quickest check; `tlwhome02` (also accepted as
`tlwhome2`) is substantially larger. This is a first-person map viewer and
physics prototype, not yet the complete game: gameplay scripts, doors, trains,
NPCs, weapons, audio, and final materials are later porting steps.

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
`--config Release`.

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

The map currently uses a white RTSS-compatible fallback material. This avoids
known D3D11/GL3+ failures in the legacy fixed-function materials and lets the
geometry and physics be evaluated without changing source assets. Texture and
shader parity is intentionally deferred to Step 9.

## Arguments and configuration

```text
run3_shell [--renderer d3d11|gl3plus] [--frames N]
           [--user-dir PATH] [--content-root PATH]
           [--map tlwcao|tlwhome02] [--map-quality low|medium|high]
           [--resource-profile FILE] [--noclip] [--physics-debug]
           [--render-hz 30|60|144]
           [--validate-content] [--manifest PATH] [--report PATH]
```

`--frames N` exits after exactly N rendered frames. `--render-hz` supplies a
deterministic render schedule for bounded regression runs; omit it for normal
interactive play. Map quality defaults to `low`, paired with
`resources_low_low.cfg`. When selecting another quality, explicitly select the
matching resource profile present in the game root.

Configuration uses `key=value` lines and this precedence:

1. `<content-root>/config/run3.cfg` (lowest)
2. `<user-root>/config/run3.cfg`
3. command-line arguments (highest)

The recognized Step 6B keys are `renderer`, `frames`, `content-root`,
`user-root`, `map`, `map-quality`, `resource-profile`, `render-hz`, `noclip`,
and `physics-debug`. Relative paths resolve from the executable directory, not
the process working directory.

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
