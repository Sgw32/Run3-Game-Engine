# Run3 porting status

Last updated: 2026-09-16

## Milestones

| Step | Status | Notes |
|---|---|---|
| 0 — legacy baseline and rights inventory | Resolved | Static inventory is recorded. By owner direction, recordings and detailed licensing work are deferred to the build-prototype stage; only the authorized `media/` tree is in scope. |
| 1 — reproducible CMake skeleton | Completed | Root CMake/vcpkg build and all four local workflows pass. |
| 2 — pinned Ogre renderer shell | Completed | Ogre classic 14.5.2 is pinned and the installed assetless shell passes Debug and Release smoke tests on Windows and Linux. |
| 3 — controlled legacy compile target | Completed | A reviewed 21/119-source compatibility subset compiled and its smoke tests passed on MSVC and GCC; Step 4 extends it. |
| 4 — platform paths, loop, configuration, and input | Completed | `Run3App` owns the explicit loop; platform-neutral paths/input/clock plus migrated UI/device compatibility batches pass on Windows and Linux. |
| 5 — content manifest, validation, and render fixture | Completed | The read-only validator and deterministic conversion boundary pass their fixtures on D3D11/GL3+; the untouched full content backlog is categorized below. |
| 6A — Bullet physics backend and tests | Completed | Pinned Bullet 3.25#3 and null backends pass the same unit/fixture contract on MSVC and GCC. |
| 6B — static world and player | Completed | The Run3-owned capsule player and static mesh map path run `tlwcao` and `tlwhome02`; deterministic fixtures and real-map 30/60/144 schedules pass. |
| 6C — dynamic physics, contacts, constraints, ragdolls, AIR3 | Completed | Typed dynamic/contact behavior, evidenced constraints, RAII ragdolls, and query-injected AIR3 pass on MSVC/GCC; Newton is absent from the live build. |
| 7 — unified audio | Completed | Run3-owned RAII audio, null and pinned miniaudio 0.11.25 backends, safe device fallback, a live `tlwcao` ambience/music/footstep slice, hashed offline FLAC conversion, and cross-platform tests pass. |

## 2026-09-16 — Step 7 playable-audio and mouse follow-up

Completed:

- Added `MapAudioRuntime` and a tolerant read-only parser for legacy map audio
  declarations. `tlwcao` now starts its nine always-active spatial ambient
  sources, streams `machining.mp3`, and alternates its four concrete footsteps
  while the grounded player moves.
- Named Lua/sequence-controlled sources are reported but deferred, preventing
  alarms and radios from starting before their events. This keeps the Step 8
  scripting boundary explicit.
- Added a text-only miniature audio-map fixture plus deterministic tests for
  spatial parsing, scene scaling, named-source deferral, startup music,
  footstep cadence, noclip suppression, and complete map cleanup. An optional
  content test checks the real attached `tlwcao` references.
- Playable map windows now enable OgreBites/SDL relative mouse mode and window
  grab, hiding the cursor and allowing unlimited yaw in windowed or fullscreen
  mode. Focus loss releases the pointer, focus gain recaptures it, and shutdown
  restores it. Automated bounded runs do not capture the pointer.

Verification:

| Configuration | Result |
|---|---|
| Windows MSVC x64 Debug | Shell/audio targets built; 12/12 focused audio tests passed |
| Windows MSVC x64 Release | Shell/audio targets built; 12/12 focused audio tests passed |
| Linux Ninja Debug (GCC 13.3) | Shell/audio targets built; 12/12 focused audio tests passed |
| Installed Windows Debug/D3D11 `tlwcao` | Real device ready; `ambient=9 failed=0`, nine scripted sources deferred, MP3 music started; 10-frame run exited cleanly |

The complete Windows Debug suite also passed 64/64 tests. The first invocation
was not launched from the Visual Studio developer environment, so CMake's
install dependency scan could not find `objdump`; rerunning the four affected
install/smoke tests from `VsDevCmd.bat` passed 4/4.

