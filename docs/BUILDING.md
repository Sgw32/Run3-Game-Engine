# Building the Run3 renderer shell

The root build compiles the isolated Step 1 probe and the assetless Step 2
`run3_shell`. It deliberately does not compile legacy engine sources or inspect
the untracked `OgreSDK/` and `Run3Dep/` directories. Ogre classic 14.5.2 is
restored solely from the pinned vcpkg manifest; see
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

The equivalent individual commands for Debug are:

```powershell
cmake --preset windows-msvc-x64-debug
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug
```

Replace `debug` with `release` for the Release build.

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

The equivalent individual commands for Debug are:

```bash
cmake --preset linux-ninja-debug
cmake --build --preset linux-ninja-debug
ctest --preset linux-ninja-debug
```

Replace `debug` with `release` for the Release build.

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

`run3_shell` accepts:

- `--renderer d3d11|gl3plus` (D3D11 defaults on Windows; GL3+ on Linux)
- `--frames N` (`0`, the default, runs until Escape or window close)
- `--user-dir PATH` for writable `ogre.cfg`, `ogre.log`, and caches
- `--content-root PATH` to register an optional read-only content directory

Relative `--user-dir` and `--content-root` values resolve from the executable's
directory, never from the process working directory. With no `--content-root`,
the shell uses only installed Ogre framework media and its built-in cube; this
is the normal smoke-test mode.

The authorized local The Long Way media can be supplied explicitly for later
evaluation without copying it into an installation:

```powershell
& (Join-Path $installRoot 'bin\run3_shell.exe') `
  --content-root 'C:\Run3-Game-Engine\Games\The Long Way\TheLongWay\media'
```

Step 2 does not load an asset from that directory. Detailed content work begins
in a later porting step.

`cmake --install` stages the executable, dependent runtime libraries, Ogre
framework media, and `plugins.cfg`. Windows installs D3D11 and GL3+ plugins;
Linux installs GL3+. Both install ParticleFX and STBI. D3D9 and Cg are absent.

## Build options

The committed presets use these defaults:

- `RUN3_BUILD_TESTS=ON` builds and registers the Catch2 test.
- `RUN3_BUILD_TOOLS=ON` builds `run3_build_probe`.
- `RUN3_ENABLE_OPTIONAL_DEVICES=OFF` keeps serial and other optional hardware
  backends out of the portable default configuration.

Override an option during configuration when required, for example:

```bash
cmake --preset linux-ninja-debug -DRUN3_BUILD_TOOLS=OFF
```

Build trees and manifest-installed packages are placed below `build/` and are
ignored by Git. Put personal preset overrides in `CMakeUserPresets.json`, which
is also ignored.
