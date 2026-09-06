# Run3 porting status

Last updated: 2026-09-06

## Milestones

| Step | Status | Notes |
|---|---|---|
| 0 — legacy baseline and rights inventory | Resolved | Static inventory is recorded. By owner direction, recordings and detailed licensing work are deferred to the build-prototype stage; only the authorized `media/` tree is in scope. |
| 1 — reproducible CMake skeleton | Completed | Root CMake/vcpkg build and all four local workflows pass. |
| 2 — pinned Ogre renderer shell | Completed | Ogre classic 14.5.2 is pinned and the installed assetless shell passes Debug and Release smoke tests on Windows and Linux. |

## 2026-09-06 — Step 2

Completed:

- Pinned Ogre classic 14.5.2 exactly through the committed vcpkg baseline and
  override. The dependency is the official `v14.5.2` source archive with its
  SHA-512 recorded in [OGRE_VERSION.md](OGRE_VERSION.md); Ogre-next and local
  SDK directories are not used.
- Added `run3_shell`, an isolated OgreBites/AppContext executable that creates
  a window, scene manager, camera, directional light, and built-in cube. It
  handles resize, Escape/window close, bounded frame runs, and orderly Ogre
  shutdown without compiling a legacy Run3 source file.
- Added working-directory-independent `--renderer`, `--frames`, `--user-dir`,
  and `--content-root` handling. Writable Ogre configuration and logs remain
  separate from read-only installed data and optional game content.
- Linked Ogre solely through imported CMake targets and installed the runtime
  closure through `cmake --install`. Windows stages D3D11 and GL3+; Linux
  stages GL3+. Both platforms stage ParticleFX, STBI, and the ZIP-capable Ogre
  main library. Overlay and RTShaderSystem are linked because the OgreBites
  scene setup uses them.
- Generated an installed `plugins.cfg` per platform and installed only Ogre's
  required framework media. No The Long Way asset is copied or required; its
  authorized `media/` directory remains an optional explicit content root.
- Added an installed, assetless, five-frame CTest smoke test. It checks clean
  exit plus Ogre 14.5.2 and renderer identity in the generated log.

Verification:

| Workflow | Local toolchain and renderer | Result |
|---|---|---|
| `windows-msvc-x64-debug` configure/build/test trio | CMake 4.3.1, Ninja 1.13.2, MSVC 19.51 x64, D3D11 | Passed; 3/3 tests |
| `windows-msvc-x64-release` workflow | CMake 4.3.1, Ninja 1.13.2, MSVC 19.51 x64, D3D11 | Passed; 3/3 tests |
| `linux-ninja-debug` workflow | WSL2 Ubuntu, CMake 3.28.3, Ninja 1.11.1, GCC 13.3, GL3+ | Passed; 3/3 tests |
| `linux-ninja-release` workflow | WSL2 Ubuntu, CMake 3.28.3, Ninja 1.11.1, GCC 13.3, GL3+ | Passed; 3/3 tests |

Release installs were also launched from unrelated temporary working
directories on both hosts with `--frames 5` and no content root. Both exited
cleanly. Their logs identify Ogre 14.5.2 and respectively Direct3D11 and
OpenGL 3+; the Linux WSLg run used Mesa llvmpipe OpenGL 4.5. Installed plugin
configuration contains no D3D9 or Cg entry.

Scope boundary and next step:

- Step 2 is complete. No legacy Run3 engine source was added to the build.
- Stop here; Step 3 remains unstarted.

## 2026-09-05 — Step 1

Completed:

- Added a CMake 3.28 root project with isolated `run3_build_info`,
  `run3_build_probe`, and `run3_build_probe_tests` targets. No legacy engine
  source is part of the target graph.
- Added reusable `run3::warnings` and `run3::warnings_as_errors` interface
  targets for MSVC, GCC, and Clang-family compilers.
- Added the `RUN3_BUILD_TESTS`, `RUN3_BUILD_TOOLS`, and
  `RUN3_ENABLE_OPTIONAL_DEVICES` options. Tests and tools default on; optional
  hardware backends default off.
- Added CMake configure/build/test/workflow presets for Windows MSVC x64 and
  Linux Ninja, each in Debug and Release configurations.
- Added a vcpkg manifest pinned to baseline
  `04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4`. At this baseline the only direct
  dependency, Catch2, resolves to 3.16.0.
- Replaced the obsolete Windows-only building guide with exact Windows and
  Linux manifest-mode workflow commands.
