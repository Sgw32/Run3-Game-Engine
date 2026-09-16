# Legacy compatibility source review

Step 3 treats `Run3.vcproj` as the source-of-truth inventory. It contains 270
file entries, including exactly 119 C/C++ translation units. The committed
explicit list in `cmake/Run3LegacySources.cmake` mirrors those 119 entries; no
filesystem glob is used.

## Compiled reusable subset

`run3_legacy` is a static compatibility library. It currently compiles 23 of
the 119 project-listed translation units:

- Bundled support retained (3): `recorder.cpp`, `strings.cpp`, and
  `Tokenizer.cpp`. Step 8 retired the three bundled TinyXML translation units.
- CaduneTree (4): `CTParameters.cpp`, `CTSection.cpp`, `CTSerializer.cpp`, and
  `CTStem.cpp`.
- Ogre deferred-render helpers (5): `AmbientLight.cpp`, `GeomUtils.cpp`,
  `LightMaterialGenerator.cpp`, `MaterialGenerator.cpp`, and `MLight.cpp`.
- Ogre runtime helpers (6): `LensFlare.cpp`, `DefaultAEnt.cpp`,
  `EventEntC.cpp`, `SceneLoadOverlay.cpp`, `Run3Batcher.cpp`, and
  `PSSMShadowListener.cpp`.
- Step 4 platform/input compatibility (5): `InputManager2.cpp`,
  `buttonGUI.cpp`, `ogreconsole.cpp`, `Serial.cpp`, and
  `NamedPipeServer.cpp`.

The new `source/legacy/LegacyFeatures.cpp` and `LegacySmoke.cpp` files are
compatibility glue and are deliberately not counted as vcproj sources.
`run3_legacy` links the Run3-owned core/optional-device/XML targets plus the
reproducible `OgreMain` and `OgreOverlay` imported targets. The old project
libraries and local SDK directories are not searched.

`main.cpp` is recorded separately as `RUN3_LEGACY_ENTRYPOINT_SOURCE`; it is not
compiled into the reusable library. A legacy application executable is not
created during Step 3 because that entrypoint directly couples Win32, Lua, the
old sample framework, and retired middleware.

## Reviewed exclusions

These project-listed files are intentionally outside the future game runtime:

| Files | Count | Disposition |
|---|---:|---|
| `FuzzyTest.cpp`, `FuzzyTest2.cpp` | 2 | Experimental aerial/fuzzy-control demonstrations; not used by The Long Way runtime. |
| `graphics.cpp` | 1 | Retired console-colour helper for the old Eliza utility; its portable compatibility functions emit a logging warning. |
| `tinystr.cpp` | 1 | Malformed, unterminated commented copy of TinyXML's non-STL string implementation. The selected `TIXML_USE_STL` path neither needs nor links it. |

No other translation unit has been labelled an obsolete demo/tool. Of the 96
project-listed units not compiled by `run3_legacy`, 48 remain deferred runtime
work, one is the separately recorded historical entrypoint, four are the
reviewed exclusions above, 37 are retired Newton implementations, and three
are retired audio implementations (`SoundManager.cpp`, `Run3SoundRuntime.cpp`,
and `MusicPlayer.cpp`). Three more are the Step 8-retired TinyXML 1
implementations. Retired sources remain historical behavior evidence; none is
silently discarded or linked.

## Feature gates and null backends

All legacy feature switches default to `OFF`:

| CMake switch | Retired subsystem |
|---|---|
| `RUN3_LEGACY_ENABLE_NEWTON` | Newton/OgreNewt |
| `RUN3_LEGACY_ENABLE_OIS` | OIS |
| `RUN3_LEGACY_ENABLE_CEGUI` | CEGUI/Ogre GUI renderer |
| `RUN3_LEGACY_ENABLE_HYDRAX` | Hydrax |
| `RUN3_LEGACY_ENABLE_SKYX` | SkyX |
| `RUN3_LEGACY_ENABLE_DIRECTSHOW` | DirectShow video path |
| `RUN3_LEGACY_ENABLE_SERIAL` | Retired switch; use `RUN3_ENABLE_OPTIONAL_DEVICES` |
| `RUN3_LEGACY_ENABLE_NAMED_PIPES` | Retired switch; use `RUN3_ENABLE_OPTIONAL_DEVICES` |

The Newton and legacy-audio feature gates were removed after Steps 6C and 7
provided their Run3-owned replacements. `run3::legacy::requireFeature` is the
temporary null-backend boundary for the remaining rows. For a
disabled feature it writes the subsystem name to standard error and throws
`FeatureUnavailable`; it never reports fake success. Enabling a switch during
Step 3 fails configuration because no reproducible implementation has yet
been connected. Bringing a subsystem up later requires adding its dependency,
sources, and validation before removing that gate.

## Deferred backlog counts

After the Step 7 retirement batches, 48 runtime translation units remain
deferred. The
following current lexical counts overlap because the monolithic files often
mix several subsystems:

| Direct blocker | Deferred translation units |
|---|---:|
| Newton/OgreNewt | 0 live; historical consumers remain unbuilt |
| OIS | 0 |
| Lua | 22 |
| Serial | 2 |
| SkyX | 3 |
| Audiere/ALUT/OpenAL | 0 live; 3 implementations retired |
| Named pipes | 1 |
| CEGUI | 1 |
| Hydrax | 1 |
| DirectShow | 1 |

Counts are translation-unit counts rather than raw compiler-error counts: one
missing dependency otherwise produces hundreds of cascading errors. OIS is
now absent from root-project public headers and from the requested migrated
translation units; the AIR3 submodule remains outside this Step 4 edit boundary.

The compiled subset has zero errors on MSVC and GCC. Known warning debt remains
in untouched legacy code, primarily initializer-order and numeric-conversion
diagnostics, unused locals, a non-virtual UI
destructor, and unsafe iterator/arithmetic patterns.
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

The three tests link representative project-listed sources, exercise replay
input through the transitional dispatcher, and verify that every disabled
legacy null backend fails visibly. Debug and Release passed on both MSVC x64
and GCC x64 during Step 4.
