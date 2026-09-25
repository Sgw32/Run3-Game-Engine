# Step 8E authored presentation runtime

Step 8E keeps gameplay state in the map-owned `SequenceRuntime`. Ogre is only
an adapter for camera and entity presentation; input uses Run3 `InputEvent`
and `Key` values, scripts use `ScriptEngine`, and audio/physics continue to use
the Step 7 and Step 6 services. No OIS, CEGUI, frame listener, or singleton was
added.

## Cutscenes

`cutscene` declarations preserve length, fixed-tick keyframe time, position,
quaternion, optional look target, wait, infinite playback, skip multiplier,
freeze, HUD, music, and ordered `run` hooks. Positions and quaternions are
interpolated from the same 60 Hz gameplay clock at every rendering rate.
Space or Escape accelerates a running cutscene; normal completion, skip,
replacement, script failure, map transition, and unload all release its camera
override and restore player/HUD state. The Step 9 HUD adapter still owns the
visual subtitle and HUD widgets, but their typed visibility/text commands are
live now.

## Computers

Every `computer` is a typed, map-owned interactive entity with a normal Run3
handle and raycast collider. Using it captures gameplay input, freezes player
movement, hides the HUD, runs its init script, forwards backend-neutral key and
text events, and exposes `dMaterialSet` through `IComputerPresentation`.
Escape, `exitAllComputers`, interruption, transition, and unload release focus,
run the shutdown script, and restore control. The proximity script runs once
within the legacy 200-unit radius. Step 9A owns the final virtual-screen and
buttonGUI drawing; Step 8E supplies its functional focus, input, script, and
display-material state without reviving CEGUI.

## Reviewed disposition of remaining authored tags

| Authored feature | Selected low-variant evidence | Step 8E disposition |
|---|---:|---|
| `computer` | 31 campaign declarations; 4 `tlwcao`, 5 `tlwhome02` | Implemented and tested; final virtual-screen/button drawing delegated to the named Step 9A computer presentation adapter. |
| `cutscene` | 23 declarations, 20 bindings, 112 ordered `run` hooks | Implemented and tested; final subtitle/HUD visuals delegated to Step 9A through live typed commands. |
| `darkzone` | 10 | Implemented by `SequenceRuntime`; typed ambient-darkness control remains the input to Step 9B lighting. |
| startup `lua` / `onexit` | 18 / 5 | Implemented; errors retain source context, onexit runs during transition/unload, and no callback survives its map. |
| trigger/event relay | 65 trigger bindings; three live `changelevel` actions | Implemented typed delayed actions and deferred-safe transitions. The real `tlwstations01` `cng -> tlwstations02` target is an automated test. |
| pickup | 0 | Explicitly unused in the selected campaign; fixture behavior remains implemented by Step 8C. |
| Sequence `flare` / `fire` | 0 / 0 | Explicitly unused as Sequence declarations. |
| DotScene `fire` | 4, all in `tlwdolg`; `fireToggle("fire1")` is script-live | Gameplay controller is typed and functional; it toggles the named Ogre ParticleFX presentation when loaded. Step 9A/9B own final effect/material polish. |
| DotScene `particleSystem` | 97 | Selected-quality templates, authored instances, typed toggling, map ownership, and teardown are implemented. Step 9A/9B own final effect/material polish. |
| nested train `entity` / `nocollide` / `object` / `phys` / `psys` parts | content-proven in station trains | Multipart visuals, authored collision/noncollision, particles, transform following, parenting, and teardown are functional. |
| `npcgroup` | 0 | Explicitly unused; no legacy group singleton was revived. |
| `seqscript` | 0 declarations / 0 bindings | Explicitly unused; ordinary startup, delayed-event, and cutscene scripts cover all selected content use. |
| `fuzzy` | 0 | Explicitly retired for this campaign variant; legacy experimental classes are not built. |
| disabled `*d` / `*v`, `npcold`, `rodt`, `blockboxes` | 38 declarations plus documented scene variants | Explicitly retired because the legacy loader used exact tag names and did not dispatch them. They remain byte-for-byte parsed and inventoried. |
| `fade`, `hud`, `portal`, fog/sky/water/light/material presentation tags | counts in `ENTITY_COMPATIBILITY.md` | Controlling state stays typed; visible implementation is delegated to the named Step 9A UI/environment adapter or Step 9B lighting/material adapter. |
| custom marker tags (`ohrana`, `rocketworker(s)`, `suicide`, `taxist`) | one each where listed | Explicitly unused metadata: legacy gameplay did not dispatch them and no selected script targets them as an entity type. |

## Persistence and transitions

`RUN3_SEQUENCE_STATE 4` is a deterministic, locale-independent text format
covering map identity, tick/order, entity transforms and flags, timer/train
internals (including speed and acceleration), queued authored actions,
explicit train parent/mode/local offset, active cutscene time, and computer
focus. Readers retain formats 1–3 for compatibility. Runtime handles are
deliberately rebuilt from authored identity rather than serialized.
`RUN3_NPC_STATE 1` covers NPC identity,
state, transform, goal, health, animation, parent name, and gravity state.
Malformed versions, count drift, and identity drift are rejected.

Live `ChangeRuntimeMap` requests are deferred until the fixed update returns.
`Run3App` then runs old-map exit handling, destroys NPC/sequence/audio/physics
ownership, loads the new map with fresh handles, and resumes the loop. This
prevents destruction while a script or queued action is on the call stack.

## Station regressions

NPC event 26 is the live `SetGravity` ABI, not an unknown experimental event.
It now updates typed NPC gravity state, so the three inspector calls in
`tlwstations01/startup.lua` do not abort map startup. `setCameraParent` and
`resetCameraParent` now bind/release the player to a named train transform.
The delta is applied after train motion on the same fixed tick and is not added
twice when the Bullet ground probe also sees that train.

`setNPCManagerStep` is also a typed command now. A positive value accumulates
the fixed 60 Hz ticks and updates the NPC state machines at that deterministic
interval; `0` restores one NPC update per fixed tick. This preserves the live
`tlwstations03` startup/train-stop control without restoring `NPCManager`.

The `ALLMILmaterial_*` definitions referenced by `air01.mesh` exist in
`run3/models/air01_r3dds.material`. The compatibility catalogue now publishes
resolvable aliases before dynamic Sequence meshes are deserialized, eliminating
the false missing-material messages while preserving textured RTSS fallbacks.
The old MeshSerializer-v1.40 warning remains informational; source assets are
not rewritten.

## Verification

`run3_step8e_tests` provides a skippable cutscene, interactive computer,
focus/input/exit handling, transition cancellation, deterministic stable-state
round trip, explicit 30/60/144 camera equivalence, all nine selected-map
computer declarations, all four selected-map cutscenes and their script paths,
typed NPC scheduling, and the real stations transition target. The adjacent
NPC tests cover event 26, scheduler cadence, and NPC-state serialization.
Windows MSVC Debug and Release build the shell; the current 48/48 focused
physics/player/Step 8C/8E tests and 124/124 complete tests pass in each
configuration. Installed D3D11 `tlwstations01`, `tlwstations02`, and
`tlwstations03` runs resolve dynamic materials, load selected-quality particle
effects, run station scripts and train binding, satisfy teardown audits, and
exit cleanly. Linux verification is currently blocked
by the local WSL preset pointing at removed
`/tmp/run3-vcpkg-step7-src2`; `$HOME/dev/vcpkg` is absent.
