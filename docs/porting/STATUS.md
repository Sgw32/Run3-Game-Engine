# Run3 porting status

Last updated: 2026-09-26

## Milestones

| Step | Status | Notes |
|---|---|---|
| 0 — legacy baseline and rights inventory | Resolved | Static inventory is recorded. By owner direction, recordings and detailed licensing work are deferred to the build-prototype stage; only the authorized `media/` tree is in scope. |
| 1 — reproducible CMake skeleton | Completed | Root CMake/vcpkg build and all four local workflows pass. |
| 2 — pinned Ogre renderer shell | Completed | Ogre classic 14.5.2 is pinned and the installed assetless shell passes Debug and Release smoke tests on Windows and Linux. |
| 3 — controlled legacy compile target | Completed | The reviewed compatibility subset now contains 23/119 project sources after later migrations; its smoke tests pass on MSVC and GCC. |
| 4 — platform paths, loop, configuration, and input | Completed | `Run3App` owns the explicit loop; platform-neutral paths/input/clock plus migrated UI/device compatibility batches pass on Windows and Linux. |
| 5 — content manifest, validation, and render fixture | Completed | The read-only validator and deterministic conversion boundary pass their fixtures on D3D11/GL3+; the untouched full content backlog is categorized below. |
| 6A — Bullet physics backend and tests | Completed | Pinned Bullet 3.25#3 and null backends pass the same unit/fixture contract on MSVC and GCC. |
| 6B — static world and player | Completed | The Run3-owned capsule player and static mesh map path run `tlwcao` and `tlwhome02`; deterministic fixtures and real-map 30/60/144 schedules pass. |
| 6C — dynamic physics, contacts, constraints, ragdolls, AIR3 | Completed | Typed dynamic/contact behavior, evidenced constraints, RAII ragdolls, and query-injected AIR3 pass on MSVC/GCC; Newton is absent from the live build. |
| 7 — unified audio | Completed | Run3-owned RAII audio, null and pinned miniaudio 0.11.25 backends, safe device fallback, a live `tlwcao` ambience/music/footstep slice, hashed offline FLAC conversion, and cross-platform tests pass. |
| 8 — XML and Lua | Completed | Golden schema adapters, pinned TinyXML2/Lua 5.4/sol2, a sandboxed `ScriptEngine`, 202-name API snapshot, and the 955-script compatibility gate pass with three explicitly broken legacy files. |
| 8B — gameplay scene schema, entity inventory, and ownership | Completed | Side-effect-free map/sequence definitions, exact-case AppPaths resolution, generation-safe map ownership, deferred name resolution, definition-driven StaticMap loading, and all 18 attached low-variant map/sequence inventories pass on MSVC/GCC. |
| 8C — fixed-tick Sequence runtime and interactive entities | Completed | Deterministic map-owned entities, typed script/render/physics/audio hooks, explicit train parenting, selected-quality particles, stable state, and audited teardown pass the Step 8C boundary. Chapter saves and the campaign soak are Step 10 work by owner direction. |
| 8D — map-owned NPC runtime | Partial, verified slice | Typed neutral/enemy construction, AIR3 movement, Lua events, Ogre/Bullet/audio adapters, content counts, and a real `tlwcao` smoke pass; remaining parity gaps are documented. |
| 8E — cutscenes, computers, and authored presentation state | Completed with named Step 9 adapters | Deterministic cutscene/computer control, stable sequence/NPC state, safe live transitions, station train binding/event 26, and complete reviewed tag dispositions pass; final HUD/computer/effect drawing is delegated to Step 9A and lighting/material output to Step 9B. |

## 2026-09-26 — delayed ParticleFX renderer crash regression

