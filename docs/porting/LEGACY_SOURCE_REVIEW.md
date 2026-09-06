# Legacy compatibility source review

Step 3 treats `Run3.vcproj` as the source-of-truth inventory. It contains 270
file entries, including exactly 119 C/C++ translation units. The committed
explicit list in `cmake/Run3LegacySources.cmake` mirrors those 119 entries; no
filesystem glob is used.

## Compiled reusable subset

`run3_legacy` is a static compatibility library. It currently compiles 21 of
the 119 project-listed translation units:

- Bundled support (6): `tinyxml.cpp`, `tinyxmlerror.cpp`,
  `tinyxmlparser.cpp`, `recorder.cpp`, `strings.cpp`, and `Tokenizer.cpp`.
- CaduneTree (4): `CTParameters.cpp`, `CTSection.cpp`, `CTSerializer.cpp`, and
  `CTStem.cpp`.
- Ogre deferred-render helpers (5): `AmbientLight.cpp`, `GeomUtils.cpp`,
  `LightMaterialGenerator.cpp`, `MaterialGenerator.cpp`, and `MLight.cpp`.
- Ogre runtime helpers (6): `LensFlare.cpp`, `DefaultAEnt.cpp`,
  `EventEntC.cpp`, `SceneLoadOverlay.cpp`, `Run3Batcher.cpp`, and
  `PSSMShadowListener.cpp`.

The new `source/legacy/LegacyFeatures.cpp` and `LegacySmoke.cpp` files are
compatibility glue and are deliberately not counted as vcproj sources.
`run3_legacy` links only the reproducible `OgreMain` and `OgreOverlay` imported
targets. The old project libraries and local SDK directories are not searched.

`main.cpp` is recorded separately as `RUN3_LEGACY_ENTRYPOINT_SOURCE`; it is not
compiled into the reusable library. A legacy application executable is not
created during Step 3 because that entrypoint directly couples Win32, Lua, the
old sample framework, and retired middleware.

## Reviewed exclusions

These project-listed files are intentionally outside the future game runtime:

| Files | Count | Disposition |
|---|---:|---|
| `FuzzyTest.cpp`, `FuzzyTest2.cpp` | 2 | Experimental aerial/fuzzy-control demonstrations; not used by The Long Way runtime. |
| `graphics.cpp` | 1 | Win32 console-colour helper for the old Eliza utility; its operations are already no-ops. |
| `tinystr.cpp` | 1 | Malformed, unterminated commented copy of TinyXML's non-STL string implementation. The selected `TIXML_USE_STL` path neither needs nor links it. |

No other translation unit has been labelled an obsolete demo/tool. The
remaining 93 units are deferred runtime work, not silently discarded files.

## Feature gates and null backends

All legacy feature switches default to `OFF`:

| CMake switch | Retired subsystem |
|---|---|
| `RUN3_LEGACY_ENABLE_NEWTON` | Newton/OgreNewt |
| `RUN3_LEGACY_ENABLE_OIS` | OIS |
| `RUN3_LEGACY_ENABLE_CEGUI` | CEGUI/Ogre GUI renderer |
| `RUN3_LEGACY_ENABLE_HYDRAX` | Hydrax |
| `RUN3_LEGACY_ENABLE_SKYX` | SkyX |
| `RUN3_LEGACY_ENABLE_LEGACY_AUDIO` | Audiere/ALUT/OpenAL-era runtime |
| `RUN3_LEGACY_ENABLE_DIRECTSHOW` | DirectShow video path |
| `RUN3_LEGACY_ENABLE_SERIAL` | Win32 serial devices |
| `RUN3_LEGACY_ENABLE_NAMED_PIPES` | Win32 named-pipe controller |

`run3::legacy::requireFeature` is the temporary null-backend boundary. For a
disabled feature it writes the subsystem name to standard error and throws
`FeatureUnavailable`; it never reports fake success. Enabling a switch during
Step 3 fails configuration because no reproducible implementation has yet
been connected. Bringing a subsystem up later requires adding its dependency,
sources, and validation before removing that gate.

## Deferred backlog counts

Of the 93 deferred runtime translation units, 65 directly name at least one
retired or not-yet-reproduced subsystem in the translation unit or its matching
header. The following lexical counts overlap because the monolithic files often
mix several subsystems:

| Direct blocker | Deferred translation units |
|---|---:|
| Newton/OgreNewt | 43 |
| OIS | 25 |
| Lua | 19 |
| Serial | 5 |
| SkyX | 3 |
| Audiere/ALUT/OpenAL | 2 |
| Named pipes | 2 |
| CEGUI | 1 |
| Hydrax | 1 |
| DirectShow | 1 |

The other 28 do not directly name those packages but remain coupled through
shared headers/global state, old compositor/scene-manager APIs, or deferred
owners. This group includes `CustomSceneManager.cpp`, `DeferredShading.cpp`,
`Run3Shadowing.cpp`, `StereoManager.cpp`, callbacks/managers that include
`global.h`, and several Lua/audio wrappers whose retired dependency is
transitive. Counts are translation-unit counts rather than raw compiler-error
counts: one missing header otherwise produces hundreds of cascading errors.

The compiled subset has zero errors on MSVC and GCC. Known warning debt is 7
MSVC sites and 10 GCC sites, primarily old numeric conversions, TinyXML switch
fallthrough, unused CaduneTree locals, and unsafe iterator/arithmetic patterns.
One behavioral probe of `Tokenizer::getTokenNumber` hung under MSVC because
`firstToken` dereferences an end iterator. The compile smoke deliberately does
not fix or exercise that gameplay-era behavior; it is recorded for a later
covered correctness change.

## Validation commands

Build and execute only the compatibility smoke tests with any committed preset:

```text
cmake --build --preset <preset> --target run3_legacy_compile_tests
ctest --preset <preset> -R ^run3_legacy\.
```

The two tests link representative project-listed sources and verify that every
disabled null backend fails visibly. Debug and Release passed on both MSVC x64
and GCC x64 during Step 3.
