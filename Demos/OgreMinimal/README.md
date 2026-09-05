# Run3 Ogre minimal demo

This is a minimal Ogre 14.6 application built with CMake and Ninja. It selects
Direct3D 11 without showing Ogre's configuration dialog, creates a lit scene,
loads `media/models/cube.mesh` from The Long Way when that content is present,
rotates the model, and exits with Escape.

The game mesh uses Ogre's legacy `MeshSerializer_v1.40` format, which Ogre 14
cannot load directly. During the build, the SDK's `OgreMeshUpgrader` converts a
staged copy to v1.10; the original asset is never modified. If the asset or
converter is absent (or loading still fails), the app falls back to Ogre's
generated cube. CMake also stages Ogre DLLs, SDL2, plugin configuration, and
minimal RTShader resources under `build/`, which is ignored by Git.

## Requirements verified in this workspace

- Visual Studio Community 2026 18.9.2
- MSVC 19.51 x64
- Windows SDK 10.0.28000
- Visual Studio CMake 4.3.1
- Visual Studio Ninja 1.13.2
- Local x64 Ogre SDK 14.6.0 at `../../OgreSDK`

The ordinary PowerShell session may not contain MSVC, CMake, and Ninja on
`PATH`. Open **Developer PowerShell for Visual Studio** or **Developer Command
Prompt for Visual Studio** before building.

## Build and run

From this directory in a Visual Studio developer shell:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc
ctest --preset windows-msvc
./build/windows-msvc/bin/Run3OgreMinimal.exe
```

Use `--smoke-test` to render eight frames and exit automatically:

```powershell
./build/windows-msvc/bin/Run3OgreMinimal.exe --smoke-test
```

The model path can be changed at configure time:

```powershell
cmake --preset windows-msvc -DRUN3_TLW_MODEL=C:/path/to/model.mesh
```

Only `cube.mesh` is currently converted under the fixed runtime name
`assets/models/cube.mesh`. A different model may require its skeleton,
materials, and textures to be converted or staged as well.