The delayed D3D11 exits on `tlwstations01` and `tlwcao` were not caused by the
reported HLSL narrowing warnings or by `air.lua`. Enabling authored particles
had exposed a compatibility-material defect: the material catalogue discarded
legacy `lighting off`, so RTShader generated a vertex shader requiring
`NORMAL0` for ParticleFX billboards, whose vertex declaration has no normals.
The resulting rendering exception was then hidden by unloading the render
plugin while that plugin-owned exception was still unwinding.

Legacy lighting state is now preserved through material inheritance and used
when compatibility aliases are generated, keeping smoke/flare/strobe particle
materials unlit. Main-loop Ogre exceptions are converted to diagnostics before
plugins unload, so a future rendering failure exits cleanly with its actual
cause. No The Long Way file was changed.

Verification on this workspace: the exact `tlwstations01` train run now passes
`air.lua`, `RunLSD`, and the formerly failing `DUniverse1` emission point and
completes 1,800 fixed ticks; high-quality `tlwcao` completes 900 fixed ticks.
Both installed Debug D3D11 runs exit 0. The MSVC Debug suite passes all 124
tests (120 in the initial run and the four install-dependent tests after
entering the Visual Studio developer environment), and the focused MSVC
Release material tests pass 2/2.

## 2026-09-25 — Step 8C completion: train seats, effects, and teardown

Step 8C is complete at its documented gameplay boundary. `setCameraParent`
now creates an explicit player/train relation instead of relying on an
incidental floor ray. While attached, the capsule is pinned to the authored
seat and receives the train's fixed-tick translation; full parenting also
rotates the saved local offset. Reset, map change, and unload release the
relation. Noclip deliberately remains free-flight while retaining the binding
for restoration. Train acceleration follows the legacy update order, terminal
trains stop, and state format 4 preserves speed, acceleration, parent mode,
and local offset while continuing to read older snapshots.

Station trains now construct their nested visual, colliding, non-colliding,
and particle parts under one owner. Material mutation reaches static map
objects, runtime entities, and named train parts. The selected texture-quality
`.particle` scripts are parsed only after compatible material aliases exist;
authored DotScene and runtime-created particle systems can be toggled and are
destroyed with the map. Existing NPC/sequence animation commands remain typed
and map scoped. Final shader/compositor/HUD quality is still Step 9
presentation work, not a second gameplay implementation.

Teardown now runs an idempotent runtime destroy even after partial load and
audits presentations, child parts, particles, physics bindings, audio handles,
attachments, ragdolls, and the Ogre runtime root before releasing the map.
StaticMap teardown is audited separately for entities, authored particles,
physics bodies, and its Ogre map root.
The fixture repeats construction/unload 100 times and verifies zero retained
service objects. A zero-length AIR3 ray guard also prevents duplicate authored
AI nodes from tripping Bullet's Debug assertion; this was the previously silent
`tlwstations02` startup breakpoint.

Verification on this workspace:

- Windows MSVC Debug and Release focused physics/player/Step 8C/8E suites pass
  48/48 in each configuration.
- The complete CTest suite passes 124/124 in both MSVC Debug and Release,
  including install and shell smoke tests.
- Installed Debug D3D11 runs of `tlwstations01`, `tlwstations02`, and
  `tlwstations03` load startup Lua, multipart trains, NPC commands, materials,
  audio, and particles, render bounded frames, satisfy the zero-resource
  teardown audit, and exit 0. The ten-tick `tlwstations01` run moves the player
  from Z 16214.0 to 16080.666667, exactly the train's 800 units/second motion
  over ten 60 Hz ticks.
- Linux could not be rerun: the existing WSL cache references removed
  `/tmp/run3-vcpkg-step7-src2/scripts/buildsystems/vcpkg.cmake`, no
  `$HOME/dev/vcpkg` checkout exists, and Lua 5.4 is consequently unavailable
  during regeneration. This is external local dependency state, not a source
  failure.

The campaign's chapter-only user-save UI/storage and a full start-to-finish
soak remain intentionally assigned to Step 10 by owner direction. No The Long
Way file was modified.

