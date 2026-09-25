# Step 8C Sequence runtime: behavior and limits

As of 2026-09-25, `Run3App` creates one `SequenceRuntime` per loaded map from
the Step 8B `MapDefinition`/`EntityRegistry`, updates it on each 60 Hz gameplay
tick, then releases it before Ogre, the physics world, and audio. No legacy
`Sequence` frame listener or singleton is used. The implementation does not
modify The Long Way assets.

## Runtime contract

- Declaration order, first-declaration name resolution, and authored source
  file/line context come from Step 8B. Startup scripts run after scene entities
  are constructed; `onexit` runs before entity teardown. Missing named targets
  requested by compatibility Lua warn and skip that command, matching legacy
  campaign behavior; malformed calls and genuine script failures retain
  contextual exceptions, even when cleanup is needed. Lookup is type-aware:
  the authored `mspz1` dark zone does not shadow the same-named train.
  Visibility is stored separately from simulation enabled state, so hiding a
  train does not stop its authored movement.
- Events enter one stable queue keyed by `(dueTick, insertionOrder)`; canceling
  a map discards pending actions. Time in seconds rounds up to the next fixed
  1/60-second tick. Timer phase, pending actions, entity state, and train
  key-point index participate in a versioned, map-checked stable-state
  serialization. Format 4 also records train speed/acceleration and explicit
  player-parent mode/local offset; formats 1–3 remain readable. Connecting
  that state to the campaign's chapter-only user-save UI/storage is deferred
  to Step 10 by owner direction.
- First-person `E` raycasts through the shared Bullet world to typed button,
  door, and pickup handles; trigger volumes do not block these queries. A
  button reuses its named DotScene presentation where available and gets a
  matching world-space interaction box instead of a second rendered object.
  Static map collision remains owned by `StaticMap`.
- Triggers intersect the player capsule's box extents, with authored trigger
  `<scale>` interpreted as a full size. Single-use events fire on entry;
  multiple triggers run `luaOnEnter`/`luaOnLeave` on edges and restore the
  previous visibility of any existing `lighton`/`lightoff` lights on exit.
  The optional-light behavior matches the legacy check for `hasLight()`.
- Doors approach their authored directional endpoint at fixed-tick speed;
  authored translating-door speed retains the legacy `/ 0.2` factor (five
  times the numeric value per second with default time shift);
  completion scripts run only after a real transition, never on the initial
  closed tick (the old eager callback broke `tlwcao` startup by naming an
  entity from `tlwhome02`). Rotators preserve the legacy stopped-by-default
  activation and five-times authored angular rate; pendulums oscillate.
  Non-infinite trains stop at their terminal authored key point; only
  `inf=true` trains wrap to the first point. Trains preserve the legacy
  acceleration update order. They carry a player either through the Bullet
  ground probe or an explicit `setCameraParent` seat relation; the latter pins
  the capsule against gravity/drift and full parenting rotates the saved local
  offset. `resetCameraParent`, transition, and unload release it. Ladders reuse
  player ladder motion; pickups are implemented
  in a fixture but have no declarations in the selected campaign. Dark zones
  modulate ambient light; state is map-owned and resets at unload.
- Nested train `<object>` parts get authored collision, `<nocollide>` parts
  remain visual-only, and all parts follow their parent transform. Selected
  texture-quality particle scripts load after material aliases; authored map
  particles, nested train `<psys>` effects, dynamic create/delete/toggle,
  material changes, and animation dispatch use typed map-owned services.
- Commands for current gameplay objects, teleport, existing map audio, ambient
  sound, music, effects, and existing sequence animations use typed services.
  NPC commands are routed to the map-owned Step 8D system. Commands owned by
  later cinematic/computer/presentation steps emit an explicit deferred
  diagnostic, not a silent compatibility no-op. `changelevel` safely queues a
  map replacement after the active fixed update returns; old-map callbacks,
  presentation, physics, audio, and handles are released before new ownership
  is constructed.

## Intentional differences and later-step work

Legacy `Trigger` used frame time and a hysteresis multiplier for multiple
trigger exits. Here trigger edges use one deterministic capsule overlap test.
Train child collision is built from the content-proven authored parts rather
than reproducing every unused experimental Newton shape. The dark-zone factor
is a bounded ambient-light control; Step 9B owns its final screen/lighting
presentation. Buttons without a matching scene object use their Sequence mesh
fallback and log the condition. Step 8E owns cinematic/computer behavior and
Step 8D owns NPC behavior; their remaining presentation/parity limits are
documented separately. Step 9 owns visual polish, including modern shader,
compositor, HUD, and particle-material quality beyond the functional effects
path present here.

Stable-state tests cover entity state, delayed actions, timer phase, train
internals, and player parenting. Fixture replay compares 30, 60, and 144
rendering frames per second across the same 180 fixed gameplay ticks. The
chapter-save integration and end-to-end campaign soak are Step 10 work rather
than Step 8C exit criteria, following owner direction.

## Lifetime contract

Every unload runs an idempotent runtime destroy, including partial-load and
script-failure paths. Before releasing the map, `Run3App` checks that runtime
presentations, child parts, particles, physics bindings, audio handles,
attachments, ragdolls, and the Ogre runtime root are all zero. A fixture
repeats construction and teardown 100 times. `StaticMap` is then explicitly
unloaded and independently checked for retained entities, map particles,
physics bodies, or its Ogre root. Onexit failure is retained and reported
after cleanup rather than bypassing cleanup.

## Stations D3D11 crash diagnosis

In both `tlwstations03` and `tlwstations01`, some classic `.mesh` submeshes
advertise textured materials but have no texture-coordinate vertex element.
The prior RTShaderSystem/D3D11 shader path used that absent vertex semantic,
raising an access violation in the first `renderOneFrame()` and then printing
only normal-looking Ogre shutdown messages. X3205 `evaluateLight` precision
warnings are unrelated. The static-map adapter now checks position/index
validity and selects a solid/unlit compatible material for submeshes that lack
UVs/normals; originals remain untouched. The observed D3D11 Debug runs on both
maps exit with code 0. `tlwstations02` also exposed duplicate-position AIR3
nodes: a zero-length ray reached Bullet's Debug assertion and looked like a
normal shutdown. AIR3 now treats such graph edges as trivially clear and has a
query-seam regression test proving no zero-length ray reaches physics. Ogre's
X3205 shader precision warnings remain presentation work for Step 9B.

## Verification

Windows MSVC Debug and Release pass all 48 focused physics/player/Step 8C/8E
tests and all 124 repository CTest cases. Installed Debug D3D11 runs of
`tlwstations01`, `tlwstations02`, and `tlwstations03` start and unload cleanly.
The station01 ten-tick run moves the attached player by exactly the train's
fixed-tick delta. The station effects load their selected-quality template and
resolved smoke texture. Linux regeneration is currently blocked by the local
WSL cache's removed vcpkg toolchain and missing Lua 5.4 dependency; see
[STATUS.md](STATUS.md) for the exact local-environment failure.
