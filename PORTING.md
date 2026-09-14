# Run3 modernization and porting plan

Status: proposed roadmap, based on a repository audit on 2026-09-05.

Primary outcome: **The Long Way can be built reproducibly and played from start to finish on the ported engine, first on Windows x64 and then on Linux x64, with no known progression blockers and only documented minor defects.** macOS is a follow-up target once the OpenGL/D3D-independent path is green.

This document is deliberately organized into small, independently verifiable jobs. Each numbered job has a prompt that can be pasted into Codex. Do not ask Codex to execute the entire document in one turn.

Progress checklist (mark a step only when its exit criteria pass):

- [x] 0 — legacy baseline and rights inventory
- [x] 1 — reproducible CMake skeleton
- [x] 2 — pinned Ogre renderer shell
- [x] 3 — controlled legacy compile target
- [x] 4 — platform/input boundary
- [x] 5 — content validation and first rendered map
- [ ] 6A — Bullet backend tests
- [ ] 6B — static world and player
- [ ] 6C — remaining physics and AIR3
- [ ] 7 — unified audio
- [ ] 8 — XML and Lua
- [ ] 9 — UI and visual portability
- [ ] 10 — complete campaign pass
- [ ] 11 — modularity and code-quality ratchet
- [ ] 12 — Windows/Linux CI and packages
- [ ] 13 — measured optimization and release

## 1. What is in the repository today

The important starting facts are:

- `Run3.sln`/`Run3.vcproj` are Visual Studio .NET 2003 projects (`Version="7.10"`), Win32 only, with output paths hard-coded to the original developer's `C:`/`D:`/`F:` drives.
- The released game log identifies the production renderer as **Ogre 1.6.3 (Shoggoth)** and Direct3D 9. The old executable is 32-bit and ships VC7-era runtimes.
- The newly copied `OgreSDK/` reports **14.6.0**, is 64-bit, and contains D3D11, GL3+, Vulkan, Overlay, RTShaderSystem, OgreBites, and OgreBullet artifacts. Its exact source tag/commit is not recorded, so it is useful as a probe but is not yet a reproducible dependency.
- The official Ogre release page currently lists 14.5.2 as the stable release. Do not assume that the untracked 14.6.0 SDK corresponds to an official tag without finding its source commit.
- The main source tree is about 51,654 lines across 293 C++ headers/sources. The old project lists 268 of them; files outside that list may be demos or abandoned code and must not automatically enter the new target.
- Physics is the largest coupling: 92 root source/header files mention Newton/OgreNewt. OIS appears in 42, Lua in 27, and direct CEGUI use is concentrated mostly in `main.cpp`.
- Other runtime dependencies are Lua 5.0 + luabind, OpenAL + ALUT, Audiere 1.9.4, CEGUI, AIR3, Hydrax 0.5.1, SkyX 0.1, Cg, DirectShow, and a small amount of Win32 serial/named-pipe code.
- The Long Way content contains roughly 670 meshes, 527 material files, 955 Lua scripts, 324 WAV files, 26 MP3 files, one OGG file, and a few tracker music files. Its legacy log already contains numerous shader/material compiler errors even on Ogre 1.6.3.
- Startup paths inconsistently use `Run3/...` and `run3/...`; this works on ordinary Windows volumes but fails on a case-sensitive Linux filesystem.
- `Games/`, `OgreSDK/`, `Run3Dep/`, and `Run3Dep.7z` are currently untracked and together occupy about 4.3 GiB. Do not commit these wholesale. `Games/` may also contain content whose redistribution terms differ from the engine's MIT license.
- The Long Way starts chapter one at `tlwhome02`; its map set also includes `background`, `tlwback01`, `tlwback02`, `tlwintro`, `tlwhome01`, `tlwstations01..03`, `tlwdolg`, `tlwcao`, `tlwoutro`, `tlwcredits`, and `tlwdelusion01..05`.

These observations make a big-bang rewrite unsafe. Keep a running, testable vertical slice while replacing one boundary at a time.

| Legacy dependency/path | Migration target | Removal gate |
|---|---|---|
| Ogre 1.6.3 + D3D9 + Cg | Pinned Ogre classic 14.x; D3D11 + GL3+; RTSS/modern shaders | All required maps render without fatal resource/shader errors |
| `.vcproj` + hard-coded library paths | CMake presets + pinned vcpkg manifest | Clean-machine Windows/Linux CI is green |
| Win32/x86 binaries | Native x64 packages | No old DLL is loaded or staged |
| Newton + OgreNewt | Run3 physics API + Bullet 3 | Physics fixtures and campaign behaviors pass |
| OIS | Run3 input API + OgreBites/SDL adapter | No gameplay/public UI header exposes OIS |
| OpenAL/ALUT + Audiere | Run3 audio API + miniaudio | Effects/music/3D audio pass; null audio is safe |
| Lua 5.0 + luabind | Lua 5.4 + sol2 behind `ScriptEngine` | Full script inventory and representative sequences pass |
| TinyXML 1 | TinyXML2 behind Run3 parsers | Golden scene/sequence/save parser tests pass |
| CEGUI | Small Run3/OgreBites menu layer; retain Overlay HUD initially | Main/options/chapter/quit flows pass |
| Hydrax 0.5.1 + SkyX 0.1 | Simple portable adapters; richer effects optional | Every required map has acceptable fallback visuals |
| DirectShow WMV intro | Skippable/no-video default; portable video is optional | Startup has no COM/DirectShow dependency |
| Win32 serial/named pipes | Optional platform backends + null implementations | Default build/run is platform-neutral |