## 2026-09-25 — rotator, FOV-script, and map teardown regressions

Three interactive campaign regressions were repaired in engine code without
modifying The Long Way. The port had treated `<rot rotating="true">` as an
instruction to start immediately and had interpreted `rotspeed` as degrees per
second. Legacy `func_door` instead used `rotating` to select angular motion,
started stopped until `Fire`, applied pitch/yaw/roll as independent local
channels, and multiplied the authored angular speed by five. The map-owned
runtime now preserves those semantics. Rotating `<door>` declarations use the
same legacy rate but remain bounded between the initial orientation and the
authored pitch/yaw/roll target. Their in-progress angles are included in
sequence-state format version 2; version 1 snapshots remain readable.

The `tlwoutro/epictimer2.lua` failure was an API regression: legacy `getFov()`
returned the active Ogre camera FOV, while the compatibility dispatcher
returned Lua `nil`. `getFov`, `setFov`, and `resetFov` now use a typed camera
service; reset restores the configured `--fov` value. The regression test runs
the real `epictimer2.lua` after its expected globals are initialized and does
not require a content edit.

Map replacement left `Run3Step6BMapRoot` registered because `StaticMap::Impl`
owned Ogre objects through raw pointers but its destructor never called the
existing idempotent `unload()`. Destruction now performs that teardown before
the next map constructs its root. An installed Debug D3D11 three-frame
`tlwcao` run loaded the real map and dynamic entities and exited through clean
Ogre shutdown.

Verification: all 13 Step 8C cases pass in Windows MSVC Debug and Release,
including the real attached `tlwcao/fake1` declaration and the real outro Lua
file. All 22 Step 8C/8E cases pass in both configurations. The Debug suite's
114 non-packaging tests passed; its four install/shell/asset smoke cases also
passed when invoked in the required Visual Studio developer environment (118
tests total). No The Long Way file was modified.

## 2026-09-23 — Step 8E authored-presentation closure

`SequenceRuntime` now owns deterministic cutscene tracks and ordered hooks,
computer focus/input/script/display state, HUD/subtitle controls, explicit
player-to-train parenting, effect-controller state, and cancellation on skip,
failure, transition, and unload. `Run3App` consumes camera/freeze state and
performs deferred map replacement only after the fixed update returns. Stable
versioned text snapshots cover persistent sequence/entity/NPC state and reject
map or authored-identity drift. Full semantics and every remaining tag
disposition are documented in
[PRESENTATION_RUNTIME.md](PRESENTATION_RUNTIME.md) and the updated
[ENTITY_COMPATIBILITY.md](ENTITY_COMPATIBILITY.md).

The reported `tlwstations01` blockers are corrected without changing content.
NPC event 26 now sets typed gravity state, and `setCameraParent` binds the
player to the authored train delta after train movement on the same fixed tick.
The content-proven `setNPCManagerStep` command now changes the map-owned NPC
scheduler interval (`0` means every fixed tick) instead of being deferred.
Legacy material definitions are published before dynamic mesh deserialization,
so `air01.mesh` retains its resolved textures and no longer emits missing
`ALLMILmaterial_*` diagnostics. Its old MeshSerializer-v1.40 warning remains
informational.

The Step 8B-8E focused suite passes 38/38 under Windows MSVC Debug and Release;
the full suite passes 116/116 in both configurations. Installed Debug D3D11
two-frame smokes for `tlwstations01` and `tlwstations03` ran startup, train
binding, typed NPC scheduling/event 26, and textured `air01.mesh`, then exited
cleanly. The Ogre RTSS X3205 conversion warnings are non-fatal and are left for
the Step 9 shader pass. Linux could not be rerun: the WSL preset still
references removed `/tmp/run3-vcpkg-step7-src2`, and `$HOME/dev/vcpkg` is
absent. No The Long Way file was modified.

## 2026-09-22 — NPC placement, dynamic materials, display quality, and Doppler

