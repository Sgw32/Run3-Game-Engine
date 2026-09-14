# Step 6A physics behavior and OgreNewt migration map

This document is the contract for the Run3-owned physics boundary. Step 6A
implements and tests the boundary; it does **not** migrate `Player`, map
loading, entities, AIR3, or ragdolls. Those call sites remain behind the
legacy Newton feature gate until Steps 6B and 6C.

## Inventory boundary

`Run3.vcproj` remains the reviewed runtime source of truth. A case-insensitive
scan for `OgreNewt`/`Newton` finds 40 project-listed translation units; two are
the already excluded `FuzzyTest.cpp`/`FuzzyTest2.cpp` demos. The 38 live
project-listed translation units with direct references are:

```text
BlastWave.cpp                  Button.cpp
ButtonContactMatCallback.cpp  Bullet.cpp
Computer.cpp                   CWeapon.cpp
DotSceneLoader.cpp             Energy.cpp
Generator.cpp                  LaserMinigun.cpp
LoadMap.cpp                    MirrorManager.cpp
NPCManager.cpp                 Pendulum.cpp
PhysObject.cpp                 PhysObjectMatCallback.cpp
Pickup.cpp                     PickupMatCallback.cpp
Player.cpp                     PlayerContactCallback.cpp
Punch.cpp                      Ragdoll.cpp
Rotating.cpp                   Sequence.cpp
Shockrifle.cpp                 Train.cpp
func_door.cpp                  generic_lua_weapon.cpp
global.cpp                     main.cpp
enemyMatCallback.cpp           neutralMatCallback.cpp
npc_aerial.cpp                 npc_enemy.cpp
npc_friend.cpp                 npc_neutral.cpp
npc_template.cpp               Ladder.cpp
```

Three root implementation files with direct references are not listed by the
old project (`Breakable.cpp`, `netrualMatCallback.cpp`, and
`PlayerCollisionForceAdder.cpp`). They are recorded as orphaned compatibility
evidence, not silently promoted to the build. AIR3 has its own direct
`OgreNewt::World`, `Body`, and `BasicRaycast` coupling in `AirPathFind.cpp` and
related headers; it is explicitly deferred to Step 6C. Shared legacy headers
also expose Newton types, so source-only counts understate the later migration.

The categories below overlap by design. Counts are not used as completion
claims; removal gates are repository searches plus behavior tests.

## Operation classification and mapping

| Category | Live OgreNewt operations and assumptions | Run3/Bullet mapping | Migration step |
|---|---|---|---|
| World/step | `OgreNewt::World`, `BasicFrameListener`, `setWorldSize`, `getTimeStep`, default material lookup, and custom global-player-position storage. The old frame listener couples simulation frequency to rendering. | `PhysicsWorld::advance` owns an exact 1/60 s accumulator with bounded catch-up. Broadphase bounds are backend policy; gameplay-owned global actor state does not belong in physics. | 6A backend; call sites 6B/6C |
| Bodies/lifetime | Raw `new/delete OgreNewt::Body`, `attachToNode`, mass/inertia setters/getters, collision replacement, center of mass, and occasional constructor type IDs. Ownership is inconsistent and map unload is manual. | Move-only `BodyHandle` destroys its backend body through a weak owner token. `BodyDesc` declares motion, kilograms, shape, transform, collision filter, trigger flag, sleep policy, and typed metadata. Ogre scene-node synchronization stays outside physics. | 6A API; call sites 6B/6C |
| Shapes | `Box`, `Cylinder`, `Ellipsoid`, mesh `TreeCollision`, `ConvexHull`, `CompoundCollision`; ragdolls also mention capsule/cone. Inertia is manually calculated. | Step 6A implements tested box, Y-axis capsule, and indexed triangle-mesh shapes. Bullet calculates dynamic inertia. Ellipsoid/player fitting, convex/compound/cylinder/cone additions are made only when a live Step 6B/6C fixture requires them. Triangle meshes are static/kinematic only. | 6A subset; extensions 6B/6C |
| Transforms | `set/getPositionOrientation`, `attachToNode`, custom transform callback, and direct scene-node reads. | `Transform` is in game units and independent of Ogre. The backend retains previous/current transforms and exposes an accumulator-alpha interpolated transform. Rendering pulls transforms after stepping. | 6A backend; binding 6B/6C |
| Forces/motion | Custom force-and-torque callbacks, standard gravity callback, `addForce`, `setForce`, `setTorque`, standard/additional/energy force fields, linear velocity, omega, and train/player bespoke force logic. | Center force and impulse plus linear velocity are defined now. Gravity is a world property. Game force/impulse units are converted once at the adapter. Torque, angular control, and gameplay force composition wait for covered consumers. | 6A subset; gameplay 6B/6C |
| Raycasts | `BasicRaycast`, `getFirstHit`, indexed hits, fractional `mDistance`, normal, and body pointer. Used by player floor/wall/use checks, weapons, blast/energy, NPC sight/floor tests, and AIR3. Several sites manually search for the nearest non-self hit. | `RaycastQuery` carries group/mask filtering. `raycastAll` always returns stable nearest-first hits with body ID, metadata, point, normal, and normalized fraction; `raycastClosest` is the safe convenience form. | 6A backend; consumers 6B/6C |
| User data/type | `set/getUserData` uses raw pointers or strings; patched OgreNewt adds numeric `set/getType`, name, damage, use, and force fields. Magic values identify player, NPC, ladder, train, and generic physical objects. | `BodyMetadata` contains stable `entityId` and `type` integers. Names, health/damage, use state, and gameplay force accumulators remain gameplay data. Bullet user pointers refer only to backend-owned records. | 6A metadata; consumers 6B/6C |
| Material/contact | `MaterialID`, `MaterialPair`, friction/softness/elasticity, continuous-collision flags, and `ContactCallback` subclasses reading bodies, position/normal, speed, and force. Callbacks directly mutate gameplay during the physics step. | Collision groups/masks replace identity-only material routing. Manifolds become copied `ContactEvent` values (`Began`, `Persisted`, `Ended`) queued per fixed step and drained afterward. Trigger pairs have no contact response. Gameplay never runs inside Bullet callbacks. | 6A queue; policies 6B/6C |
| Freeze/sleep | `setAutoFreeze(0)`, `freeze`, `unFreeze`/`unfreeze`, zero velocity, and script-facing freeze toggles. The old code often disables automatic sleeping to keep controllers/trains responsive. | Explicit enabled/disabled simulation, sleeping-allowed policy, `sleepBody`, `wakeBody`, and `isBodySleeping`. Applying force/impulse wakes dynamic bodies. | 6A backend; tuning 6B/6C |
| Joints | Player/NPC/train `BasicJoints::UpVector`; train pin changes; ragdoll `BallAndSocket` limits and `Hinge` angle/stop-acceleration callbacks. Raw joint ownership is unclear. | Move-only `Constraint` with a tested point constraint proves RAII and body-lifetime cleanup. Character orientation constraints and ragdoll angular limits require dedicated behavior fixtures before adding corresponding adapters. | RAII base 6A; concrete mappings 6B/6C |
| Ragdolls | `RagBone` builds per-bone convex bodies, custom force/transform callbacks, center of mass, ball/hinge joints, limits, user pointers, and manual recursive disposal. | No ragdoll migration in 6A. The body/constraint RAII and metadata foundations are exercised now; ragdolls migrate last in Step 6C with unload/lifetime tests. | 6C |