The listening test and expected log line are documented in
[RUNNING.md](../RUNNING.md#audible-tlwcao-test). Cursor capture requires a
manual interactive check because bounded smoke runs intentionally leave the
desktop pointer alone.

## 2026-09-16 — Step 7

Completed:

- Added `IAudioEngine`, move-only generation-safe `SoundHandle`, four buses,
  listener/source transforms, playback state/control, loop, gain, pitch,
  attenuation, fade, streaming/seek, stats, and update. Bounded pools default
  to 32 voices and return visible failures instead of overflowing.
- Added deterministic null audio plus pinned miniaudio 0.11.25. The miniaudio
  header and implementation macro are private to one implementation target;
  gameplay/public headers expose no backend types.
- Device initialization failure logs its reason and falls back to null audio.
  `--audio-backend auto|miniaudio|null` is available through CLI/config, and
  an installed-shell CTest forces null audio for device-free gameplay.
- Replaced the live roles of `SoundManager`, `Run3SoundRuntime`, and the
  Audiere `MusicPlayer` with RAII `SoundRuntime`/`MusicPlayer` facades. Their
  three vcproj sources are explicitly retired historical evidence and cannot
  enter the target graph. A boundary test rejects live OpenAL, ALUT, Audiere,
  oalufmod, old link names, and the removed legacy-audio switch.
- Kept Ogre/legacy +Y-up, -Z-forward coordinates through the single
  `fromGameCoordinates` backend boundary. Sources, velocities, listener
  position, forward, and up all pass through it.
- Added error, pool reuse, map cleanup, streaming/loop/transition/fade,
  pitch/time, bus, attenuation, WAV, MP3, converted FLAC, and null-backend
  coverage. ASan exposed a miniaudio 0.11.25 missing-file failure-path
  use-after-free; decoder preflight now rejects missing/corrupt files before
  that resource-manager path, and the sanitizer rerun is clean.
- Added `tools/convert_audio.py`. It found five active OGG/XM/MOD sources, no
  missing files, and no active IT source; FFmpeg 6.1.1 created hashed FLACs in
  ignored `converted-content/audio-flac/`, retained originals, copied license
  metadata, and left all content references unchanged. The author listening
  check remains required before any reference update.

Detailed API, hashes, conversion/listening policy, and the retired-stack
boundary are in [AUDIO.md](AUDIO.md).

Verification:

| Configuration | Toolchain/device path | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64; miniaudio default device ready | Build passed; 62/62 CTests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64; miniaudio default device ready | Build passed; 62/62 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSLg; miniaudio default device ready | Build passed; 62/62 CTests passed |
| `linux-ninja-release` | GCC 13.3 x64 under WSLg; miniaudio default device ready | Build passed; 62/62 CTests passed |
| `linux-ninja-sanitizers` | GCC 13.3 ASan + UBSan | Focused audio targets built; 10/10 audio tests passed with leak detection |

WAV was generated by the fixture; MP3 came from the attached local game
content; FLAC included the converted XM/MOD tracker output. All decoded through
the no-device miniaudio test engine. Installed Debug/Release shells also
opened the real miniaudio device and completed bounded D3D11/GL3+ smoke runs.
No listening approval is claimed, so conversion references remain pending.

## 2026-09-15 — Step 6C

Completed:

- Extended the physics contract with scoped `BodyType`, stable entity/part IDs,
  campaign collision groups, and RAII Bullet/null hinge constraints. Bullet
  user pointers remain backend-owned; gameplay exposes no Bullet headers.
- Added `DynamicPhysicsScene` slices for physical/breakable objects, pickups,
  buttons, triggers, doors, trains, projectile damage, NPC contact, and a
  lifetime-owned ragdoll fallback. Gameplay dispatch drains copied contact
  values only after stepping.
- Main-scene `<phys>` and `<breakable>` objects now use dynamic Bullet box
  bounds and pull interpolated transforms into Ogre. Static structural meshes
  retain the Step 6B triangle path.
- Implemented only point/ball-socket and limited-hinge mappings. Player
  orientation uses its angular lock and doors/trains are kinematic; unused
  fuzzy-test machinery was not ported.
- Added `IPhysicsQuery`, `WorldPhysicsQuery`, and static `run3_air3`. AIR3 path
  search has no Ogre/OgreNewt dependency and is tested through an injected
  obstruction query.
- Classified 37 project-listed Newton translation units as retired historical
  evidence and removed the Newton feature switch from CMake. A CTest scan
  rejects Newton headers, link names, or the retired option in every live
  source/header. Historical files remain unbuilt so unrelated gameplay is not
  destroyed merely to erase migration evidence.
- Added eight Step 6C cases covering contacts, damage, doors/trains, NPC IDs,
  hinge lifetime, AIR3, ragdoll expiry/unload, and attached representative
  content. `tlwcao` and `tlwhome02` are checked for required declared slices;
  this one integration case skips if optional local content is absent.

Mappings and tuning are in [DYNAMIC_PHYSICS.md](DYNAMIC_PHYSICS.md).

Verification:

| Configuration | Toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 | Build passed; 51/51 CTests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 | Build passed; 51/51 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSL | Build passed; 51/51 CTests passed |
| `linux-ninja-release` | GCC 13.3 x64 under WSL | Build passed; 51/51 CTests passed |
| `linux-ninja-sanitizers` | GCC 13.3 ASan + UBSan | Focused targets built; 24/24 physics-labeled tests passed with leak detection |

An installed Windows Debug/D3D11 `tlwcao` run loaded 368 visuals and 166
collision sections, skipped zero, rendered one fixed-step frame, and shut down
cleanly. Sequence XML/Lua binding to the typed factories remains Step 8; it is
not duplicated inside physics. No Step 7 work was started.

## 2026-09-15 — Pre-6C texture and player-height adjustments

Completed:

- Corrected the compatibility catalog's depth classification after visual
  evaluation: the legacy opaque base materials use `scene_blend add` for a
  secondary per-light pass, not for object transparency. The collapsed RTSS
  material now leaves depth writes enabled for those surfaces, while genuine
  `alpha_blend`/`depth_write off` materials remain transparent. This removes
  the far-object-on-top ordering artifact without changing mesh normals or
  source assets.
- Added `fullscreen=true` configuration plus `--fullscreen` and `--windowed`
  CLI overrides. Windowed mode remains the default.
- Removed the Step 6B whole-map `BaseWhite` override. A read-only legacy
  material catalog now resolves diffuse texture aliases, inheritance,
  transparency, and double-sided culling without executing the old
  D3D9/Cg-era programs. Ogre's mesh-serializer listener captures each original
  submesh material name before Ogre can substitute a missing material, and the
  loader creates a small RTSS-compatible material for it. Explicit scene XML
  entity/subentity material overrides are honored as well. Source assets are
  neither modified nor converted.
- Kept a visible white fallback at individual-material granularity. On
  `tlwcao`, 205 textured materials were generated, 195 textures were loaded,
  four material definitions were unresolved, and one referenced texture was
  absent from the source content. On `tlwhome02`, the corresponding counts were
  375, 346, five, and five. These isolated content gaps remain logged instead
  of making the whole map white. Advanced normal/specular/effect parity remains
  Step 9.
- Added the game-facing `player-height-cm` setting and
  `--player-height-cm N` override. It defaults to 180 cm, accepts 120–240 cm,
  and proportionally derives the standing/crouching capsule and camera offsets.
  The persistent setting lives in `<user-root>/config/run3.cfg`.
- Added a portable legacy-material catalog fixture and extended the existing
  configuration-precedence test for the new height option.

Verification:

| Configuration | Toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 | Build passed; 42/42 CTests passed; corrected-depth D3D11 `tlwcao` created an 800x600 fullscreen window, rendered one frame, and shut down cleanly; earlier texture launches covered `tlwhome02` |
| `windows-msvc-x64-release` | MSVC 19.51 x64 | Build passed; 42/42 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSL | Build passed; 42/42 CTests passed; GL3+ `tlwcao` loaded 205 compatible materials and shut down cleanly |
| `linux-ninja-release` | GCC 13.3 x64 under WSL | Build passed; 42/42 CTests passed, including GL3+ smoke |

Exact rebuild, launch, controls, and player-height configuration commands are
in [RUNNING.md](../RUNNING.md). This is a compatibility-material pass, not
Step 6C; no additional legacy gameplay subsystem was migrated.

## 2026-09-14 — Step 6B

Completed:

- Added `run3_gameplay` with a Run3-owned, rotation-locked Bullet capsule
  controller. Its public API contains no OgreNewt, Newton, Bullet, or Ogre
  type. It covers gravity/floor probes, walk/run, edge-triggered jump, duck
  with ceiling rejection, low-step traversal, teleport, explicit parent/train
  displacement, ladder motion, noclip, and filtered use/weapon raycasts.
- Added a tolerant, read-only loader for the structural subset of the legacy
  scene format. It accepts `tlwhome02`/`tlwhome2` and `tlwcao`, reads each
  quality directory's `scene.cfg`, applies scene/node multipliers, derived
  transforms and nonuniform scale, preserves source triangle winding, reverses
  winding for mirrored transforms, and owns one RAII static mesh body per
  colliding section. `<entity>` and `<phys>` collide, `<nocollide>` remains
  visual-only, and invisible blocking boxes collide. Dynamic/breakable scene
  behavior remains Step 6C.
- Added F3 collision-section bounds, N runtime noclip toggle, `--noclip` at
  startup, `--physics-debug`, `--map`, `--map-quality`,
  `--resource-profile`, and deterministic `--render-hz` options. The app logs
  map counts, fixed-step count, and final player position on bounded runs.
- Added eight focused Catch2 behavior cases plus a public-header boundary
  check. The fixture covers indexed triangle floors, standing/falling,
  walk/run/jump/duck, blocked unduck, stairs, ladder, parent motion, teleport,
  noclip, interaction rays, replay repeatability, and 30/60/144 render
  schedules driving the same 60 Hz simulation. `EngineClock` now treats only
  picosecond-scale rounding at a fixed-step boundary as that boundary.
- Loaded the untouched local maps without copying or converting assets.
  `tlwcao` produced 368 visual / 166 collision sections and 199,959 triangles;
  `tlwhome02` produced 990 / 407 and 225,359 triangles. No section was skipped.
  At 30, 60, and 144 render Hz, each one-second run made exactly 60 physics
  steps and each map ended at an identical transform across all three runs.

Verification:

| Configuration | Toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 | Build passed; 41/41 CTests passed; D3D11 real-map schedules passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 | Build passed; 41/41 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSL | Build passed; 41/41 CTests passed, including GL3+ smoke |
| `linux-ninja-release` | GCC 13.3 x64 under WSL | Build passed; 41/41 CTests passed, including GL3+ smoke |

The installed Windows Debug shell was also launched with `--map tlwcao
--noclip --frames 1 --render-hz 60`; D3D11 loaded the map, ran one fixed step,
reported the expected unchanged noclip spawn, and shut down cleanly.

Known Step 6B boundaries:

- The 2026-09-15 compatibility pass now preserves legacy diffuse textures,
  transparency, and culling through generated RTSS materials. Advanced
  normal/specular maps and custom shader effects remain the Step 9 visual
  portability backlog; this milestone does not claim final material parity.
- The parent/train and ladder controller contracts are fixture-tested, but
  moving train bodies, scripted ladder triggers, breakables, doors, weapons,
  and use actions are not instantiated from maps until Step 6C/Step 8.
- `tlwhome02` currently loads as one full scene. Spatial streaming/batching is
  an optimization after correctness; `tlwcao` is the recommended quicker
  first-person evaluation map.

Exact launch and controls are in [RUNNING.md](../RUNNING.md), and design/tuning
details are in [PLAYER_PHYSICS.md](PLAYER_PHYSICS.md). Step 6B is complete;
stop before Step 6C.

## 2026-09-14 — Step 6A

Completed:

- Inventoried the direct OgreNewt/Newton surface from the reviewed vcproj
  runtime, orphan implementation evidence, shared headers, and AIR3. The
  operation-by-operation classification and migration decisions are in
  [PHYSICS_BEHAVIOR.md](PHYSICS_BEHAVIOR.md). No Player, map, entity, NPC,
  weapon, AIR3, joint, or ragdoll call site was migrated.
- Added exact Bullet 3.25 port revision 3 to the existing pinned vcpkg
  baseline. `run3_physics` links only the imported `BulletDynamics`,
  `BulletCollision`, and `LinearMath` targets. Newton/OgreNewt remains disabled
  and no old SDK/library path was added.
- Added a backend-neutral C++17 API containing `PhysicsWorld`, move-only
  `BodyHandle`/`Constraint`, box/capsule/indexed-mesh `Shape`, transforms,
  raycasts, copied contact events, typed metadata, and collision groups/masks.
  Its public headers contain neither Bullet nor Ogre types.
- Centralized the production scale at exactly 0.01 metre/game unit. Alternate
  scales are constructible only through the separate physics testing header.
- Implemented a Bullet backend with an application-owned fixed 60 Hz
  accumulator, bounded catch-up/drop reporting, previous/current transforms,
  deterministic interpolation, nearest-first filtered raycasts, automatic and
  explicit sleep/wake, enable/disable, forces/impulses, RAII cleanup, and
  begin/persist/end contact events copied to a post-step queue.
- Implemented a null backend with the same handle/lifetime and fixed-step API.
  It retains state safely while intentionally producing no simulation,
  raycasts, or contacts.
- Added 13 Catch2 mapping/behavior tests plus a repository boundary test. They
  cover unit conversion, 30/60/144 Hz step equivalence, bounded catch-up,
  gravity, falling/resting, box/capsule/mesh creation, ray order/filtering,
  trigger contacts, force/impulse behavior, sleeping, enable/disable, handle
  and dependent-constraint lifetime, transform interpolation, and null mode.

Verification:

| Configuration | Toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 | Configure/build passed; 32/32 CTests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 | Configure/build passed; 32/32 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSL | Configure/build passed; 32/32 CTests passed |
| `linux-ninja-release` | GCC 13.3 x64 under WSL | Configure/build passed; 32/32 CTests passed |
| `linux-ninja-sanitizers` | GCC 13.3 ASan + UBSan, leak detection | Physics target built; 14/14 focused CTests passed |

The fixed-step tests produce exactly 60 simulation steps for one second fed at
30, 60, or 144 render Hz. A four-step catch-up limit reports and drops excess
whole steps while preserving the fractional accumulator. The header-isolation
test confirms Bullet includes occur only in the private backend translation
unit.

Historical note: Step 6A stopped before Player/static-map migration. Step 6B
is now completed in the entry above.

## 2026-09-14 — Step 5

Completed:

- Added `run3_asset_check` and `run3_shell --validate-content`. Both resolve
  content, manifest, report, and user paths independently of the working
  directory and keep reports/logs outside read-only content.
- Added version 1 of the The Long Way manifest. It records exact Lua 5.4.8 and
  Ogre 14.5.2 compatibility runtimes, scans untracked `run3/` plus `media/`,
  validates all ten legacy resource configurations, and selects
  `resources_high_high.cfg` for active Ogre parsing.
- Added deterministic file size/SHA-256 inventory, exact-case path walking,
  physical case-collision checks, resource-location and ZIP inventory,
  duplicate logical-name checks, reference resolution, TinyXML2 parsing, and
  syntax-only Lua parsing. Lua chunks are never executed.
- Added Ogre-side active-profile program/material/compositor parsing and direct
  serializer imports for every mesh/skeleton. Fixed validation-layer bugs found
  during the full pass: duplicate root registration, writable registration of
  read-only archives, repeated quality-profile reference resolution, duplicate
  logical script parsing, and bulk Ogre diagnostic echo.
- Added and installed a purpose-built `step5.scene`. Its tiny loader uses the
  standard Ogre scene manager to create only ambient light, a point light, and
  an assetless cube; there is no physics or gameplay path.
- Enabled the official `tools` feature on the already pinned Ogre 14.5.2 vcpkg
  dependency. Added `cmake/ConvertOgreAsset.cmake`, which always supplies a
  separate destination, keys output by input/tool/invocation hashes, checks the
  source hash afterward, writes a deterministic receipt, reuses only verified
  output, and refuses overwrites. `converted-content/` is Git-ignored.
- Added miniature valid/invalid content fixtures and tests for SHA-256,
  structural categories, exact Lua/XML parsing, deterministic conversion reuse,
  installed JSON reporting, Ogre parsing, D3D11/GL3+ fixture rendering, and
  clean bounded exit. No real game asset was converted, copied, or edited.
- Documented exact validation and per-file conversion commands in
  [CONTENT_VALIDATION.md](CONTENT_VALIDATION.md), with updated build/run guides.

Full local The Long Way Release baseline (Windows D3D11, untouched content):

| Remaining category | Count |
|---|---:|
| `duplicate-logical-resource` | 11,540 |
| `ogre-script-parse` | 229 |
| `referenced-file-missing` | 451 |
| `resource-path-missing` | 90 |
| `xml-not-well-formed` | 53 |
| `case-mismatch` | 10 |
| `lua-parse` | 4 |
| **Total errors** | **12,377** |

The machine report is local at
`build/step5-reports/the-long-way-windows-release.json` (5,027,088 bytes). It
records 8,344 files; 955 Lua and 193 XML parses; 735 structurally inventoried
Ogre script files; 549 active-profile Ogre script attempts, of which 439
produced no logged parser error; 5,472 resolved references; and successful
header plus Ogre serializer reads for all 670 meshes and 63 skeletons. There
are no `mesh-readability`, `skeleton-readability`, `case-collision`, or
render-fixture failures. The report intentionally fails until later scoped
content/renderer work resolves the categories above; no bulk asset edits were
made to hide them.

Verification:

| Preset | Toolchain/renderer | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 / D3D11 | Configure/build passed; 18/18 CTests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 / D3D11 | Configure/build passed; 18/18 CTests passed; full-content report completed in about 4 minutes |
| `linux-ninja-debug` | GCC 13.3 x64 / GL3+ under WSLg | Configure/build passed; 18/18 CTests passed |
| `linux-ninja-release` | GCC 13.3 x64 / GL3+ under WSLg | Configure/build passed; 18/18 CTests passed |

Both packaged `OgreMeshUpgrader` and `OgreXMLConverter` identify themselves as
Ogre 14.5.2 on Windows and Linux. The conversion-wrapper regression used only
a generated stub fixture below `build/`; no The Long Way conversion was run.
The WSL verification used a fresh pinned checkout in `/tmp` and non-root
temporary `zip`/`unzip` package extraction because this image has no installed
zip tools and interactive sudo is unavailable.

Historical boundary: Step 5 stopped before physics work. Step 6A is now
completed and recorded above.

## 2026-09-07 — Step 4

Completed:

- Replaced the Step 2 `startRendering` shell loop with `run3::Run3App`, which
  explicitly polls OgreBites events, advances `EngineClock`, dispatches input,
  renders one frame, handles resize/focus/quit, and closes Ogre cleanly.
- Added `AppPaths` and layered configuration. Relative paths are anchored at
  the executable, precedence is CLI over user config over content defaults,
  and read-only content is separated from writable `config/`, `saves/`,
  `logs/`, and `cache/` directories.
- Added backend-neutral `Key`, `MouseButton`, `InputEvent`, `InputState`, and
  `IInput` APIs with live queue, null, and deterministic replay inputs. OgreBites
  and SDL-compatible values are translated only in `OgreBitesInputAdapter`.
- Removed OIS declarations from root-project public headers and migrated the
  requested `Run3Input`, `Player`, console, HUD, and `buttonGUI` interfaces,
  plus their Display/MagicManager/weapon input chain. `InputManager2` is now a
  backend-neutral transitional dispatcher.
- Added Run3-owned optional-device interfaces. The default is a logging no-op;
  `RUN3_ENABLE_OPTIONAL_DEVICES=ON` selects Win32 serial/named-pipe sources only
  on Windows. The opt-in Win32 library was compile-checked without opening a
  device. Legacy device classes are portable facades with no Windows types in
  their headers.
- Replaced MessageBox and console-colour/cursor APIs with logging. No game or
  The Long Way asset is required, copied, or modified.
- Grew `run3_legacy` from 21 to 26 of 119 vcproj sources by compiling
  `InputManager2`, `buttonGUI`, `ogreconsole`, `Serial`, and
  `NamedPipeServer`. Eighty-eight runtime units remain deferred behind later
  subsystem work.
- Added seven Catch2 runtime tests covering path confinement, configuration
  precedence, key/button translation, no-op devices, replay/focus/resize
  behavior, and fixed/variable clock policies. The legacy suite now also tests
  replay dispatch. See [PLATFORM_BOUNDARY.md](PLATFORM_BOUNDARY.md).
- Added exact manual configure, compile, test, install, and launch instructions
  for both supported platforms to [BUILDING.md](../BUILDING.md) and
  [RUNNING.md](../RUNNING.md), including CLI/config examples, writable output
  locations, working-directory-independent launches, and troubleshooting.

Verification (repeated 2026-09-09):

| Preset | Toolchain/renderer | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 / D3D11 | Configure/build passed; 13/13 CTests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 / D3D11 | Configure/build passed; 13/13 CTests passed |
| `linux-ninja-debug` | GCC 13.3 x64 / GL3+ under WSLg | Configure/build passed; 13/13 CTests passed |
| `linux-ninja-release` | GCC 13.3 x64 / GL3+ under WSLg | Configure/build passed; 13/13 CTests passed |

The install-and-launch smoke runs use an unrelated working directory and five
frames. Logs are verified at `<user-root>/logs/ogre.log`. Static repository
checks find no OIS or Win32 includes in root-project public headers and no
MessageBox/console-colour API use. The AIR3 submodule was not changed.

Historical boundary: Step 4 stopped before content work. Step 5 is now
completed and recorded above.

## 2026-09-06 — Step 3

Completed:

- Added `run3_legacy`, a static compatibility library based on the explicit
  119-translation-unit inventory from `Run3.vcproj`; no source glob is used.
- Compiled the maximum reviewed dependency-free/Ogre-only subset reached in
  this step: 21 project-listed units covering TinyXML/string support,
  CaduneTree, deferred-render helpers, lens flare, batching/shadows, loading
  overlay, entity spawning, and the Newton-free base entity.
- Kept `main.cpp` separate from reusable code. Explicitly excluded three
  obsolete experimental/console files and one malformed unused TinyXML file;
  no other runtime source was labelled obsolete.
- Added nine default-off feature switches and logging/throwing null backends
  for Newton/OgreNewt, OIS, CEGUI, Hydrax, SkyX, Audiere/ALUT, DirectShow,
  serial, and named pipes. An unimplemented feature cannot be enabled silently.
- Linked the compatibility library only with imported `OgreMain` and
  `OgreOverlay` targets from the pinned manifest. No old binary library or
  local SDK path is used.
- Added two Catch2 compile/link smoke cases and documented source accounting in
  [LEGACY_SOURCE_REVIEW.md](LEGACY_SOURCE_REVIEW.md). Recorded each Ogre API
  batch in [OGRE_API_LEDGER.md](OGRE_API_LEDGER.md).

Verification:

| Preset | Target build | Focused CTest result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 passed | 2/2 passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 passed | 2/2 passed |
| `linux-ninja-debug` | GCC 13.3 x64 passed | 2/2 passed |
| `linux-ninja-release` | GCC 13.3 x64 passed | 2/2 passed |

Remaining backlog:

- 93 runtime translation units are deferred. Of these, 65 directly name at
  least one unavailable dependency; overlapping direct-reference counts are
  Newton/OgreNewt 43, OIS 25, Lua 19, serial 5, SkyX 3, legacy audio 2, named
  pipes 2, and one each for CEGUI, Hydrax, and DirectShow.
- The other 28 are primarily blocked by transitive global/header coupling or
  old Ogre scene-manager/compositor APIs. Raw diagnostic counts are not used
  because a missing dependency header creates cascading errors.
- The compiling layer retains 7 known MSVC and 10 GCC warning sites. A
  tokenizer end-iterator hang discovered by an exploratory test is documented
  but intentionally not behavior-changed without dedicated coverage.
- Step 3 is complete. Stop here; Step 4 remains unstarted.

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

Historical boundary at Step 2 completion:

- Step 2 is complete. No legacy Run3 engine source was added to the build.
- That pass stopped before Step 3. The controlled compatibility target is now
  recorded in the completed Step 3 entry above.

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
