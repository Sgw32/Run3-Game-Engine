# Ogre + MyGUI demo

This asset-independent demo builds the pinned `mygui` submodule against the
same pinned Ogre classic 14.5.2 package as Run3. It displays a rotating Ogre
cube behind an interactive MyGUI window and button. Escape closes the demo;
the button pauses/resumes the cube.

Initialize the submodule once from the repository root:

```powershell
git submodule update --init mygui
```

The MyGUI build pins its own utf8cpp fetch to v4.1.1. The first configure may
therefore need network access; subsequent builds use the populated build tree
and never update it.

## Windows (MSVC x64, Ninja)

Run these commands in an **x64 Native Tools Command Prompt for VS 2022**:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake -S Demos/MyGUIOgre -B build/demos/mygui-ogre-windows-debug -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_MANIFEST_DIR="$PWD" `
  -DVCPKG_INSTALLED_DIR="$PWD/build/windows-msvc-x64-debug/vcpkg_installed" `
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build/demos/mygui-ogre-windows-debug --target run3_mygui_ogre_demo
Set-Location build/demos/mygui-ogre-windows-debug/bin
./run3_mygui_ogre_demo.exe --renderer d3d11
```

For a bounded automated run, replace the final command with:

```powershell
ctest --test-dir build/demos/mygui-ogre-windows-debug -C Debug --output-on-failure
```

## WSL/Linux (Ninja)

The root Linux preset must already have restored the pinned vcpkg graph. From
the repository root inside WSL:

```bash
export VCPKG_ROOT="$HOME/dev/vcpkg"
cmake -S Demos/MyGUIOgre -B build/demos/mygui-ogre-linux-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_MANIFEST_DIR="$PWD" \
  -DVCPKG_INSTALLED_DIR="$PWD/build/linux-ninja-debug/vcpkg_installed" \
  -DVCPKG_TARGET_TRIPLET=run3-x64-linux \
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/cmake/triplets"
cmake --build build/demos/mygui-ogre-linux-debug --target run3_mygui_ogre_demo
cd build/demos/mygui-ogre-linux-debug/bin
./run3_mygui_ogre_demo --renderer gl3plus
```

WSLg or another working X11/Wayland display is required to open the window.
Use `--smoke-test` or `ctest --test-dir ... --output-on-failure` for a bounded
12-frame run.

