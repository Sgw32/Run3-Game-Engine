# NPC Classes Reference

This document describes how NPCs are created and controlled in Run3, including:

- how `Sequence` parses `<npc>` definitions,
- how `NPCManager` spawns and updates NPC instances,
- what shared behavior exists in `npc_template`,
- and what class-specific runtime options/events exist for each NPC class.

## 1) Overall runtime flow

1. `Sequence::processNPC` reads one `<npc>` XML entry, collects transform/gameplay/sound options, and fills an `NPCSpawnProps` object.  
2. `Sequence` passes `NPCSpawnProps` to `NPCManager::npc_spawn`.  
3. `NPCManager` constructs the class named by `className` (`npc_enemy`, `npc_neutral`, `npc_friend`, `npc_aerial`), initializes it, sets the nav node list, sends `SPAWN_NPC`, assigns physics material, then sends `RUNTO_NPC`.  
4. Each frame, `NPCManager::frameStarted` runs NPC updates on a fixed-step tick (`mStep`, default `0.05s`), then updates `AIManager`.

## 2) Spawn options (`<npc>` in Sequence)

`Sequence::processNPC` supports these commonly used options:

### Identity / class
- `name`
- `className` (default `npc_enemy`)
- `meshFile`
- `materialFile`

### Transform / physics shape
- Child nodes: `<position>`, `<rotate>`, `<scale>`
- Child nodes: `<physPosit>`, `<physSize>`, `<axis>`, `<angle>`

### Movement / behavior tuning
- `velocity`
- `farFind` (child `<farFind dist="..."/>`)
- `stopAtDist`, `stopDist`
- `yShift`, `rotateSpeed`
- `attackAnimDist`, `renderDist`
- `applyGravity`, `landed`, `ascendFromGround`, `takeOffBypass`

### Gameplay / effects
- `health`
- `scriptOnDeath` (`luaOnDeath`)
- `scriptOnReach` (`luaOnReach`)
- `cNearScript` (runs once when player is close)
- `headshot`, `headshotDist`, `headMesh`, `headBone`, `handBone`, `headAxis`
- `exploding`, `ragdoll`, `fac_anim`, `strange_look`
- `sounds`, `sounds2`

### Sound block
Optional `<sounds>` child supports:
- `soundAttack`, `soundHit`, `soundFind`, `soundRandom1`
- `soundSubRandom21`, `soundSubRandom22`, `soundSubRandom23`
- `soundFootstepPrefix`

## 3) Shared behavior (`npc_template`)

`npc_template` provides behavior reused by multiple NPC classes:

- Common state and spawn fields (`mProps`, physics body, scene nodes, animation pointers).
- Generic base events via `processBaseEvent`:
  - `SPAWN_NPC`, `RUNTO_NPC`, `STOP_NPC`, `SETANIM_NPC`,
  - `ALERT_NPC`, `FEAR_NPC`, `CRAZY_NPC`,
  - `FACIAL_ACTIVITY`, `TELEPORT_NPC`.
- Pathfinding helpers:
  - straight-line visibility checks (`check_straight`),
  - graph path search (`find_path`) through `AIManager` pathfinder.
- Proximity script trigger via `processNoticeStep` using `mProps.cNearScript`.

In short: class implementations mostly add class-specific reactions, while movement/path/event plumbing is standardized here.

## 4) Class-by-class notes

## `npc_enemy`

- Inherits `npc_template` and uses base event handling.
- Adds class-specific events:
  - `GOTO_NPC`: sets command destination and enters command mode.
  - `KILL_NPC`: blood/gibs/headshot/explosion branches and cleanup.
- Movement logic (`step`) tracks the player (or command destination), plays footsteps, pathfinds when needed, rotates toward path segments, and advances movement based on `moveActivity`.

Use when you need the default hostile ground-chasing behavior.

## `npc_neutral`

- Inherits `npc_template` and is the most feature-rich NPC variant.
- Adds additional events/options:
  - `TRANSIT_ANIMATION` (smooth animation transition),
  - `SETGRAVITY_NPC`, `SET_MOVEACTIVITY`,
  - `SETPARENT_NPC` / `RESETPARENT_NPC` / `TELEPORT_PARENT_NPC`,
  - `SETGOAL_SCRIPT`, `SETUSE_SCRIPT`,
  - `ROTATE_OVERRIDE_NPC`,
  - `TOGGLE_FLASHLIGHT`,
  - `ATTACH_PHYSOBJECT` / `DETACH_PHYSOBJECT`,
  - `ATTACH_PHYSOBJECT2` / `DETACH_PHYSOBJECT2`.
- Supports optional ragdoll-on-death flow and per-frame facial/look/animation transition helpers.

Use when you need scripted actor-style NPCs with attachments, parent relations, or richer in-game interactions.

## `npc_friend`

- Similar classic ground movement model to `npc_enemy`.
- Implements direct event handling for:
  - `SPAWN_NPC`, `RUNTO_NPC`, `STOP_NPC`,
  - `ALERT_NPC`, `FEAR_NPC`, `CRAZY_NPC`,
  - `GOTO_NPC`, `KILL_NPC`.

Use for friendly companions or non-hostile followers that still use standard navigation/chase movement.

## `npc_aerial`

- Air-oriented NPC class with two behavior modes (`behavior0`, `behavior1`).
- Supports all common runtime controls plus aerial-specific events:
  - `SETAI_NPC` (binds to AI group element and enables swarm-like behavior),
  - `TAKEOFF_NPC`, `LAND_NPC`,
  - `REACH_NPC` (runs `luaOnReach` script if set),
  - plus `GOTO_NPC`, `KILL_NPC`.

Use for flying/swarm NPCs or actors that need explicit takeoff/landing transitions.

## 5) Sequence/Lua control surface for NPCs

`Sequence::init` registers Lua bindings for NPC control:

- `npcEvent(name, type, param)` → dispatches to `NPCManager::npc_event(name, type, param)`.
- `npcEvent2(name, type, param1, param2)` → dispatches to `NPCManager::npc_event(name, type, param1, param2)`.
- `__all_npcEvent(name, type, param)` → broadcasts same event to all NPC collections.
- `destroyNPC(name)` → removes one NPC from manager vectors and deletes it.
- `setNPCManagerStep(step)` and `turnoff__NPCManager()` expose manager timing/pause controls.

This is the main runtime mechanism for firing the event flags listed above from scripts.

## 6) Practical authoring notes

- Always set a unique `name` in XML; manager routing uses NPC name matching for event delivery.
- If you plan to use command movement (`GOTO_NPC`), keep nav nodes available (`NodeList`) so pathfinding has valid graph nodes.
- For `npc_neutral` attachment events, verify `handBone` exists in the target skeleton.
- Prefer `npc_neutral` for heavy scripting/interactions; prefer `npc_enemy`/`npc_friend` for lighter patrol/chase style actors.

