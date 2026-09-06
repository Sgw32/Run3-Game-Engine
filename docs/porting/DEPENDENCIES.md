# Run3 dependency inventory

This living inventory describes the legacy build and source boundaries. Counts
are lexical references in the tracked C/C++ worktree (including the initialized
AIR3 submodule), not ABI analysis. Regenerate them with
`python tools/inventory.py --max-reference-paths 0`.

## Legacy components

| Component | Version evidence | Tracked-source footprint | Legacy evidence and disposition |
|---|---|---:|---|
| Ogre | 1.6.3 in `Run3.log` | 249 files / 2,199 references | Core renderer; replace with a separately pinned Ogre classic 14.x source dependency in Step 2 |
| OIS | Not recorded | 45 / 395 | Linked as `OIS[_d].lib`; replace behind Run3 input boundary |
| Newton and OgreNewt | Not recorded | 97 / 679 | `newton.lib`, `OgreNewt_Main.lib`; replace behind Bullet adapter |
| CEGUI | Not recorded | 6 / 473 | `CEGUIBase` and `OgreGUIRenderer`; replace menu layer, retain only temporary compatibility where required |
| Lua and luabind | Lua 5.0 path in project; luabind version unknown | 26 / 972 | `lua.lib`, `lualib.lib`; later isolate and move to Lua 5.4/sol2 |
| OpenAL and ALUT | OpenAL 1.1 SDK path | 4 / 100 | `OpenAL32.lib`, `alut.lib`; replace with unified audio backend |
| Audiere | 1.9.4 in project path | 2 / 15 | Release links `audiere.lib`; replace with unified audio backend |
| TinyXML | Bundled 2.5.3 constants in `tinyxml.h` | 17 / 1,663 | Bundled sources; replace behind parser tests with TinyXML2 |
| Hydrax | 0.5.1 in project path | 2 / 39 | Release-only library; make water implementation optional |
| SkyX | 0.1 in project path | 3 / 9 | Release-only library; make sky implementation optional |
| NVIDIA Cg | Plugin visible in runtime log | 0 direct source references | Cg programs are content/material driven; remove from required renderer path |
| DirectShow / WMV | `strmiids.lib`, `comsupp.lib` | 3 / 7 | Legacy intro video; disabled/skippable portable fallback |
| Theora video | Debug link input | 1 / 2 | `Plugin_TheoraVideoSystem.lib`; not a required future runtime dependency |
| AIR3 | Gitlink `6fd06acf662ababd4d5092bd14699fa61b370dad` | 10 / 15 | Source-built submodule; keep pinned and isolate from gameplay |
| Win32 optional devices | Win32 APIs/includes | 10 / 31 | Serial/named-pipe and other platform code must be optional with null backends |
| FreeImage | 3.10.0 in runtime log | Transitive/no direct count | Legacy Ogre image codec evidence only |

The source scanner finds no direct C++ Cg API use; this does not mean Cg is
absent. The legacy log loads `Plugin_CgProgramManager` and records Cg shader
compile failures, so the dependency lives primarily in plugins and content.

## Legacy linker inputs

`Run3.vcproj` specifies these direct libraries:

- Common/core: OgreMain, OIS, CEGUIBase, OgreGUIRenderer, OpenAL32,
  OgreNewt_Main, Newton, ALUT, Lua, lualib, and AIR3System.
- Debug-only project entry: `Plugin_TheoraVideoSystem.lib`.
- Release-only project entries: `audiere.lib`, `Hydrax.lib`, `SkyX.lib`,
  `strmiids.lib`, `comctl32.lib`, and `comsupp.lib`.

These names describe the old link contract only. None is a dependency of the
modern root CMake skeleton yet, and the untracked `OgreSDK/` and `Run3Dep/`
directories are not valid reproducible dependency sources.

## Machine-specific path inventory

The old projects contain the following hard-coded path groups:

- Outputs and copies under `F:\VARIOUS_BACKUPS!\Run3v0.72`,
  `C:\Run3v0.72`, and `D:\VARIOUS_BACKUPS!\TheLongWay`.
- Newton and OgreNewt under `D:\Program Files\NewtonSDK` and
  `D:\Program Files\OgreNewt`.
- Lua 5.0 and Audiere 1.9.4 under `D:\OgreSDK\addons`.
- AIR3 under `D:\Program Files\AIR3\GIT-AIR3`.
- Ogre 1.6.3 dependencies under `D:\ogre-v1-6-3`.
- Hydrax 0.5.1 and SkyX 0.1 under `D:\VARIOUS_BACKUPS!`.
- OpenAL 1.1 under `D:\Program Files\OpenAL 1.1 SDK`.
- Visual Studio .NET 2003 VC7, Platform SDK, ATL/MFC, and .NET 1.1 library
  directories under `C:\Program Files (x86)`.
- A solution reference to `F:\VARIOUS_BACKUPS!\src v0.72\Run3.vcproj` and an
  AIR3 output path under `D:\Program Files\AIR3`.
- `$(OGRE_HOME)` plus relative `..\..\..\Dependencies` include/library paths.

The pre-existing runtime log additionally exposes the old Ogre build root
`\CodingExtra\Ogre\Shoggoth_VC7.1` and the game root
`C:\Games\The Long Way\TheLongWay`. The inventory script reports each unique
absolute path with its source location; modern build files must not reproduce
any of them.

## Modern manifest dependency

Step 1 currently has one direct vcpkg dependency: Catch2 3.16.0, selected by
the pinned baseline `04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4`. It is used
only by the isolated build-probe test.

## Update template

When a component changes, add an entry containing:

- old and new version/tag/commit;
- authoritative source URL and source/archive SHA-256;
- vcpkg port, features, target names, and license;
- public/private headers exposing it;
- Windows/Linux verification commands and results;
- removal gate for the legacy component.
