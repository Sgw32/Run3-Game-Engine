# Building Run3

The root build compiles the Step 1 probe, the controlled legacy compatibility
library, `run3_shell`, the Step 5 `run3_asset_check`, and the Step 6A
`run3_physics` Bullet/null backend. The shell uses `Run3App` and an
application-owned loop; it does not inspect local `OgreSDK/` or `Run3Dep/`
directories. Ogre classic 14.5.2 and its official conversion tools are restored solely from the pinned vcpkg
manifest; see
[OGRE_VERSION.md](porting/OGRE_VERSION.md).

## Common prerequisites

- Git
- CMake 3.28 or newer
- Ninja
- A C++17 compiler and platform OpenGL development libraries
- vcpkg checked out at the manifest's `builtin-baseline`

The repository pins vcpkg commit
`04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4`. Dependency downloads and builds
are performed by vcpkg manifest mode; do not install Catch2 or Ogre globally.
The commands below must be run from the repository root. See
[RUNNING.md](RUNNING.md) after compiling; `run3_shell` should be launched from
an installed tree because installation stages its Ogre plugins, runtime
libraries, configuration, and framework media.

## Windows MSVC x64

Install Visual Studio with **Desktop development with C++**, CMake, Ninja, and
a Windows SDK. In PowerShell, prepare vcpkg once:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
git -C C:\dev\vcpkg checkout 04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4
C:\dev\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Open **Developer PowerShell for Visual Studio** configured for x64, then run
these exact workflows from the repository root:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake --workflow --preset windows-msvc-x64-debug
cmake --workflow --preset windows-msvc-x64-release
```

To configure, compile, and test Debug manually instead of using the workflow:

```powershell
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug
```

Replace `debug` with `release` for the Release build.

To compile selected targets after configuration:

```powershell
cmake --build --preset windows-msvc-x64-debug --target run3_shell run3_asset_check run3_legacy run3_runtime_tests run3_physics_tests
```

The build-tree executables are placed in
`build\windows-msvc-x64-debug\` (or the corresponding Release directory).
`run3_build_probe.exe` and the Catch2 test executables may be run there, but
install `run3_shell.exe` before launching it normally.

Install and launch Debug from a directory outside the source and build trees:

```powershell
$installRoot = Join-Path $env:TEMP 'run3-install-debug'
$userRoot = Join-Path $env:TEMP 'run3-user-debug'
cmake --install build/windows-msvc-x64-debug --prefix $installRoot --config Debug
Push-Location $env:TEMP
& (Join-Path $installRoot 'bin\run3_shell.exe') --renderer d3d11 --frames 120 --user-dir $userRoot
Pop-Location
```

Use `--renderer gl3plus` to exercise the Windows GL3+ plugin. For Release,
replace both `debug` occurrences with `release` and pass `--config Release`.

## Linux Ninja x64

On Ubuntu or Debian, install the base tools, Ogre's window/OpenGL prerequisites,
and prepare vcpkg once:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git curl zip unzip tar pkg-config \
  autoconf autoconf-archive automake libtool \
  libx11-dev libxft-dev libxext-dev libxrandr-dev libxi-dev libxaw7-dev \
  libgl1-mesa-dev libglu1-mesa-dev libegl1-mesa-dev \
  libwayland-dev libxkbcommon-dev libibus-1.0-dev
git clone https://github.com/microsoft/vcpkg.git "$HOME/dev/vcpkg"
git -C "$HOME/dev/vcpkg" checkout 04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4
"$HOME/dev/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
```

Then run both workflows from the repository root:

```bash
export VCPKG_ROOT="$HOME/dev/vcpkg"
cmake --workflow --preset linux-ninja-debug
cmake --workflow --preset linux-ninja-release
```

To configure, compile, and test Debug manually instead of using the workflow:

```bash
cmake --preset linux-ninja-debug
cmake --build --preset linux-ninja-debug
ctest --preset linux-ninja-debug
```

Replace `debug` with `release` for the Release build.

To compile selected targets after configuration:

```bash
cmake --build --preset linux-ninja-debug --target run3_shell run3_asset_check run3_legacy run3_runtime_tests run3_physics_tests
```

The build-tree executables are placed in `build/linux-ninja-debug/` (or the
corresponding Release directory). `run3_build_probe` and the Catch2 test
executables may be run there, but install `run3_shell` before launching it
normally.

Install and launch Debug from a directory outside the source and build trees:

```bash
cmake --install build/linux-ninja-debug --prefix /tmp/run3-install-debug --config Debug
cd /tmp
/tmp/run3-install-debug/bin/run3_shell --renderer gl3plus --frames 120 \
  --user-dir /tmp/run3-user-debug
```