Four interactive-test regressions were corrected after Step 8D. NPC
`physPosit` is once again a scaled local offset of a visual child node rather
than part of the body/world transform, while `physSize` independently scales
the mesh-derived collision box. The authored axis/angle adjustment is also
preserved. This specifically covers the different origin conventions used by
`fac_soldier01.mesh`, `alex_mezhin02.mesh`, `clgrl01.mesh`, and seated meshes.

Doors, trains, buttons, NPCs, and other Sequence-spawned meshes now pass
through the same legacy-material catalogue and RTSS-compatible textured
material generator as static map sections. Generated material names are mapped
back to their legacy source before reuse, avoiding false white/fallback
materials on shared meshes.

Runtime configuration now includes validated vertical FOV (35–120 degrees),
display resolution, independently selected texture and scene quality, and a
model LOD bias. The matching `resources_<texture>_<scene>.cfg` is derived
automatically unless an explicit resource profile is supplied. Audio source
and listener velocities cross a single game-unit-to-metre conversion boundary
before miniaudio Doppler processing; positional attenuation remains in legacy
game units.

MSVC Debug and Release build the shell and focused runtime/audio/NPC targets.
All 29 focused tests pass in each configuration. Installed D3D11 Debug
`tlwcao` smokes at 1280x720, FOV 75, null audio, and high textures/models
exited 0 with both low and high scene quality. They selected
`resources_high_low.cfg` and `resources_high_high.cfg` respectively, loaded
all 19 NPCs and dynamic entities, and shut down cleanly. Interactive
visual/listening judgement remains the author-facing check described in
[RUNNING.md](../RUNNING.md).

## 2026-09-22 — Step 8D NPC slice and Step 8C train/door corrections

Step 8D is a **partial, tested vertical slice**. A new map-owned `NpcSystem`
constructs all 19 `tlwcao` and 28 `tlwhome02` declarations using Step 8B
handles. It implements the content-proven `npc_neutral` and `npc_enemy`
classes as policies over one deterministic state machine, uses the AIR3
`IPhysicsQuery` seam, existing Bullet dynamic scene, Ogre presentation,
miniaudio voice path, and typed ScriptEngine command dispatcher. There is no
`NPCManager` singleton and no Ogre/Bullet type in the public NPC behavior API.
The modular/Lua extension contract and exact numeric event ABI are in
[NPC_RUNTIME.md](NPC_RUNTIME.md).

Implemented integration includes authored transform/scale/yaw, mesh/material,
render distance, movement/stopping, animations, proximity/use/goal/death Lua,
health and headshot classification, neutral/enemy policy, parent/train motion,
teleports, physical-object bone attachments, positional facial-definition
voice audio, and generic Step 6C ragdoll transition/cleanup. Unsupported known
events fail visibly; missing NPC names warn and continue for legacy campaign
compatibility. Friend/aerial policies and `npcgroup` were not invented because
the selected content census does not prove them live.

Nine Step 8D cases pass under MSVC Debug and Release. They cover navigation/blocked paths,
callbacks, fixed-rate replay, animations, parent motion, damage/death/ragdoll,
lifetime/unload, the selected declaration counts, and referenced Lua targets.
The installed D3D11 Debug shell constructs both the 19 `tlwcao` and 28
`tlwhome02` NPC sets, completes startup scripts (including event 17 parent and
event 31 attachment), renders two frames on each map, and exits 0. The Linux
rerun is blocked by external local state:
the prior WSL cache points at removed `/tmp/run3-vcpkg-step7-src2`, and the
documented `$HOME/dev/vcpkg` checkout is absent. As a narrower compiler check,
GCC 13 compiles `NpcSystem.cpp`, `SequenceRuntime.cpp`, and
`DynamicPhysicsScene.cpp` directly with `-Wall -Wextra -Wpedantic`; the final
NPC compile also passes `-Werror`. No content was modified.

