# Pinned Ogre dependency

Run3 Step 2 uses **Ogre classic 14.5.2**, port revision 0, from the vcpkg
registry at builtin baseline
`04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4`.

This is the official `v14.5.2` tag from
[OGRECave/ogre](https://github.com/OGRECave/ogre/tree/v14.5.2). The selected
vcpkg port verifies its source archive with SHA-512:

```text
74c83cd4248bce7c8ee603845e753acde0fe2efaa8edb5f3f0205bc43534d933
4acd3e16c0f3836888aa77fc0ed43f739184181548509c59389aac494dac722b
```

`vcpkg.json` also contains an exact `14.5.2` override. CMake requests
`find_package(OGRE 14.5.2 EXACT CONFIG ...)`, and `run3_shell` has a compile-time
version assertion. These independent checks prevent a registry or local package
change from silently selecting another Ogre version.

## Selected package surface

The manifest disables Ogre's default features and enables only `overlay` and
`zip`. The vcpkg port builds OgreBites and RTShaderSystem, the ParticleFX and
STBI plugins, GL3+, and (on Windows) D3D11. ZIP archive support is compiled into
OgreMain. DDS and several GPU texture codecs are also OgreMain functionality;
STBI supplies the shell's external image codec plugin. Assimp, FreeImage,
Direct3D 9, Cg, and Ogre-next are not selected.

The application links only exported CMake targets:

- `OgreMain`
- `OgreBites`
- `OgreOverlay`
- `OgreRTShaderSystem`

The install rules stage the imported `RenderSystem_Direct3D11` (Windows),
`RenderSystem_GL3Plus`, `Plugin_ParticleFX`, and `Codec_STBI` targets. No raw
library filename or machine-specific SDK directory occurs in the target graph.

The untracked `OgreSDK/` and `Run3Dep/` trees are not inspected or consumed by
the root build. In particular, `ogre-next` is a different vcpkg port and must
never replace the classic `ogre` dependency.