For Release, replace both `debug` occurrences with `release` and pass
`--config Release`. A graphical X11/Wayland session with working OpenGL is
required to run the shell. Configure/build/unit tests remain available on a
headless host; CMake registers the renderer smoke test only when a display is
available.

## Shell arguments and installed layout

`run3_shell` and `run3_asset_check` accept:

- `--renderer d3d11|gl3plus` (D3D11 defaults on Windows; GL3+ on Linux)
- `--frames N` (`0`, the default, runs until Escape or window close)
- `--user-dir PATH` for the writable user root
- `--content-root PATH` to register an optional read-only content directory
- `--validate-content` to run the versioned content checks from `run3_shell`
- `--manifest PATH` to select a versioned validation manifest
- `--report PATH` to select the machine-readable JSON report

Relative `--user-dir` and `--content-root` values resolve from the executable's
directory, never from the process working directory. Read-only content is kept
under `content-root`; the user root has separate `config/`, `saves/`, `logs/`,
and `cache/` directories. In particular, Ogre writes `config/ogre.cfg` and
`logs/ogre.log`, never into installed or game content. With no explicit content
root, the shell uses only installed Ogre framework media and its built-in cube;
this is the normal smoke-test mode.

Optional configuration files use `key=value` lines. Content defaults are read
from `<content-root>/config/run3.cfg`, then user settings from
`<user-root>/config/run3.cfg`; command-line values win over both. Supported
Step 4 keys are `renderer`, `frames`, `content-root`, and `user-root`. Relative
configured roots and CLI paths are anchored at the executable directory.

The authorized local The Long Way media can be supplied explicitly for later
evaluation without copying it into an installation:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --content-root 'C:\Run3-Game-Engine\Games\The Long Way\TheLongWay\media'
```

For Step 5 validation and the guarded Ogre conversion commands, see
[CONTENT_VALIDATION.md](porting/CONTENT_VALIDATION.md).

`cmake --install` stages the executable, dependent runtime libraries, Ogre
framework media, and `plugins.cfg`. Windows installs D3D11 and GL3+ plugins;
Linux installs GL3+. Both install ParticleFX and STBI. D3D9 and Cg are absent.

## Build options

The committed presets use these defaults:

- `RUN3_BUILD_TESTS=ON` builds and registers the Catch2 test.
- `RUN3_BUILD_TOOLS=ON` builds `run3_build_probe`.
- `RUN3_ENABLE_OPTIONAL_DEVICES=OFF` selects logging no-op serial and named-pipe
  implementations. On Windows, `ON` compiles the Win32 implementations; on
  other platforms it remains a no-op backend.
- `RUN3_BUILD_LEGACY=ON` builds the reviewed Step 3 compatibility subset.
- `RUN3_ENABLE_SANITIZERS=OFF` can be enabled with GCC or Clang to instrument
  first-party physics code and its tests with AddressSanitizer and
  UndefinedBehaviorSanitizer. Standard presets deliberately remain unchanged.

The non-device `RUN3_LEGACY_ENABLE_*` switches remain `OFF`. Turning one on
fails configuration until that retired subsystem has a reproducible
implementation. The old serial/named-pipe switches are rejected in favor of
the single `RUN3_ENABLE_OPTIONAL_DEVICES` gate.
See [LEGACY_SOURCE_REVIEW.md](porting/LEGACY_SOURCE_REVIEW.md) for the exact
source inventory, exclusions, and focused compile-smoke commands.

Override an option during configuration when required, for example:

```bash
cmake --preset linux-ninja-debug -DRUN3_BUILD_TOOLS=OFF
```

Build trees and manifest-installed packages are placed below `build/` and are
ignored by Git. Put personal preset overrides in `CMakeUserPresets.json`, which
is also ignored.

## Physics-only verification

The normal cross-platform physics tests are part of every workflow. To rebuild
and run only them:

```text
cmake --build --preset <preset> --target run3_physics_tests
ctest --preset <preset> -R "^run3_physics\." --output-on-failure
```

On GCC/Clang, configure an isolated sanitizer tree without changing a committed
preset:

```bash
cmake --preset linux-ninja-debug -B build/linux-ninja-sanitizers \
  -DRUN3_ENABLE_SANITIZERS=ON -DRUN3_BUILD_LEGACY=OFF \
  -DRUN3_BUILD_TOOLS=OFF
cmake --build build/linux-ninja-sanitizers --target run3_physics_tests
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 \
  ctest --test-dir build/linux-ninja-sanitizers \
  -R "^run3_physics\." --output-on-failure
```

See [PHYSICS_BEHAVIOR.md](porting/PHYSICS_BEHAVIOR.md) for the unit contract,
legacy operation inventory, and Step 6B/6C migration boundary.