Legacy parity is not yet complete: bone look/head motion, animation blending,
facial morphs/subtitles, the detailed NPC sound set, flashlight presentation,
dynamic gravity/floor resolution, blood/gibs, full enemy combat/LOS, and exact
legacy ragdoll bone maps remain explicit gaps. Step 8D therefore does not yet
meet the guide's full 1:1 exit criterion despite passing construction and smoke
coverage.

Two reported Step 8C regressions were corrected at the same boundary. A
non-`inf` train now stops at its terminal key point instead of modulo-wrapping
to the first point; a one-key train also stops. An `inf=true` train retains
legacy wrap behavior. Translating doors use the legacy `TIME_SHIFT / 0.2`
rate, i.e. five times the authored speed per second, instead of treating the
authored value as direct units/second. The regression fixture checks a door's
first two fixed ticks and proves a train remains at its destination after
additional ticks.

## 2026-09-22 — Step 8C and station-map crash

Historical status at 2026-09-22: Step 8C was a **partial, tested vertical
slice**. `SequenceRuntime` is map
owned, constructed from Step 8B definitions/registry, and updated from the
fixed gameplay tick. Typed game-service commands connect authored Lua to live
doors, trains, timers, triggers, visibility, teleport, script chaining, and
the existing physics and audio layers. The stable tick/order queue supports
delayed events, one-shot and repeated edges, startup/onexit Lua, contextual
errors, cancellation on unload, and in-memory snapshot/restore. NPC commands
are now consumed by Step 8D; cinematic, computer, and presentation-only
commands log explicit deferrals and do **not** yet execute campaign gameplay. Missing named objects requested
by compatibility Lua now produce a console/log warning and leave the script
running; malformed calls and real script failures still propagate. Absent
optional authored lights follow the legacy `hasLight` check.

### Missing-object compatibility correction

The author reported two further shutdowns: `tlwcao` requested absent door
`right3`, and `tlwstations01` requested train `mspz1`. The latter is not
absent: the selected Sequence also declares a **dark zone** named `mspz1`
before the train. Lookup now selects the first record of the requested type,
so `startTrain("mspz1")` reaches the train. Visibility commands also target
its rendered train rather than the dark zone, and visibility no longer disables
train simulation. Truly absent door, train, timer, trigger, event, entity, and
named-ambient targets warn and skip only that
command. The same rule applies to missing door targets in queued authored
events. Empty target names, invalid numeric arguments, missing script files,
and Lua execution errors remain errors. A fixture runs a command following an
absent door to prove script continuation; attached-content tests execute the
reported `close_turnik.lua` and `air.lua` files and verify the duplicate-name
train selection. The focused 10/10 Step 8C cases pass on MSVC Debug and Linux
Ninja Debug with this correction. The full Windows Debug CTest suite passes
93/93 inside the VS developer environment, and the updated installed D3D11
shell renders two `tlwstations01` frames and exits 0. No game content was
modified.

The fixture tests cover trigger light restoration, buttons and use rays,
translating doors, rotators/pendulums, train key points and Bullet ground-probe
parenting, ladder/pickup classification, dark-zone state, audio hooks, cleanup
during queued work, script failure, saved timers/actions, and identical state
after 180 fixed ticks at 30/60/144 rendering FPS. Full-content construction
tests include `tlwcao`, `tlwhome02`, `tlwstations01`, and `tlwstations03`.
Campaign-wide interaction and a serialized disk save remain unverified; see
[SEQUENCE_RUNTIME.md](SEQUENCE_RUNTIME.md) and the dated overlay in
[ENTITY_COMPATIBILITY.md](ENTITY_COMPATIBILITY.md).