## 2. Fixed decisions and non-goals

Use these decisions unless a short written architecture decision record (ADR) demonstrates a better option.

1. **Renderer:** port to Ogre classic 14.x, not Ogre-next. Ogre classic preserves much more of the Ogre 1.x API. Pin one exact official Ogre tag or source commit before merging the build-system work. The copied 14.6.0 SDK is only a local migration probe until its provenance is known.
2. **Platforms:** Windows x64 is the first playable target; Linux x64 is required before declaring the port complete. macOS/arm64 follows after Linux. Drop Win32/x86.
3. **Render backends:** D3D11 on Windows and GL3+ on Windows/Linux initially. Vulkan is experimental until the game is correct. Remove D3D9 and NVIDIA Cg from the required path.
4. **Build:** CMake presets + Ninja/MSVC, with dependencies described by a pinned vcpkg manifest. Never use global include/library directories or machine-specific absolute paths.
5. **Language:** compile the compatibility port as C++17 first. Move first-party targets to C++20 only after the game is green; do not modernize syntax and behavior in the same patch.
6. **Physics:** direct Bullet 3 backend behind Run3-owned interfaces. Do not expose Bullet or recreate the `OgreNewt` namespace in gameplay headers.
7. **Input/window events:** use OgreBites' SDL-backed event path initially and translate it to Run3-owned input events. This reuses the SDL version selected by the pinned Ogre build and keeps SDL types out of gameplay.
8. **Audio:** replace OpenAL/ALUT and Audiere with one Run3 audio interface and a pinned miniaudio backend. Preserve WAV/MP3 playback, 3D attenuation, looping, pitch/time effects, fades, music streaming, and volume groups. Convert the few used OGG/tracker tracks to FLAC in an offline, reproducible content step, retaining originals and rights metadata.
9. **Scripting:** preserve Lua 5.0 behavior during the first playable milestone, then move to pinned Lua 5.4 + sol2. Do not jump to Lua 5.5 until the existing 955 scripts and bindings pass under 5.4.
10. **XML:** replace bundled TinyXML 1 with TinyXML2 only behind the existing scene/sequence schema tests.
11. **UI:** do not make old CEGUI a permanent dependency. Recreate the small main/options/chapter menu with OgreBites Trays or a thin Run3 UI layer; keep the Ogre Overlay HUD and `buttonGUI` working during the port. Replace OIS types in `buttonGUI` with Run3 input types.
12. **Sky/water/video:** make SkyX, Hydrax, DirectShow, serial hardware, and named pipes optional. Supply simple Ogre sky/water and no-video/null-device fallbacks so these cannot block the game. The single WMV logo can be skipped for the first release.
13. **Refactoring:** no repository-wide rename, file move, formatting pass, ECS conversion, or smart-pointer rewrite before the relevant behavior is covered. Small behavior-preserving refactors are encouraged.

## 3. Target architecture

Dependencies point downward only:

```text
the_long_way (content + campaign rules)
             |
run3_gameplay (player, maps, entities, AI, sequences)
             |
run3_runtime (application loop, service ownership, configuration)
             |
+------------+-------------+-------------+-------------+
| rendering  | physics API | audio API   | input/UI API| scripting API
+------------+-------------+-------------+-------------+
| Ogre 14.x  | Bullet 3    | miniaudio   | OgreBites/  | Lua 5.4 + sol2
| adapter    | backend     | backend     | SDL adapter | backend
+------------+-------------+-------------+-------------+
             |
run3_platform (paths, files, timing, logging, optional devices)
```

Rules:

- Gameplay includes Run3 interfaces and value types, never Bullet, SDL, miniaudio, Win32, or CEGUI headers.
- Ogre types may remain in rendering-facing gameplay during the compatibility port. Gradually move generic values to `run3::Vec3`, `Quat`, and handles when that reduces coupling; do not write a second scene graph.
- `Run3App` owns services using RAII. Replace `global::getSingleton()` one subsystem at a time with an injected `EngineServices`/`GameContext`; do not introduce a new service locator.
- Configuration and content paths are supplied through `AppPaths`; the process working directory must not matter.
- Backends have null/test implementations so unit tests and content validation do not need a window, sound device, or GPU.

## 4. Definition of done

The port is complete only when all of the following are true:

