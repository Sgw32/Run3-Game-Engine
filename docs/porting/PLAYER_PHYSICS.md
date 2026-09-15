# Step 6B player and static-map physics

## Delivered boundary

`run3::gameplay::PlayerController` replaces the live first-person portion of
the old `Player` for the ported shell. It depends only on the Run3 physics API.
`run3::gameplay::StaticMap` is the corresponding render/static-collision path.
The historical `Player.cpp` and `DotSceneLoader.cpp` remain as reference for
unmigrated campaign systems; they are not linked into `run3_shell`.

No migrated public header includes or names OgreNewt, Newton, or Bullet. A
CTest boundary check enforces that rule.

## Behavior mapping

| Legacy behavior | Step 6B implementation | Validation |
|---|---|---|
| Newton ellipsoid plus UpVector joints | Dynamic Y-axis capsule, zero angular factor, 40 kg, fixed 60 Hz; game default 180 cm | Upright/fall/rest fixture |
| Walk and sprint | Camera-yaw-relative velocity, normalized diagonal input, 300/520 game units/s | Walk/run and replay tests |
| Gravity and floor test | Bullet gravity at -981 game units/s² and an own-body-excluding downward ray | Box and indexed-mesh floors |
| Jump | Grounded, rising-edge jump at 360 game units/s | Jump fixture |
| Duck/unduck | Rebuild between 200- and 110-unit capsules while preserving the foot; upward clearance ray rejects blocked unduck | Duck/ceiling fixture |
| Stair effect | Paired low/high forward rays and a bounded 35-unit step | Stair fixture |
| Noclip | Disable collision/simulation and integrate camera-relative XYZ motion; re-enable at the retained position | Noclip fixture and `N`/`--noclip` |
| Teleport | Atomic transform plus velocity reset | Teleport fixture |
| Parent/train carry | Explicit per-fixed-step parent displacement | Parent-motion fixture |
| Ladder | Ladder AABBs are derived from map nodes; inside one, forward/up input sets vertical ladder speed | Ladder fixture and map inventory |
| Use/weapon | Closest, filtered Run3 physics rays at 250/100000 game units, excluding the player | Raycast fixture |

The capsule is intentional. It has stable ground contact and slides around
corners more predictably than attempting to reproduce Newton's highly
anisotropic 20x100x20 ellipsoid. The game default is 180 cm with an
approximately 165 cm eye height. `player-height-cm` in the user configuration
or `--player-height-cm` on the command line accepts 120–240 cm and derives the
crouch/eye dimensions proportionally. Speeds and the 35-unit step remain
initial compatibility values to tune against a recorded legacy playthrough.

## Static geometry rules

- `scene.cfg` chooses the primary XML file. The loader recognizes
  `tlwhome02` (and the user-facing alias `tlwhome2`) plus `tlwcao` at low,
  medium, or high quality.
- The structural parser ignores malformed sound/script attributes that are not
  needed by Step 6B and never repairs or rewrites the source XML.
- `<entity>` and legacy static `<phys>` form collision; `<nocollide>` is
  render-only. `pblock`/`blockbox` form invisible collision. Breakables,
  ragdolls, doors, trains, NPCs, triggers, and scripted contacts are Step 6C.
- Each colliding section owns one `btBvhTriangleMeshShape` indirectly through
  a move-only `BodyHandle`. Vertex/index buffers are read once. Node and scene
  scale is baked into vertices; derived position/orientation remains the body
  transform. Mirrored scale reverses triangle winding. Static-world masks do
  not collide with other static-world bodies.
- Unload destroys the player, static body handles, then Ogre entities/nodes.
  Originals remain read-only.

## Visual compatibility boundary

The original material scripts contain duplicate GPU program names and many
D3D9-only programs. Loading them directly under D3D11 failed at render time.
The compatibility loader now inventories material inheritance and aliases,
then builds simple Ogre 14 RTSS materials with each legacy diffuse texture.
XML subentity overrides are honored. Alpha blending and double-sided flags are
carried across where declared or inherited. Advanced shader behavior remains a
Step 9 task, and a genuinely missing texture falls back per material rather
than making the whole map white.

## Recorded map runs

| Map | Visual sections | Collision sections | Triangles | Skipped | 30/60/144 result |
|---|---:|---:|---:|---:|---|
| `tlwcao` | 368 | 166 | 199,959 | 0 | 60 ticks; identical `(-4638.281250, -9262.699890, 1600.035667)` |
| `tlwhome02` | 990 | 407 | 225,359 | 0 | 60 ticks; identical `(1257.599735, -296.402812, -2387.063599)` |

These are one-second D3D11 Debug runs with `--frames N --render-hz N` for
N=30, 60, and 144. The tracked fixture and all four preset test suites provide
the compiler-independent regression. The large map is intentionally not
streamed yet; use `tlwcao` for quick manual checks.