The reported station crash was a D3D11 first-frame access violation when an
old textured mesh had no UV vertex semantic, **not** the X3205
`evaluateLight` shader warning. StaticMap checks required mesh semantics and
indices and selects a safe compatible material for missing UVs/normals. No
content was modified. MSVC Debug D3D11 two-frame runs of `tlwstations01`,
`tlwstations03`, and `tlwhome02` exited 0 with clean shutdown. A first
`tlwcao` run uncovered an unintended initial `lOnClosed` callback: the authored
script referenced an entity in another map. The runtime now only runs the
completion callback after a real door transition, covered by a fixture test.
The corrected installed Debug build also rendered two `tlwcao` D3D11 frames
and exited 0, preserving its player position. That run logged 21 buttons
without a matching already-presented scene object (they use authored Sequence
transforms and meshes), missing authored-light presentations, and deferred
UI/presentation callbacks (NPC callbacks are now handled by Step 8D). These are visible compatibility gaps, not a
claim of fully playable map interactions.

The Windows install/smoke CTest tail passes inside the VS developer
environment; running `cmake --install` outside it copies the shell but fails
at `file Could not find objdump` during dependency packaging.

Verification: the current `windows-msvc-x64-debug` full CTest suite passes
93/93 in the VS developer environment, including install and audio/player
tests. The current `linux-ninja-debug` shell/Step 8C build passes 10/10
focused cases. Before this missing-object correction, both Windows and Linux
Release presets built the shell and passed 10/10 Step 8C cases; Release was
not rerun after this correction. D3D11 visual smokes were performed on Windows
Debug only; a Linux graphical session and long campaign replay were not run.

## 2026-09-18 — Static-map root regression repair

The Step 8B runtime adapter was corrected after an interactive regression test:

- Cause: renderables below authored inactive `<nodev>` branches were admitted
  into the runtime scene. A root-level `<phys>` was consequently attached to
  `Run3Step6BMapRoot`; dynamic-body synchronization then moved and rotated the
  complete rendered world while its static collision was left behind.
- Fix: active map renderables are selected by one tested adapter that requires
  an exact active `<node>` and rejects complete `<nodev>` subtrees. StaticMap
  also refuses to construct or synchronize a dynamic binding on its map root
  as a local safety invariant.
- Verification: all 17 Step 6B player and Step 8B entity tests pass. A 300-step
  D3D11 `tlwcao` run restored the known-good `368` visual / `166` collision
  inventory, retained the player at map elevation, and produced live weapon
  ray hits. `tlwhome02` also loaded and advanced through Bullet without a
  root-level binding. No game content was modified.

## 2026-09-18 — Step 8B

Completed:

- Replaced live `StaticMap` regex scanning with the TinyXML2-backed
  `MapDefinition` tree. Parsing preserves configuration entries, external and
  integrated sequence ordering, every XML element/attribute, mixed transform
  representations, source order, and file/line locations without constructing
  runtime objects or changing content.
- Added exact-case, root-confined resolution through `AppPaths`. Missing paths,
  Linux-visible case mismatches, malformed XML, duplicate config keys, and
  preserved unknown tags have contextual diagnostics.
- Added generation-checked `EntityId`/`EntityHandle` values and a map-scoped
  `EntityRegistry`. It preserves duplicate declarations and first-authored
  lookup with diagnostics, resolves required event targets after construction,
  exposes presentation/physics bindings, destroys in reverse order, and
  invalidates stale handles on unload/reload.
- Adapted the existing Ogre/static-map and Step 6C body construction to the
  parsed definition tree. No entity state machine or gameplay behavior was
  added; that remains Steps 8C-8E.
- Added the read-only `run3_entity_inventory` tool and a reviewed compatibility
  matrix for every observed DotScene/Sequence tag and attribute, all 18
  `low/*/scene.cfg` Sequence references, per-map declaration/event counts,
  implementing owner, status, evidence, disabled tag variants, and legacy
  defaults. The author content stayed byte-identical.
