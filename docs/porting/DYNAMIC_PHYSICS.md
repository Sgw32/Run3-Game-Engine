# Step 6C dynamic physics migration

This is the behavior contract for the last Newton-to-Bullet migration slice.
All production physics enters through `run3::physics`; gameplay and AIR3 public
headers contain no Bullet, Newton, OgreNewt, or Ogre types.

## Vertical slices

| Legacy behavior | Run3/Bullet implementation | Evidence |
|---|---|---|
| `PhysObject` | Dynamic box body, default 10 kg, positive source `mass` honored; Ogre pulls its interpolated transform | real-map inventory and bounded `tlwcao` render smoke |
| `Breakable` | Dynamic box body, default 40 kg; typed health/damage stays outside Bullet | projectile/damage/broken fixture and eight `tlwhome02` declarations |
| Pickup | No-response `Pickup` group; player begin-contact queues `PickupCollected` and disables the body | contact fixture |
| Button | No-response `Button` group; player begin-contact queues `ButtonPressed` | contact fixture and representative sequences |
| Trigger/ladder | No-response `Trigger` group queues enter/exit; Step 6B ladder volumes stay backend-neutral | trigger and ladder fixtures |
| Door | Kinematic `Door` group moves toward explicit closed/open transforms at bounded speed | deterministic door fixture |
| Train/parent | Kinematic `Train` group accepts an authoritative transform; player parent displacement is explicit | train and parent-motion fixtures |
| Projectile/damage | `Projectile` begin-contact queues impact and typed damage; destroyed breakables are disabled after stepping | projectile fixture |
| NPC | `Npc` group and `BodyType::Npc`; contact uses stable numeric entity IDs | NPC fixture and representative sequences |
| Ragdoll | RAII torso/head/leg fallback, point and limited hinge joints, timed expiry | lifetime and unload/reload fixture |
| AIR3 | `IPhysicsQuery` injection and breadth-first visible-node search in static `run3_air3` | fake-query obstruction fixture |

The sequence XML presentation/script adapters are not duplicated inside
physics. Step 8 will bind the typed factories and events to the modern XML/Lua
layer. Step 6C owns their collision behavior, filters, lifetimes, and fixtures.

## Typed identity and contacts

`BodyMetadata` is `{entityId, BodyType, partId}`. `BodyType` distinguishes
world, player, physical object, breakable, pickup, button, trigger, door,
train, projectile, NPC, and ragdoll bone. Bullet's user pointer always points
to a backend-owned record, never a gameplay object, string, or content buffer.

Bullet manifolds are copied into `ContactEvent` values after each fixed step.
`DynamicPhysicsScene::processContactEvents` drains them after stepping and maps
group/type pairs to gameplay events. Begin/persist/end are preserved; one-shot
interactions consume only begin events.

## Constraints and tuning

Only constraints evidenced by The Long Way were implemented:

- Newton `BallAndSocket` maps to a Bullet point-to-point joint for the current
  neck/torso fallback. Newton cone/twist limits are not silently approximated.
- Newton `Hinge` plus stop-acceleration maps to Bullet's native lower/upper
  radian limits. The fallback hip uses plus/minus 0.785398 radians (45 degrees).
- Newton `UpVector` is not retained as a general joint. The player uses
  `angularFactor={0,0,0}`; trains and doors are authoritative kinematic bodies.

Bullet normalizes hinge axes. Pivots remain game-unit lengths and cross the
central 0.01 metre/game-unit boundary. Linked collision is disabled by default.
Destroying either body invalidates and removes its constraint.

The ragdoll is deliberately a portable collision fallback: three capsule
parts split total mass 55/10/35 percent and default to a 10 second lifetime.
Exact skeleton shapes/blending belong with later NPC visual work. Unused
cone/cylinder/convex-hull and fuzzy-test joint experiments were not ported.

## Campaign evidence and live boundary

The attached high-quality representatives contain:

| Map | Physical | Breakable | Door | Button | Trigger | Train | NPC |
|---|---:|---:|---:|---:|---:|---:|---:|
| `tlwcao` | 11 | 0 | 29 | 21 | 30 | 2 | 19 |
| `tlwhome02` | 13 | 8 | 50 | 11 | 42 | 14 | 28 |

CTest checks conservative minimums. If local content is absent, only that
integration case skips; content is never copied or modified.

`CheckStep6CBoundary.cmake` scans all live modern/compatibility code and the
root build for Newton/OgreNewt headers, link names, and the retired feature
switch. Old root files and the AIR3 submodule remain only as historical
behavior evidence and are excluded from every CMake target. The legacy source
manifest explicitly classifies Newton-coupled translation units as retired.
There is no Newton link flag, DLL staging, header include, or enable option in
the live graph.
