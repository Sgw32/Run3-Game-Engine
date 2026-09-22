# Step 8D NPC runtime

`NpcSystem` is owned by one loaded map. It consumes the side-effect-free Step
8B definitions and generation-checked entity handles, uses `AirPathFind`
through `IPhysicsQuery`, advances only on Run3's fixed 60 Hz gameplay tick,
and emits typed commands to presentation, physics, audio, and scripts. Its
public API contains no Ogre, Bullet, legacy singleton, or raw global-pointer
types. The Long Way content remains read-only.

## Architecture and extension model

An NPC has an authored data record, a small explicit state (`Idle`,
`Navigating`, `Blocked`, `Reached`, or `Dead`), and typed events. Behavior does
not depend on a C++ class per character. `npc_neutral` and `npc_enemy` select
tested policy data; friend/aerial variants are rejected with source context
because the selected low campaign does not declare them. This is the intended
replacement for `NPCManager` and its class vectors.

New characters should normally be assembled from data and Lua:

1. Put physical/render defaults in the NPC definition.
2. Express story-specific reactions with `cNearScript`, `scriptOnReach`,
   `scriptOnDeath`, and event 19 (`SetUseScript`).
3. Add reusable state-machine policies to `NpcSystem`, not another singleton
   or Ogre-dependent subclass.
4. Add a deterministic fixture for every new transition and invalid state.

Scripts submit commands; they do not own NPC lifetime. Unknown numeric events
and events that are known but not implemented for an active class fail with
the NPC name and event code. A missing named NPC follows the established
campaign compatibility rule: it logs a warning and skips that command.

## Legacy event ABI

The numeric values are content ABI and must never be renumbered.

| Code | Typed event | Current disposition |
|---:|---|---|
| 0 | `RunTo` | path to current player position |
| 1 / 2 / 3 | `Alert` / `Fear` / `Crazy` | legacy movement multipliers 2 / 0.3 / 0.3 |
| 4 / 5 | `Stop` / `Resume` | deterministic suspend/resume |
| 6 | `Kill` | death script, collision removal, optional ragdoll |
| 7 / 8 | `Weapon` / `Shoot` | rejected; no selected-map behavior implementation yet |
| 9 | `Spawn` | restores visibility/collision and idle state |
| 10 | `GoTo` | AIR3 path to authored vector; blocked state is visible |
| 11 / 12 | `SetAi` / `Reach` | rejected; selected maps do not require direct handling |
| 13 / 14 | `TakeOff` / `Land` | rejected for neutral/enemy; aerial class is not live |
| 15 | `SetAnimation` | sets animation and stops movement, matching legacy |
| 16 | `Teleport` | absolute typed transform |
| 17 | `SetParent` | captures a relative transform to a named train/entity |
| 18 / 19 | `SetGoalScript` / `SetUseScript` | replaces the active Lua callback |
| 20 | `RotateOverride` | absolute quaternion |
| 21 | `TeleportParent` | parent-relative teleport |
| 22 | `ToggleGravity` | rejected; legacy implementation was empty |
| 23 | `TransitAnimation` | switches through the animation service; blend parity pending |
| 24 | `SetMoveActivity` | validated non-negative multiplier |
| 25 | `ResetParent` | releases parent transform following |
| 26 | `SetGravity` | rejected; dynamic-character gravity parity pending |
| 27 | `FacialActivity` | parses the facial XML and plays its voice as positional audio; pose morphs/subtitles pending |
| 28 | `ToggleFlashlight` | rejected; light presentation belongs to the later lighting step |
| 29-32 | attach/detach physical object variants | typed bone attachment with shared map physics disabled while attached |

`npcEvent`, `npcEvent2`, `__all_npcEvent`, and `destroyNPC` route to these
typed operations. Broadcast exists because `__all_npcEvent` is present in the
955-script API and content, even though selected map startup does not require
it. No `npcgroup` declaration exists in the selected variant, so the unused
experimental group machinery was not revived.

## Implemented legacy data and behavior

- Name, class, mesh, material override, scale, spawn transform, `yShift` yaw,
  render distance, velocity, stop distance, health, headshot threshold,
  animation enable/default, proximity/use/reach/death scripts, hand bone,
  parent motion, teleport, attachments, and ragdoll request are preserved.
- Neutral movement and callbacks use a deterministic path/state transition.
  Enemy policy periodically acquires a nearby player and issues bounded damage
  at the authored attack distance. Both use the same AIR3 query and Bullet NPC
  collision path; no second navigation or collision world was introduced.
- NPC diffuse materials go through the existing legacy-material compatibility
  adapter. Animation advancement and positional voice audio remain in the Ogre
  service boundary.
- Destruction is reverse-order and map scoped. It detaches physical objects,
  removes NPC/ragdoll bodies and voices, destroys presentation, invalidates
  explicitly destroyed registry handles, and cannot survive a map unload.

## Intentional/incomplete differences

This is a tested Step 8D vertical slice, not a claim of complete 1:1 combat or
facial presentation. The legacy random animation timing, per-bone head/look
tracking, animation cross-fade weights, subtitle/mouth-pose output, detailed
footstep/attack/random sound set, dynamic gravity/floor resolution, blood/gib
effects, flashlight presentation, and attachment offsets need content-specific
fixtures before parity can be claimed. Enemy line-of-sight/attack animation is
currently a bounded distance policy. Ragdoll construction uses the existing
generic Step 6C constraint rig rather than legacy `auto.xml` bone mapping.

These gaps stay visible in logs/tests and are not implemented as silent
placeholders. Adding them should extend the Run3-owned interfaces and state
machine; it must not restore `NPCManager`, OgreNewt, or global callbacks.

## Verification

`run3_step8d_tests` covers neutral/enemy construction, AIR3 success and blocked
paths, 30/60/144 deterministic replay, reach/near/use/death callbacks,
animation changes, parent/train following and relative teleport, damage,
headshot classification, enemy attack scheduling, ragdoll commands, explicit
destruction, reverse unload, invalid/missing commands, all 19 `tlwcao` and 28
`tlwhome02` declarations, and referenced NPC Lua target existence. Installed
Windows Debug D3D11 two-frame smokes construct the 19 `tlwcao` and 28
`tlwhome02` NPC meshes, run startup Lua including parent and attachment
commands, render, and exit cleanly.