- Added seven fixture/content tests covering external plus integrated ordering,
  source locations, expected `tlwcao`/`tlwhome02` counts, all low-variant maps,
  malformed XML, exact case, unknown required tags, duplicates, missing
  references, inactive-node runtime filtering, bindings, deterministic reverse
  cleanup, and handle reuse.

Verification:

| Configuration | Scope | Result |
|---|---|---|
| `windows-msvc-x64-debug` | Full build and CTest suite | Build passed; 81/81 tests passed (the install/smoke tail was rerun inside the required VS developer environment) |
| `windows-msvc-x64-release` | Full build; Step 8B tests | Build passed; 6/6 Step 8B tests passed |
| `linux-ninja-debug` | Step 8B, shell, and inventory targets | Build passed; 6/6 Step 8B tests passed under Ubuntu/WSL GCC |
| `linux-ninja-release` | Step 8B, shell, and inventory targets | Build passed; 6/6 Step 8B tests passed under Ubuntu/WSL GCC |
| Windows D3D11 Debug | Real-map bounded smoke | `tlwcao` and `tlwhome02` loaded from the definition adapter, rendered two frames, and shut down cleanly with null audio |

Detailed schema, counts, ownership, and deferred/unused/retired decisions are
in [ENTITY_COMPATIBILITY.md](ENTITY_COMPATIBILITY.md).

## 2026-09-16 — Step 8

Completed:

- Captured deterministic semantic golden outputs for scene, sequence, save,
  facial-animation, config-adjacent, and malformed XML. The attached real
  `tlwcao`, save, facial, and config representatives parse through the same
  schema adapter.
- Added `run3::xml`, with TinyXML2 private to one implementation. The adapter
  preserves ordered schema data and performs the minimal in-memory unquoted
  attribute normalization required by legacy sequences. Contextual parse
  errors include path, schema, line, column, and cause.
- Retired bundled TinyXML 1 from `run3_legacy`; its reviewed subset is now
  23/119 vcproj translation units. Asset and Ogre fixture parsing also use the
  Run3 adapter. A repository test rejects parser headers outside that boundary.
- Inventoried 1,016 sites using 23 distinct Lua 5.0 C API operations and all
  202 unique exported globals across 296 tracked Run3 legacy source/header
  files. No tracked luabind registration exists. The machine-readable
  inventory covers all 955 attached scripts and
  generates the binding catalog plus golden names/signatures snapshot.
- Added `run3::scripting` on exact Lua 5.4.8 and sol2 3.5.0#1. Script files are
  confined to approved content/user roots; file, OS, package, and debug globals
  are unavailable; runtime calls have traceback and instruction-budget
  protection; errors are propagated as contextual `ScriptError` values.
- The only content-driven compatibility shim is Lua 5.0's tolerated `\s`
  string escape in `lua/computers/demo_comp5.lua`. Three unreferenced timer
  scripts with missing parentheses remain explicitly documented as broken.
  The other 952 scripts compile, and representative chapter, NPC, trigger,
  music, subtitle, and map-change scripts execute through compatibility
  bindings.

Detailed inventory, policies, and known exceptions are in
[XML_LUA.md](XML_LUA.md).

Verification:

| Configuration | Toolchain | Result |
|---|---|---|
| `windows-msvc-x64-debug` | MSVC 19.51 x64 | Build passed; 75/75 CTests passed; 11/11 Step 8 tests passed |
| `windows-msvc-x64-release` | MSVC 19.51 x64 | Build passed; 75/75 CTests passed; 11/11 Step 8 tests passed |
| `linux-ninja-debug` | GCC 13.3 x64 under WSL | Build passed; 75/75 CTests passed; 11/11 Step 8 tests passed |
| `linux-ninja-release` | GCC 13.3 x64 under WSL | Build passed; 75/75 CTests passed; 11/11 Step 8 tests passed |

The full-content cases used the attached author-provided tree. No game asset was
modified, copied, or generated by Step 8.

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
