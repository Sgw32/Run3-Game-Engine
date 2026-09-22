# Step 8C Sequence runtime: behavior and limits

As of 2026-09-22, `Run3App` creates one `SequenceRuntime` per loaded map from
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
  key-point index participate in an in-memory map-checked snapshot/restore.
  There is not yet a disk save-file serialization format.
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
  entity from `tlwhome02`). Rotators update orientation and pendulums
  oscillate. Non-infinite trains stop at their terminal authored key point;
  only `inf=true` trains wrap to the first point. Trains carry the player only if a
  Bullet ground probe hits that
  exact train body. Ladders reuse player ladder motion; pickups are implemented
  in a fixture but have no declarations in the selected campaign. Dark zones
  modulate ambient light; state is map-owned and resets at unload.
- Commands for current gameplay objects, teleport, existing map audio, ambient
  sound, music, effects, and existing sequence animations use typed services.
  NPC commands are routed to the map-owned Step 8D system. Commands owned by
  later cinematic/computer/presentation steps emit an explicit deferred
  diagnostic, not a silent compatibility no-op. `changelevel`
  safely requests shutdown and cancels queued work; actual map transition is
  not implemented here.

## Known differences and incomplete campaign behavior

Legacy `Trigger` used frame time and a hysteresis multiplier for multiple
trigger exits. Here trigger edges use one deterministic capsule overlap test.
Legacy door/train acceleration, rotating doors, authored train subparts and
effects, movable non-static button variants, and complete material/animation
hooks still need separate map-specific fixtures. Train interaction bodies are
currently boxes from the primary mesh, not a reconstruction of every authored
`physPosit`/`physSize` child. The dark-zone factor is a bounded ambient-light
approximation rather than a full legacy screen-space effect. Buttons without a
matching scene object log a warning and use a sequence mesh fallback; these
must be checked on the specific campaign map. Cinematic/computer commands do
not execute gameplay and should not be mistaken for completed campaign
coverage. NPC behavior is now owned by Step 8D and its limits are documented
in [NPC_RUNTIME.md](NPC_RUNTIME.md); cinematic playback and computers remain
unported.

The sequence snapshot test covers in-memory state and delayed events. A
persistent user save file and end-to-end campaign replay remain unverified.
Fixture replay compares 30, 60, and 144 rendering frames per second across
the same 180 fixed gameplay ticks. This is not yet an interactive long-duration
campaign test.

## Stations D3D11 crash diagnosis

In both `tlwstations03` and `tlwstations01`, some classic `.mesh` submeshes
advertise textured materials but have no texture-coordinate vertex element.
The prior RTShaderSystem/D3D11 shader path used that absent vertex semantic,
raising an access violation in the first `renderOneFrame()` and then printing
only normal-looking Ogre shutdown messages. X3205 `evaluateLight` precision
warnings are unrelated. The static-map adapter now checks position/index
validity and selects a solid/unlit compatible material for submeshes that lack
UVs/normals; originals remain untouched. The observed D3D11 Debug two-frame
runs on both maps exit with code 0. This does not mean every mesh has correct
textures or that shader warnings have been resolved.
