# Running the Step 5 renderer shell and content validator

The current runnable programs are `run3_shell` and `run3_asset_check`. The shell is an Ogre 14.5.2
with the Run3 application, path, clock, configuration, and input boundaries.
It displays a lit rotating cube. Step 5 additionally loads a tiny render-only
fixture and validates external content; this is not yet the legacy Run3
executable or a playable The Long Way build.

Build and test first by following [BUILDING.md](BUILDING.md). Run the installed
binary, not a copied build-tree executable: `cmake --install` stages the exact
plugins, shared libraries, `plugins.cfg`, and Ogre framework media needed by
the shell.

## Windows Debug

Open an x64 **Developer PowerShell for Visual Studio**, change to the
repository root, and run:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug

$installRoot = Join-Path $PWD 'build\install\windows-debug'
$userRoot = Join-Path $PWD 'build\user\windows-debug'
cmake --install build/windows-msvc-x64-debug --prefix $installRoot --config Debug
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --user-dir $userRoot
```

Press Escape or close the window to quit. Add `--frames 120` for an automatic
limited run. Use `--renderer gl3plus` to test the staged Windows GL3+ renderer.

For Release, substitute `windows-msvc-x64-release`, use install/user directory
names ending in `windows-release`, and pass `--config Release`.

## Linux Debug

In a graphical X11 or Wayland session, change to the repository root and run:

```bash
export VCPKG_ROOT="$HOME/dev/vcpkg"
cmake --preset linux-ninja-debug
cmake --build --preset linux-ninja-debug
ctest --preset linux-ninja-debug

install_root="$PWD/build/install/linux-debug"
user_root="$PWD/build/user/linux-debug"
cmake --install build/linux-ninja-debug --prefix "$install_root" --config Debug
"$install_root/bin/run3_shell" \
  --renderer gl3plus --user-dir "$user_root"
```

Press Escape or close the window to quit. Add `--frames 120` for an automatic
limited run. For Release, substitute `linux-ninja-release`, use directory names
ending in `linux-release`, and pass `--config Release`.

## Running independently of the working directory

All relative application paths are anchored at the executable, not the shell's
current directory. For example, this Windows command deliberately launches
from another directory:

```powershell
Push-Location $env:TEMP
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --renderer d3d11 --frames 120 --user-dir $userRoot
Pop-Location
```

The Linux equivalent is:

```bash
cd /tmp
"$install_root/bin/run3_shell" \
  --renderer gl3plus --frames 120 --user-dir "$user_root"
```

The user root is writable and receives:

- `config/ogre.cfg` and optional `config/run3.cfg`
- `logs/ogre.log`
- `saves/`
- `cache/`

The content root is treated as read-only and never receives configuration,
logs, or saves.

## Arguments and configuration

```text
run3_shell [--renderer d3d11|gl3plus] [--frames N]
           [--user-dir PATH] [--content-root PATH]
           [--validate-content] [--manifest PATH] [--report PATH]
```

- `--renderer`: D3D11 by default on Windows and GL3+ on Linux.
- `--frames`: render exactly this many frames and exit; `0` runs interactively.
- `--user-dir`: writable configuration, save, log, and cache root.
- `--content-root`: optional read-only game content root.
- `--validate-content`: validate content and load the installed render fixture.
- `--manifest`: versioned manifest used by validation.
- `--report`: destination for the machine-readable JSON report.
- `--help`: print usage without opening a window.

Configuration uses `key=value` lines. Values are merged in this order:

1. `<content-root>/config/run3.cfg` (lowest priority)
2. `<user-root>/config/run3.cfg`
3. command-line arguments (highest priority)

The recognized keys are `renderer`, `frames`, `content-root`, and `user-root`.
For example:

```ini
# <user-root>/config/run3.cfg
renderer=gl3plus
frames=600
```

Supplying `--renderer d3d11 --frames 120` overrides those two values. Relative
configured paths and command-line paths resolve from the executable directory.

The authorized The Long Way media may be named as an explicit read-only root:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --content-root 'C:\Run3-Game-Engine\Games\The Long Way\TheLongWay\media'
```

Use `run3_asset_check` for validation-only workflows; it enables
`--validate-content` automatically and defaults to one frame. Exact Windows and
Linux commands, report semantics, and safe conversion instructions are in
[CONTENT_VALIDATION.md](porting/CONTENT_VALIDATION.md). Neither executable
modifies the content root.

## Other compiled programs

The build probe can be launched directly from its build directory:

```powershell
build\windows-msvc-x64-debug\run3_build_probe.exe
```

```bash
build/linux-ninja-debug/run3_build_probe
```

Run the complete registered test suite with `ctest --preset <preset>`. To run
only the platform/runtime tests or the legacy compile-smoke tests:

```text
ctest --preset <preset> -R ^run3_runtime\.
ctest --preset <preset> -R ^run3_legacy\.
```

## Manual checks and troubleshooting

For an interactive check, confirm that the cube rotates, resizing preserves
its aspect ratio, focus can be lost and regained without stuck input, and
Escape/window close exits cleanly. Then inspect `<user-root>/logs/ogre.log`.

If `plugins.cfg` is missing, or Ogre cannot load a renderer/plugin, reinstall
the shell rather than copying individual DLL or shared-library files. On Linux,
window creation requires a working `DISPLAY`/Wayland graphical session and
OpenGL driver. Use the five-frame CTest smoke test for the same staged launch
used by automated verification.