- A clean checkout can configure, build, test, install, and package using documented commands on Windows x64 and Linux x64.
- Dependencies are restored from a lock/baseline; the untracked `OgreSDK/` and `Run3Dep/` are not needed.
- The executable accepts `--content-root`, `--map`, `--renderer`, `--fixed-dt`, `--frames`, `--no-audio`, `--skip-intro`, and a writable `--user-dir` (or equivalent configuration).
- Every campaign map loads with no missing required resource and no fatal material/script error. Asset names work on a case-sensitive filesystem.
- New game, movement, jump/duck, use interactions, doors/buttons, trains, ladders, pickups, damage, NPC progression, weapons used by the campaign, map transitions, save/load, menus, HUD, subtitles, sound, and music pass a written checklist.
- A complete campaign play-through has no progression blocker. A 60-minute representative soak test has no crash and no continuously growing object/body/audio-source count.
- CI builds Debug and Release on Windows and Linux, runs unit/integration tests and assetless smoke tests, and produces installable ZIP/TGZ artifacts. Full-content tests may run locally or in private CI if content cannot be redistributed.
- Compiler warnings have a documented baseline and new warnings fail CI. ASan/UBSan passes on Linux. No first-party include exposes a platform or third-party backend unintentionally.
- Release notes list remaining visual differences and minor bugs. “It starts on my machine” is not completion.

## 5. Tools, downloads, and reference links

Install tools rather than copying SDK files into the source tree.

### Windows developer setup