- Ignored root CMake build trees, vcpkg installed trees, in-source generated
  files, and `CMakeUserPresets.json`.

Verification:

| Workflow | Local toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | CMake 4.3.1, Ninja 1.13.2, MSVC 19.51 x64 | Configure/build passed; 1/1 tests passed |
| `windows-msvc-x64-release` | CMake 4.3.1, Ninja 1.13.2, MSVC 19.51 x64 | Configure/build passed; 1/1 tests passed |
| `linux-ninja-debug` | WSL2 Ubuntu, CMake 3.28.3, Ninja 1.11.1, GCC 13.3 | Configure/build passed; 1/1 tests passed |
| `linux-ninja-release` | WSL2 Ubuntu, CMake 3.28.3, Ninja 1.11.1, GCC 13.3 | Configure/build passed; 1/1 tests passed |

All four built `run3_build_probe` executables printed
`Run3 modern build skeleton is operational`. Generated Ninja inputs were also
checked: they reference only the new probe/test sources and pinned vcpkg
packages, with no `OgreSDK/`, `Run3Dep/`, or legacy engine source dependency.

Local verification note: the WSL image lacked `zip` and `unzip`, so temporary
copies were extracted below `/tmp` to bootstrap the pinned vcpkg checkout. The
documented Linux prerequisite command installs both on a normal developer
machine. No temporary tool or dependency file was added to the repository.

Historical boundary at Step 1 completion:

- Step 0 was subsequently resolved by owner direction; manual recordings and
  detailed licensing/provenance work are deferred to the build-prototype stage.
- Step 2 still needed to choose a reproducible Ogre source version. That item
  is now resolved by the completed Step 2 entry above; local `OgreSDK/` and
  `Run3Dep/` trees remain outside the root build.
- The Step 1 pass stopped before adding Ogre or legacy engine sources.

## 2026-09-05 — Step 0 documentation and inventory

Completed in this pass:

- Inspected tracked source, `Run3.vcproj`, the pre-existing local legacy
  `Run3.log`, the authorized The Long Way `media/` tree, and adjacent content
  notices without launching a game or project executable.
- Added [BASELINE.md](BASELINE.md) with stable evidence hashes, legacy runtime
  observations, a behavioral capture matrix, and VM recording/log attachment
  instructions.
- Added [DEPENDENCIES.md](DEPENDENCIES.md) with old linker inputs, lexical
  source-reference counts, pinned AIR3 revision, versions that can be proven,
  migration dispositions, and every group of machine-specific build paths.
- Added [CONTENT_LICENSES.md](CONTENT_LICENSES.md) recording the owner's
  unrestricted-use declaration for the exact local `media/` tree, discovered
  third-party notices, and the requested exceptional-reference policy.
- Added `tools/inventory.py`, a Python-standard-library, cross-platform,
  read-only inventory. It reports counts, sizes, extension totals,
  deterministic tree and optional per-file SHA-256 values, Unicode-aware
  case-colliding paths, legacy absolute build/log paths, project file coverage,
  and major dependency references.
- Added `docs/porting/evidence-local/` to `.gitignore` for local VM artifacts.

Verification:

| Host | Interpreter | Result |
|---|---|---|
| Windows | Python 3.14.6 | Full tracked-source and authorized-media inventory completed |
| WSL2 Ubuntu | Python 3.12.3 | Full inventory completed with matching counts, sizes, case results, and tree SHA-256 values |

Both hosts reported 380 tracked files (7,810,642 bytes) with manifest SHA-256
`a1f8a5501a984d6da9798458ee7e956d6f62b581a5777cd62383ef940f88fc4e` and
246 authorized media files (22,881,252 bytes) with manifest SHA-256
`a556af65053d717176322aa55118d54f2bc9197d1256b738a0f865895de386ec`.
Neither tree has a case-colliding path group. `Run3.vcproj` declares 270 files
(268 C/C++ headers/sources), all present. The script reported 32 unique
absolute paths across build descriptors and the legacy log.

No Run3, The Long Way, demo, tool, or other project executable was launched.
No SDK/game file was copied, converted, modified, deleted, staged, or committed.

Resolution:

- On 2026-09-05 the owner explicitly directed the port to exit Step 0 and
  postpone VM recordings plus detailed licensing/ownership work until the
  build-prototype stage.
- Subsequent content work is limited to
  `Games/The Long Way/TheLongWay/media`. That local tree may be referenced for
  evaluation, but it remains optional and is not copied, installed, or made a
  dependency of the renderer shell.