## Units and numeric contract

- Ogre, content, and gameplay remain in game units.
- Bullet receives metres through one `UnitConversion` object.
- Production uses exactly `0.01 metre/game unit` (100 game units = 1 metre).
- An alternate scale can only be obtained from the deliberately separate
  `run3/physics/PhysicsTesting.hpp` test-support API.
- Positions, lengths, linear velocities, gravity, center forces, and impulses
  scale by metres-per-game-unit. Mass remains kilograms; time remains seconds;
  unit quaternions and normalized directions are dimensionless.
- Public tolerances are expressed in game units. Step 6A tests use loose
  settling tolerances because Bullet contact stabilization is iterative; fixed
  step counts and filtering/order are exact expectations.

## Step and event contract

`PhysicsWorld::advance(frameSeconds)` accumulates non-negative finite frame
time. It runs zero or more exact 1/60 s Bullet steps, never Bullet's internal
variable-step accumulator. At most `maxCatchUpSteps` are run per call; excess
whole steps are reported as dropped time while the sub-step remainder is kept.
Consequently, equal elapsed time at 30, 60, and 144 render Hz produces equal
step counts unless a frame hits that explicit catch-up bound.

Contacts are discovered after each fixed step and copied into a FIFO queue.
`drainContactEvents` transfers and clears that queue. Events contain IDs and
values, never borrowed Bullet pointers. Destroying a body or world invalidates
its weak handles safely and removes dependent constraints.

## Step 6A test mapping

| Legacy risk | Test contract |
|---|---|
| Scattered centimetre/metre constants | standard and test-only conversion round trips |
| Render-rate-dependent Newton update | equal 30/60/144 Hz step counts and bounded catch-up |
| Gravity/force callback behavior | gravity, center force, impulse, and wake behavior |
| Primitive/tree collision creation | box, capsule, and indexed mesh construction/raycast |
| Player/NPC hand-sorted ray hits | stable fraction order and group/mask filtering |
| Contact callback re-entrancy | queued begin/persist/end trigger events with copied metadata |
| `setAutoFreeze`, `freeze`, `unFreeze` | automatic sleep, explicit sleep/wake, enable/disable |
| Raw bodies/joints surviving map unload | move-only handle and dependent-constraint cleanup |
| Scene interpolation jitter | previous/current transform interpolation using accumulator alpha |

Step 6A is complete only when these tests pass with the Bullet and null
backends on supported compilers and the repository check confirms that no
gameplay/public header added by this step includes a Bullet header.