1. Install the latest serviced [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with **Desktop development with C++**, the current Windows SDK, CMake tools, Ninja, and Git. VS 2022 is 64-bit and supports CMake projects; its current requirements are documented [here](https://learn.microsoft.com/en-us/visualstudio/releases/2022/system-requirements).
2. Install/verify [CMake](https://cmake.org/download/) and [Ninja](https://github.com/ninja-build/ninja/releases). The project should declare CMake 3.28 or newer even if a newer version is installed.
3. Clone and bootstrap [vcpkg](https://github.com/microsoft/vcpkg) in a stable developer-tools directory, not inside this repository:

   ```powershell
   git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
   C:\dev\vcpkg\bootstrap-vcpkg.bat -disableMetrics
   $env:VCPKG_ROOT = 'C:\dev\vcpkg'
   cmake --version
   ninja --version
   cl
   ```

   `VCPKG_ROOT` can be set persistently by the developer. Do not encode `C:\dev\vcpkg` in project files.

### Linux developer setup

Use a currently supported distribution. On Ubuntu/Debian, start with:

```bash
sudo apt update
sudo apt install build-essential clang cmake ninja-build git pkg-config \
  libx11-dev libxrandr-dev libxi-dev libxaw7-dev libgl1-mesa-dev \
  libglu1-mesa-dev libwayland-dev
git clone https://github.com/microsoft/vcpkg "$HOME/dev/vcpkg"
"$HOME/dev/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/dev/vcpkg"
```

The exact system packages depend on the Ogre render systems enabled. Ogre's authoritative [build guide](https://ogrecave.github.io/ogre/api/latest/building-ogre.html) lists its Linux prerequisites.

### Primary references

- [Ogre releases](https://github.com/OGRECave/ogre/releases), [build guide](https://ogrecave.github.io/ogre/api/latest/building-ogre.html), [project setup](https://ogrecave.github.io/ogre/api/latest/setup.html), [API](https://ogrecave.github.io/ogre/api/latest/), and [mesh tools](https://ogrecave.github.io/ogre/api/latest/manual.html)
- [Bullet source/build](https://github.com/bulletphysics/bullet3) and [C++ API](https://pybullet.org/Bullet/BulletFull/annotated.html)
- [vcpkg manifest mode](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode) and [versioning/baselines](https://learn.microsoft.com/en-us/vcpkg/users/versioning)
- [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html), [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html), and [CPack](https://cmake.org/cmake/help/latest/manual/cpack.1.html)
- [miniaudio source](https://github.com/mackron/miniaudio) and [manual](https://miniaud.io/docs/manual/index.html)
- [FFmpeg downloads](https://ffmpeg.org/download.html) for offline developer-only audio conversion; FFmpeg is not a planned runtime dependency
- [Lua 5.4 manual](https://www.lua.org/manual/5.4/) and [sol2](https://github.com/ThePhD/sol2)
- [TinyXML2](https://github.com/leethomason/tinyxml2)
- [CEGUI 0.8.7](https://github.com/cegui/cegui/releases/tag/v0-8-7) only as a format/API migration reference, not the desired final dependency
- [Git LFS](https://git-lfs.com/) if and only if the project has permission to redistribute large game assets
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html), [clang-tidy](https://clang.llvm.org/extra/clang-tidy/), and [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)

Version rule: record an exact `builtin-baseline` in `vcpkg.json`, record any non-vcpkg source tag and SHA-256, and update dependencies in dedicated PRs after CI/play tests. Never put “latest” into a build script.

Expected direct vcpkg ports by the end of the migration are `ogre` (classic), `bullet3`, `catch2`, `lua`, `sol2`, and `tinyxml2`; confirm names/features at the selected baseline instead of copying an untested manifest from this document. SDL normally arrives through OgreBites. Follow miniaudio's upstream recommendation by compiling its pinned `miniaudio.c`/`.h` in a private backend target and include its license and source hash. AIR3 remains source built from the pinned submodule. CMake must consume imported targets, not manually construct include or library paths.

## 6. Execution rules for every step

For every Codex job below:

- Start from a clean branch and inspect `git status`. Preserve unrelated user changes.
- Read this document and do only the requested job.
- Keep each commit buildable if possible. Never mix mass formatting with functional changes.
- Add or update tests with behavior changes.
- Run the exact relevant configure/build/test commands and report commands plus results. If local prerequisites or content are missing, leave a reproducible check and state the blocker; do not claim success.
- Append a short dated entry to `docs/porting/STATUS.md`: completed work, verification, new risks, and the next step. Do not mark a checkbox merely because code was written.
- Stop when the exit criteria are met. Do not continue into the next numbered job.

## 7. Step-by-step roadmap and Codex prompts

### Step 0 — Preserve and measure the legacy baseline (manual + documentation)

The old executable is useful as a behavioral oracle, not as a dependency. Run unknown legacy binaries only in a VM/Windows Sandbox or on a machine where the user accepts the risk; Codex must not launch it automatically.

User actions:

1. Decide whether The Long Way assets may be redistributed. Record their owner/license separately from the engine MIT license.
2. Back up `Games/The Long Way/TheLongWay` and compute a SHA-256 manifest.
3. In an isolated Windows environment, record video/screenshots and copy logs for: launch/menu, New Game, `tlwhome02`, movement/jump/duck/use, one door/button, one train/ladder, one NPC conversation, one map transition, save/load, WAV effect, MP3 and tracker music, pause/options/quit.
4. Record frame rate and player/physics behavior at VSync on/off. The readme already warns that excessive FPS affects the train; this is evidence for a fixed simulation step.

Paste into Codex:

```text
Read PORTING.md, especially Step 0. Do only the baseline documentation/inventory work; do not run any executable. Inspect the tracked source, Run3.vcproj, the legacy Run3.log, and the local content if present. Create docs/porting/STATUS.md, BASELINE.md, DEPENDENCIES.md, and CONTENT_LICENSES.md templates. Add a read-only cross-platform inventory script that reports file counts, extensions, sizes, SHA-256 values, case-colliding paths, absolute build paths, and source references to major legacy dependencies. It must not copy, delete, convert, or commit SDK/game files. Document how the user attaches VM recordings and logs. Verify the script on this workspace and stop.
```

Exit criteria:

- Baseline evidence and its hashes are stored or linked, and ownership/redistribution is explicitly known or marked unresolved.
- All hard-coded paths and old dependencies are inventoried.
- No large binary directory has accidentally entered Git history.

### Step 1 — Establish a reproducible modern build skeleton

Keep existing files in place for now. Derive the initial source list from `Run3.vcproj`; do not glob every `.cpp` in the root.

Paste into Codex:

```text
Read PORTING.md and current porting status. Implement only Step 1: a root CMake build skeleton using CMake >=3.28, CMakePresets.json, CTest, and a pinned vcpkg manifest/baseline. Add Windows MSVC x64 and Linux Ninja Debug/Release presets plus matching build/test workflows. Add first-party warning helper targets, RUN3_BUILD_TESTS, RUN3_BUILD_TOOLS, and RUN3_ENABLE_OPTIONAL_DEVICES options. Do not add the legacy engine sources or depend on untracked OgreSDK/Run3Dep. Build a trivial run3_build_probe and one Catch2 test, add .gitignore entries for build/vcpkg/user preset outputs, and document exact Windows/Linux commands in docs/BUILDING.md. Configure, build, and test every locally possible preset, record results in docs/porting/STATUS.md, then stop.
```

Exit criteria:

- `cmake --workflow --preset <local-preset>` (or the documented configure/build/test trio) succeeds without absolute paths.
- A fresh machine can infer every dependency from committed manifests and documentation.

### Step 2 — Pin Ogre and create a minimal cross-platform renderer shell

Before coding, resolve the Ogre version gate:

- Prefer an exact official stable Ogre classic 14.x tag available through the pinned vcpkg baseline.
- If using the copied 14.6.0 SDK temporarily, record its compiler, architecture, source tag/commit, CMake options, and SHA-256. Do not merge an opaque SDK dependency.
- Do not select `ogre-next` and do not link any DLL from the old game.

Paste into Codex:

```text
Read PORTING.md and do only Step 2. Resolve and document one exact reproducible Ogre classic 14.x version; never use Ogre-next or an unversioned local SDK. Add a small run3_shell executable using OgreBites/AppContext that opens a window, creates a scene manager/camera/light/cube, handles resize and quit, and exits cleanly. Link with imported CMake targets (including Overlay/RTShaderSystem only if used), never raw .lib names. Stage plugins/configuration through cmake --install: D3D11 and GL3+ on Windows, GL3+ on Linux, ParticleFX and required codecs. Add --renderer, --frames, --user-dir, and --content-root arguments with paths independent of the working directory. Add an assetless startup/limited-frame smoke test where the platform permits. Verify Debug and Release locally and update status; stop before adding legacy Run3 code.
```

Exit criteria:

- The installed shell starts from a directory other than the source/build directory.
- Its logs identify the pinned Ogre version and chosen render system.
- D3D9 and Cg are absent from required plugin configuration.

### Step 3 — Add the legacy engine as a controlled compatibility target

The goal is an honest compiler-error backlog, not immediate gameplay.

Paste into Codex:

```text
Read PORTING.md and do only Step 3. Create a run3_legacy compatibility target from the source files actually listed by Run3.vcproj, with an explicit reviewed exclusion list for obsolete demos/tools. Split main.cpp from reusable code but do not move or mass-format files. Add narrowly scoped compile options for legacy code and link only reproducible dependencies. Introduce compile-time feature switches and temporary null stubs for Newton/OgreNewt, OIS, CEGUI, Hydrax, SkyX, Audiere/ALUT, DirectShow, serial, and named pipes so the target can be brought up subsystem by subsystem; stubs must fail visibly when a required feature is called. Port Ogre API errors in small mechanical batches and maintain docs/porting/OGRE_API_LEDGER.md with old API, replacement, affected files, and validation. Do not change gameplay behavior. Get the maximum meaningful target subset compiling on both available compilers, add compile smoke tests, update status with remaining error categories/counts, and stop.
```

Important Ogre migration checks:

- Overlay is now a component; do not assume it is in OgreMain.
- Replace removed sample-framework startup code in `Run3Application.h`/`Run3FrameListener.h` with the Step 2 shell.
- Review shared-pointer casts, iterators, resource group lookups, render target statistics, compositor APIs, mesh buffer access, shadow APIs, and scene manager factory ownership against Ogre 14 documentation.
- Treat `CustomSceneManager` and PCZ/octree reliance as optional until a standard scene manager loads a map.
- Never silence a resource/shader exception with a catch-all just to make a test green.

Exit criteria:

- A documented subset of legacy code compiles in MSVC and Clang/GCC without old binary libraries.
- Remaining failures are categorized by subsystem and are not hidden by uncontrolled preprocessor code.

### Step 4 — Platform paths, loop, configuration, and input boundary

Status: **Completed 2026-09-07.** See
`docs/porting/PLATFORM_BOUNDARY.md` and `docs/porting/STATUS.md`.

Paste into Codex:

```text
Read PORTING.md and do only Step 4. Replace the sample-framework loop and working-directory assumptions with Run3App, AppPaths, EngineClock, and an explicit main loop based on the Step 2 OgreBites shell. Add CLI/config precedence: CLI > user config > content defaults. Keep read-only content separate from saves/logs/config. Define backend-neutral Key, MouseButton, InputEvent, InputState, and IInput interfaces; translate OgreBites/SDL events once at the platform edge. Migrate Run3Input, Player, the console, HUD, and buttonGUI away from OIS types in small compilable batches. Add null/replay input for tests and focus/resize handling. Gate Win32 serial/named-pipe implementations behind RUN3_ENABLE_OPTIONAL_DEVICES and provide no-op implementations elsewhere. Replace MessageBox/console-color uses with logging. Add tests for path resolution, config precedence, key translation, focus loss, and fixed/variable clock behavior. Verify and stop.
```

Exit criteria:

- No gameplay/public UI header includes OIS or Win32 headers.
- Startup, resize, focus, input capture, and clean shutdown work on Windows and Linux shell builds.
- Saves/logs never write into installed content.

### Step 5 — Content manifest, case correctness, meshes, and a simple map

Status: **Completed 2026-09-14.** See
`docs/porting/CONTENT_VALIDATION.md` and `docs/porting/STATUS.md`.

Do not convert original assets in place.

Paste into Codex:

```text
Read PORTING.md and do only Step 5. Implement run3_asset_check and a versioned content manifest for The Long Way. It must validate resource.cfg paths, referenced files, duplicate logical resource names, case mismatches as Linux would see them, XML well-formedness, Lua parseability for the active compatibility runtime, mesh/skeleton readability, and Ogre material/program/compositor parsing. Produce machine-readable JSON plus a concise report. Add a deterministic conversion output directory ignored by Git. Wrap OgreMeshUpgrader/OgreXMLConverter invocations in a script that preserves originals, hashes inputs/outputs, and never rewrites source assets. Make the engine load one low-risk map with a standard scene manager, render-only and without physics/gameplay; use tlwhome02 only if it is simpler than a purpose-built tiny fixture. Fix loader bugs, not hundreds of assets blindly. Add a --validate-content mode and tests with miniature fixtures. Record every remaining content failure by category and stop.
```

Exit criteria:

- The validator can run in CI without full proprietary content by using fixtures, and locally against the full content.
- At least one map/fixture loads and renders on D3D11 and GL3+ with no fatal resource error.
- All conversions are reproducible and originals remain untouched.

### Step 6A — Design and test the Bullet physics backend

Run3 content appears to use centimeters (`Player` is roughly 20 x 100 x 20 and worlds span thousands of units). Use an explicit boundary of **100 game units = 1 meter** unless baseline measurements disprove it. Ogre/gameplay stays in game units; Bullet stays in meters. Never scatter `0.01` conversions through gameplay.

Paste into Codex:

```text
Read PORTING.md and do only Step 6A. Inventory every live OgreNewt operation and classify it: world/step, bodies, shapes, transforms, forces, raycasts, user data/type, material/contact callbacks, freeze/sleep, joints, ragdolls. Write PHYSICS_BEHAVIOR.md and mapping tests before migration. Define small Run3-owned PhysicsWorld/BodyHandle/Shape/Constraint/Raycast/ContactEvent APIs with RAII ownership, collision groups/masks, and one centralized game-unit-to-meter conversion (default 0.01, configurable only for tests). Implement a Bullet 3 backend with a fixed 60 Hz accumulator, bounded catch-up, interpolation-ready transforms, safe queued contact events, and null backend. Add unit tests for conversion, gravity, falling/resting bodies, box/capsule/mesh shapes, raycast ordering/filtering, trigger contacts, forces/impulses, sleeping, and handle lifetime. No gameplay file may include Bullet headers. Verify under sanitizers where available and stop before migrating Player/maps.
```

Exit criteria:

- Physics backend tests pass identically on Windows and Linux within documented numeric tolerances.
- Rendering FPS cannot change the number of 60 Hz simulation steps except through the bounded accumulator policy.

### Step 6B — Static world, player controller, interactions, and raycasts

Paste into Codex:

```text
Read PORTING.md and do only Step 6B. Migrate the static map collision path and Player from OgreNewt to the Run3 physics API/Bullet backend. Build triangle mesh collision once per static map section with correct winding, scale, transforms, and lifetime. Reproduce baseline movement, gravity, jump, duck, stairs, floor tests, noclip, teleport, parent/train motion, ladders, and use/weapon raycasts. Prefer a tested capsule/kinematic-character design over emulating Newton's ellipsoid blindly, but document any behavior change. Add deterministic input-replay tests and debug-draw toggles. Validate a small fixture and tlwhome02 at 30/60/144 rendering FPS with the same fixed simulation. Remove OgreNewt from migrated public headers, update the migration ledger/status, and stop.
```

Exit criteria:

- The player can spawn, stand, walk, run, jump, duck, climb/use a fixture, and cannot fall through the map.
- Recorded final transforms from the same replay agree within a documented tolerance at multiple render rates.

### Step 6C — Dynamic entities, constraints, contacts, ragdolls, and AIR3

Suggested order: `PhysObject`/`Breakable`/pickups, buttons/triggers/doors, rotating/pendulum/train, projectiles/explosions, NPC bodies, then ragdolls and unusual fuzzy-test machinery.

Paste into Codex:

```text
Read PORTING.md and do only Step 6C. Finish the OgreNewt-to-Run3-physics migration in vertical slices with a fixture/integration test per behavior. Map material callbacks to collision groups and queued ContactEvents; preserve body type/user data without unsafe stringly typed pointers. Implement only constraints actually used by The Long Way, documenting Newton-to-Bullet differences and tuned values. Decouple AIR3 path/raycast code from OgreNewt by injecting the physics query interface and build AIR3 from source as a normal CMake target, preferably static. Migrate ragdolls last and add lifetime/map-unload tests. Remove Newton/OgreNewt source, link flags, DLL staging, and headers only after repository searches and the campaign checklist prove no live use. Do not port unused experimental machinery solely because a file exists. Verify, update status, and stop.
```

Exit criteria:

- `rg` finds no live Newton/OgreNewt dependency outside historical documentation.
- Map unload/reload leaves zero bodies/constraints from the prior map.
- Required doors, buttons, trains, ladders, pickups, damage, projectiles, NPC collision, and ragdolls pass fixtures and representative maps.

### Step 7 — Replace the audio stack

Paste into Codex:

```text
Read PORTING.md and do only Step 7. Define IAudioEngine with typed SoundHandle, buses (master/music/effects/voice), listener transform, play/stop/pause, loop, gain, pitch, 3D position/velocity, attenuation distances, fades, streaming, and update. Add null and pinned miniaudio backends; hide miniaudio in one implementation target. Replace SoundManager, Run3SoundRuntime, MusicPlayer, Audiere, OpenAL, ALUT, oalufmod, and manual fixed arrays with RAII handles and bounded pools. Preserve the existing Ogre/game coordinate convention through one conversion function. Add an offline, hashed conversion script for actively referenced OGG/XM/MOD/IT tracks to FLAC; retain originals and license metadata, and update content references only after listening checks. Tests must cover failed device initialization, missing/corrupt files, handle reuse, map cleanup, music streaming/loop/fade, pitch/time shift, and 3D attenuation. Verify WAV, MP3, converted tracker music, and null-audio gameplay, update status, and stop.
```

Exit criteria:

- No old audio DLL/header/link dependency remains.
- Audio device failure never prevents the game from running.
- Long map transitions do not leak sources or decoded buffers.

### Step 8 — Modernize XML and Lua without breaking content

Do XML and Lua in separate commits even though they share this milestone.

Paste into Codex:

```text
Read PORTING.md and do only Step 8. First capture golden parser outputs for representative .scene, sequence, save, facial-animation, config-adjacent XML, including malformed input. Replace bundled TinyXML 1 with TinyXML2 behind Run3 parser functions; preserve schema semantics and improve contextual errors. Then inventory every Lua 5.0 C API call, luabind registration, and global function exposed to the 955 scripts. Create ScriptEngine plus a binding compatibility test that loads/parse-checks every script and snapshots the exported API names/signatures. Move to pinned Lua 5.4 and sol2 in small binding groups, adding explicit compatibility shims only where content uses removed behavior. Sandbox file/OS access to approved content/user roots, add traceback and instruction-budget protection, and never catch and discard script errors. Remove vendored Lua 5.0/luabind only after full-content script checks and representative sequences pass. Verify and stop.
```

Exit criteria:

- All shipped Lua files compile/load or are explicitly documented as intentionally unused/broken.
- Representative chapter, NPC, trigger, music, subtitle, and map-change scripts behave like baseline.
- Malformed XML/Lua reports file, location, and cause without crashing.

### Step 9 — Menus/HUD, shaders, sky/water, and intro-video fallback

This is the visual-parity milestone. Correctness beats exact legacy effects.

Paste into Codex:

```text
Read PORTING.md and do only Step 9. Remove direct CEGUI use from main.cpp by separating menu state/actions from presentation, then implement the required main/new-game/chapter/options/quit UI with the chosen Run3/OgreBites UI adapter. Preserve HUD, subtitles, console, and buttonGUI through Ogre Overlay while removing OIS types. Build a shader/material compatibility matrix and eliminate required Cg/ps_2_0/vs_2_0 programs: use Ogre RTSS for ordinary materials and maintained GLSL/HLSL implementations only for effects that materially affect gameplay. Make shader compile errors test failures for the required set. Replace Hydrax/SkyX with simple portable sky/water adapters first; port richer effects only behind optional backends. Make intro video skippable and disabled by default, removing DirectShow/WMV from the portable runtime. Validate UI scaling at 16:9, 16:10, 4:3, and high DPI. Verify D3D11 and GL3+, update status, and stop.
```

Exit criteria:

- A player can start/continue/quit the game and change supported settings without CEGUI.
- Required scenes have a visible fallback material rather than disappearing when a fancy shader is unavailable.
- Required material/shader set produces zero compiler errors on D3D11 and GL3+.

### Step 10 — The Long Way campaign completion pass

Maintain `docs/porting/TLW_COMPATIBILITY.md` as a matrix with rows for every map and columns for load, visuals, collision, spawn, scripts, interactions, NPCs, audio, transition, save/load, D3D11, and GL3+.

Paste into Codex:

```text
Read PORTING.md and do only Step 10. Create/update the full The Long Way compatibility matrix and an executable smoke manifest. Add --map, --frames, fixed seed, deterministic input replay, structured log, screenshot, and clean-exit support without adding cheats to normal gameplay. Exercise every campaign map in story order on full content, first with null audio and then real audio; fail on crash, missing required resource, fatal shader/script error, invalid spawn, or leaked per-map resources. Fix issues in the smallest responsible subsystem and add a fixture/regression test for each fixed class of bug. Perform and document a complete human play-through covering progression and save/load. Do not mark a map green based only on loading its first frame. Update status and stop when the matrix and open issues are honest.
```

Exit criteria:

- All maps and story transitions are green on Windows/D3D11.
- The full campaign has been completed at least once on the ported engine.
- Linux/GL3+ has at minimum automated map smoke coverage, with every blocker listed for Step 12.

### Step 11 — Modularize and raise code quality after behavior is green

Paste into Codex:

```text
Read PORTING.md and do only Step 11. With the campaign tests green, reorganize code incrementally into the target modules documented here. Enforce target-level dependency direction and include-what-you-use principles; remove MainModules.h and globals gradually by injecting EngineServices/GameContext. Introduce RAII and unique ownership for Ogre objects/listeners, managers, bodies, audio, Lua, XML, and callbacks. Replace macros/NULL/C arrays/unsafe strcpy and platform typedefs with scoped constants, nullptr, containers/spans, and fixed-width types where behavior is covered. Move first-party targets to C++20. Add .clang-format and a conservative .clang-tidy; format only files touched by each commit. Enable MSVC /W4 /permissive- and GCC/Clang -Wall -Wextra -Wpedantic, record a warning baseline, and ratchet it down. Break cycles rather than adding public include paths. Verify the campaign suite after every module move and stop.
```

Exit criteria:

- Architecture matches the dependency diagram and can be explained from CMake targets.
- First-party targets build as C++20; new warnings fail CI; no broad `using namespace` appears in public headers.
- Application shutdown/map unload is ownership-driven rather than a fragile manual singleton order.

### Step 12 — Cross-platform CI, install, and packaging

Paste into Codex:

```text
Read PORTING.md and do only Step 12. Add GitHub Actions (or the repository's selected CI) for pinned Windows MSVC x64 and Linux Clang/GCC configure/build/test matrices, Debug sanitizer coverage, Release install/package, dependency caching keyed by the vcpkg baseline, and artifact/log upload on failure. CI must use CMake presets, not duplicate build logic. Add cmake --install rules and CPack ZIP/TGZ packages containing the executable, required Ogre plugins/runtime libraries, generated configuration, notices/SBOM, and no developer SDK. Make content an explicit separate directory/package controlled by redistribution rights. Set Linux RPATH appropriately and test the installed tree in a clean directory/container. Finish all Linux/GL3+ map blockers. Add macOS/arm64 compile/shell CI as an allowed-experimental job, promoting it only when playable. Update building/release docs and stop.
```

Exit criteria:

- CI is green from a clean checkout and its commands also work locally.
- Packaged binaries run without PATH hacks or source-tree files.
- Windows and Linux meet the Definition of Done; unsupported platforms are labeled accurately.

### Step 13 — Profile, optimize, and release

Do not optimize from intuition; compare identical scenes/build types with VSync off and fixed simulation.

Paste into Codex:

```text
Read PORTING.md and do only Step 13. Establish reproducible CPU/GPU/frame-time/memory benchmarks for menu, tlwhome02, one outdoor map, one NPC-heavy map, and a 60-minute transition soak. Capture median and 95th/99th percentile frame times, draw calls, triangles, physics step time/body count, asset load time, audio voices, and memory high-water mark. Profile Release-with-debug-info before changing code. Optimize measured bottlenecks in separate patches: resource duplication and shader compilation, mesh/texture LOD, batching/instancing, visibility/culling, Bullet broadphase/static meshes/sleeping, allocation churn, Lua call frequency, and audio streaming/pools. Add performance budgets to a non-flaky benchmark report; do not make correctness CI depend on noisy wall-clock thresholds. Run the complete campaign regression and package/license checks, write release notes with remaining known issues, tag the reproducible versions, and stop.
```

Exit criteria:

- There is before/after evidence for every retained optimization.
- No optimization changes fixed-step gameplay results or reintroduces map/resource leaks.
- A release package and complete known-issues list are ready.

## 8. Recommended test pyramid

Keep most tests content-independent so public CI remains fast and legal:

- **Unit:** unit conversion, handles/ownership, fixed-step accumulator, config/path precedence, input mapping, XML schema adapters, Lua bindings, audio state, save serialization.
- **Fixture integration:** a tiny redistributable room with static collision, player, ladder, button, door, trigger, pickup, NPC placeholder, sound, script, map transition, and save point.
- **Asset validation:** full content file/case/reference/parser checks, optionally private/local.
- **Map smoke:** load, simulate scripted frames, screenshot/log/resource counts, unload; all campaign maps.
- **Behavior replay:** deterministic input/seed checks at 30/60/144 render FPS.
- **Human:** campaign progression, subjective physics feel, audio mix, UI, and visual parity.
- **Soak:** repeated map cycling and 60-minute play with body/audio/resource/allocation counters.

Every bug found during the port should become the smallest practical fixture or regression test before it is considered fixed.

## 9. Optimization opportunities after correctness

Likely candidates found during the audit, ordered by expected value rather than implementation excitement:

1. Stop registering every asset directory in one global Ogre resource group; use per-core/per-map groups and unload them deterministically.
2. Detect duplicate resources and case mismatches offline instead of paying for ambiguous lookup at runtime.
3. Convert old meshes once, generate appropriate LOD/tangents, and cache hashes; do not convert at startup.
4. Replace legacy Cg/fixed-function variants with RTSS/common modern materials to reduce shader permutations and startup errors.
5. Batch/instance repeated props and vegetation only after draw-call capture proves it helps.
6. Build one Bullet triangle mesh per static map chunk, use collision masks, sleeping, and sensible broadphase bounds. Keep the fixed step independent of render rate.
7. Stream music and long voice files; cache short effects by content key; cap voices by priority/distance rather than fixed magic IDs.
8. Remove per-frame singleton lookups/string conversions/log spam in hot loops after profiling.
9. Move map parsing/asset preparation off the render loop where Ogre thread-safety permits; submit GPU objects on the render thread.
10. Replace the many catch-all exception handlers with contextual errors. Hidden failures currently make both debugging and performance diagnosis harder.

## 10. Risks and stop conditions

- **Content rights unresolved:** continue engine/fixture work, but do not commit or publish The Long Way assets or old third-party binaries.
- **No reproducible Ogre 14.6 source:** use a pinned official 14.x release; never depend on an opaque binary SDK.
- **Ogre API port grows without a running shell:** stop and restore the last shell/map milestone before refactoring more files.
- **Physics feel diverges:** compare fixed input recordings and measured transforms/velocities, then tune the documented adapter—not individual gameplay call sites.
- **Shaders dominate schedule:** ship portable fallback/RTSS materials first. Visual parity is secondary to a complete playable campaign.
- **An obsolete component blocks Linux:** put it behind a Run3 interface with a null/simple backend. Do not contaminate cross-platform gameplay with `#ifdef _WIN32`.
- **A step creates an enormous diff:** split by subsystem or mechanical transformation and keep tests green between commits.

The safest delivery rhythm is one numbered step per PR, with Steps 6A/6B/6C and Step 8 already split into smaller reviewable commits. A playable vertical slice should remain available throughout the project.
