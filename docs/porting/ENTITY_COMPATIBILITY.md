# Step 8B entity compatibility inventory

## Step 8C runtime overlay (2026-09-22)

This inventory's declaration and attribute counts remain unchanged. The map-owned
`SequenceRuntime` now constructs and schedules the following exact-tag slices from
the Step 8B definitions and map registry: 48 `button`, 171 `door`, 10
`darkzone`, 4 `ladder`, 18 startup `lua`, 5 `onexit`, 68 `pendulum`, 166
`rot`, 70 `timer`, 80 `train`, and 91 `trigger`. A miniature fixture also
exercises `pickup` (zero declarations in the selected campaign). `trigger`
event bindings (65) schedule `lua`, `door`, `player`, `hurt`, `changelevel`,
and `entc` actions, with `entc` explicitly logged as deferred. The 20
`cutscene` event bindings and their 112 nested `run` actions remain deferred
to Step 8E; they are not active SequenceRuntime events. The selected maps still
have 154 NPC and 31 computer declarations, owned by Steps 8D and 8E.

The Step 8C service creates Ogre presentations and kinematic Bullet bodies,
while existing Step 6C owns the physics world and Step 7 owns audio. Authored
buttons with a corresponding scene object use its world-space bounds for a
ray-query collider, without duplicating that visible mesh. Unsupported nested
train visual components and non-presented authored lights produce diagnostics.
An absent optional light does not abort legacy Lua startup; unknown required
doors, trains, timers, triggers, and event targets produce contextual errors.
Triggered `lighton`/`lightoff` values are captured and restored on exit where
those lights exist. `SequenceRuntime` contains no frame listener or singleton.

The corrected `tlwcao` D3D11 startup identified 21 buttons without a
matching already-presented scene object; they use their Sequence-authored
mesh/transform fallback and log the case. The map still ran for two frames
with a stable player spawn. This is a **partial behavioral implementation**,
not proof that all listed
attributes are honored. See [SEQUENCE_RUNTIME.md](SEQUENCE_RUNTIME.md) for
exact semantics, known gaps, and deterministic replay evidence. The table below
remains the Step 8B baseline; interpret its historical `required/deferred 8C`
labels together with this dated overlay.

Audit date: 2026-09-18  
Selected content: author-provided `Games/The Long Way/TheLongWay`, `low` map
variant. Content was read only; no SDK or game file was copied or changed.

## Contract and evidence

Step 8B separates parsing from construction:

- `run3::content::loadMapDefinition` reads `scene.cfg`, its exact-case
  `Scene=` and optional `Sequence=` paths, DotScene XML, and embedded
  `<integratedSequence>` blocks through the private TinyXML2 adapter. It
  preserves every element, attribute, text value, source file/line, and source
  order in `MapDefinition`/`SequenceDefinition`; it creates no Ogre, physics,
  audio, script, or gameplay object.
- Integrated sequences are ordered before the external sequence, matching the
  legacy `DotSceneLoader` call that ran embedded data while loading the scene.
  The selected campaign has 0 integrated sequences and 18 external sequence
  files. The miniature fixture covers the mixed case.
- `EntityRegistry` uses generation-checked `EntityId`/`EntityHandle` values,
  retains duplicate declarations in authored order, reports duplicates, and
  makes legacy-compatible name lookup select the first declaration. Required
  event targets resolve after construction; missing and ambiguous targets have
  file/line diagnostics. Map cleanup runs owner callbacks in reverse creation
  order and invalidates every old handle.
- `StaticMap` now consumes the definition tree instead of regex-scanning XML.
  Existing render/static/dynamic-physics creation is an adapter over that tree;
  created objects and bodies can be associated with registry handles. This step
  does not implement entity state machines.
- All attributes below are preserved even when their future owner has not yet
  consumed them. Consequently an unsupported attribute is inventory-visible,
  not silently discarded. Unknown elements are preserved and diagnosed; strict
  parsing can reject them.

Reproduce the census after building with:

```powershell
build/windows-msvc-x64-debug/run3_entity_inventory.exe `
  "Games/The Long Way/TheLongWay" low
```

The inventory tool is read only and emits one `MAP`, `SCENE`, `DECL`, or
`EVENT` record per line.

Status meanings:

- **supported**: Step 8B or an earlier ported subsystem consumes it.
- **required/deferred**: live campaign content; named later step owns behavior.
- **unused**: legacy loader supports it, but the selected variant has zero use.
- **retired**: content deliberately renamed the tag so the exact-match legacy
  loader did not consume it, or the old subsystem has been replaced.

## Per-map direct declarations and event bindings

Counts are only direct children of `<adents>` and `<events>`. Nested action,
transform, frame, mesh, and key-point tags are inventoried separately below.

| Map | Direct `<adents>` declarations | Direct `<events>` bindings |
|---|---|---|
| `background` | `lua=1, onexit=1, timerv=1` | none |
| `tlwback01` | `cutscene=1, lua=1` | `cutscene=1` |
| `tlwback02` | `cutscene=1, lua=1, timerv=2` | `cutscene=1` |
| `tlwcao` | `button=21, computer=4, cutscene=1, darkzone=1, door=29, ladder=2, lua=1, npc=19, rot=2, timer=33, train=2, trigger=20` | `cutscene=1, trigger=10` |
| `tlwcredits` | `cutscene=1, lua=1, onexit=1` | `cutscene=1` |
| `tlwdelusion01` | `cutscene=1, lua=1, onexit=1` | `cutscene=1` |
| `tlwdelusion02` | `cutscene=1, door=1, lua=1, npc=10, rot=2, train=1` | `cutscene=1` |
| `tlwdelusion03` | `cutscene=1, lua=1, npc=1` | `cutscene=1` |
| `tlwdelusion04` | `cutscene=1, lua=1, onexit=1, pendulum=1, rot=1, timer=3, train=1, traind=1` | `cutscene=1` |
| `tlwdelusion05` | `cutscene=2, door=4, doord=1, lua=1, npc=1, rot=1, timer=1, train=1` | `cutscene=2` |
| `tlwdolg` | `button=9, computer=7, cutscene=1, darkzone=1, door=42, ladder=2, lua=1, npc=36, npcv=2, pendulum=1, rot=43, rotd=3, timer=6, train=22, traind=2, trigger=37` | `cutscene=1, trigger=26, triggerd=1` |
| `tlwhome01` | `button=6, buttonv=1, cutscene=4, cutscened=1, darkzone=1, door=5, lua=1, pendulum=1, timer=5, train=4, trigger=7` | `cutscene=3, trigger=7` |
| `tlwhome02` | `button=11, computer=5, cutscene=3, darkzone=2, door=50, lua=1, npc=28, rodt=1, rot=40, rotd=1, timer=3, train=14, traind=1, trainv=1, trigger=23, triggerd=1, triggerv=1` | `cutscene=2, trigger=19, triggerv=2` |
| `tlwintro` | `cutscene=1, door=4, lua=1, npc=5, npcd=8, npcv=2, onexit=1, pendulum=5, pendulumd=1, rot=60, timer=4, train=12` | `cutscene=1` |
| `tlwoutro` | `cutscene=1, door=11, lua=1, npc=4, pendulum=55, rot=2, timer=3` | `cutscene=1` |
| `tlwstations01` | `darkzone=2, door=1, doord=1, lua=1, npc=15, pendulum=3, train=9, trigger=1` | `trigger=1` |
| `tlwstations02` | `blockboxes=1, button=1, computer=15, cutscene=3, door=21, doord=1, lua=1, npc=20, npcold=1, npcv=3, pendulum=2, rot=15, timer=8, train=7, traind=1, trigger=2` | `cutscene=2, trigger=2` |
| `tlwstations03` | `darkzone=3, door=3, doord=1, lua=1, npc=15, timer=4, train=7, trigger=1` | none |

Campaign totals for exact live direct tags are: `button=48`, `computer=31`,
`cutscene=23`, `darkzone=10`, `door=171`, `ladder=4`, `lua=18`, `npc=154`,
`onexit=5`, `pendulum=68`, `rot=166`, `timer=70`, `train=80`, and
`trigger=91`. NPC classes are `npc_neutral=146` and `npc_enemy=8`.

Tags ending in `d`/`v`, plus `npcold`, `rodt`, and `blockboxes`, are preserved
author data but were not dispatched by the legacy exact-name `Sequence`
loader. They are **retired**, not aliases to activate automatically.

## DotScene observed schema

Every observed tag and attribute in the 18 selected scenes is listed here.
Counts include nested occurrences. `Parser` always means Step 8B preserves the
complete record; the runtime owner identifies who consumes or will consume it.

| Tag (count) | Observed attributes | Runtime owner | Status/evidence |
|---|---|---|---|
| `scene` (18) | `formatVersion, glass, glow, multiplier` | Parser/StaticMap | supported |
| `nodes` (18), `node` (5586) | node: `ParentSceneNode, PositionX, PositionY, PositionZ, SceneNode, Visible, id, mass, name, p_shiftX, p_shiftY, p_shiftZ, p_sizeX, p_sizeY, p_sizeZ, position` | StaticMap | supported hierarchy/name preservation |
| `position` (5705) | `m, x, y, z` | StaticMap | supported |
| `rotation` (5690) | `qw, qx, qy, qz, w, x, y, z` | StaticMap | supported |
| `scale` (5700) | `m, x, y, z` | StaticMap | supported |
| `entity` (1637) | `buildTangents, castShadows, materialFile, meshFile, name, poMd, run3batcher, scaleU, scaleV, scrollU, scrollV` | StaticMap/render | supported geometry; advanced flags Step 9 |
| `subentity` (1431) | `index, materialName` | StaticMap/render | supported |
| `nocollide` (3722) | `castShadows, materialFile, materialName, maxDist, meshFile, name, run3batcdher, run3batchder, run3batchedr, run3batcher, run3batcherv, run3batdcher` | StaticMap | supported render-only object; misspelled batching attrs preserved |
| `subnocollide` (1334) | `index, materialFile, materialName, run3batcher` | StaticMap | supported material mapping |
| `phys` (200) | `castShadows, mass, materialFile, meshFile, name, poMd, rfun3batcher, rudn3batcher, run3baftcher, run3batcher, run3bfatcher, run3dbatcher, runs3batcher` | StaticMap/Step 6C | supported body construction; misspellings preserved |
| `subphys` (76) | `index, materialName` | StaticMap | supported definition, material adapter pending |
| `breakable` (8) | `box, castShadows, explosive, gibMesh, gibScale, meshFile, name` | Step 6C/8C | body supported; behavior deferred 8C |
| `pblock` (12) | `castShadows, meshFile, name` | StaticMap/Step 6C | supported invisible collision |
| `particleSystem` (97) | `castShadows, file, meshFile, name` | presentation | required/deferred 9A |
| `fire` (4) | `pSys, renderDist` | presentation | required/deferred 8E/9A |
| `light` (20) | `castShadows, dist, name, type` | rendering | required/deferred 9B |
| `colourDiffuse` (20), `colourSpecular` (20) | `b, g, r` | rendering | required/deferred 9B |
| `lightAttenuation` (20) | `range` | rendering | required/deferred 9B |
| `lightRange` (20) | `inner, outer` | rendering | required/deferred 9B |
| `dynamic` (1) | `b, g, r` | rendering | required/deferred 9B |
| `normal` (20) | `x, y, z` | rendering | required/deferred 9B |
| `aiNodes` (14), `npcnode` (210) | npcnode: `drawNPCNode, m, x, y, z` | NpcSystem/AIR3 | required/deferred 8D |
| `environment` (18) | none | Parser | supported container |
| `player` (17) | `fov, mpr, startFreeze, x, y, z` | StaticMap/player | spawn supported; remaining fields deferred 8C/9A |
| `newtonWorld` (16) | `x1, x2, y1, y2, z1, z2` | Step 6 physics | retired as Newton config; preserved for bounds comparison |
| `fog` (17) | `mode` | rendering | required/deferred 9A/9B |
| `skyBox` (15) | `material` | rendering | required/deferred 9A |
| `colourAmbient` (17) | `a, b, g, r` | rendering | required/deferred 9B |
| `fade` (17) | `duration, material, overlay, speed, startFade` | UI/presentation | required/deferred 8E/9A |
| `hud` (4) | `show` | UI | required/deferred 9A |
| `farClip` (1) | `dist` | camera | required/deferred 9A |
| `sounds` (5) | none | Step 7 audio | supported container |
| `ambient` (67) | `distance, id, loop, m, maxDistance, minGain, name, objname, x, y, z` | Step 7 audio | supported/deferred named control |
| `portal` (5) | `farClip` | map/presentation | required/deferred 8E/9A |
| `sunColor` (1), `sunPos` (1) | `x, y, z` | rendering | required/deferred 9B |
| `pos` (1) | `x, y, z` | Parser | preserved; content-specific child, deferred with owner |
| `nodes1` (1), `nodev` (114), `npcnoded` (3), `playerd` (9), `ambientd` (5), `soundd` (1), `clippingd` (1), `colourAmbientd` (1), `colourAmbientf` (1), `waterv` (1) | same family attributes shown by inventory output | none | retired disabled variants; legacy exact dispatch ignored them |
| `ohrana` (1), `rocketworker` (1), `rocketworkers` (1), `suicide` (1) | none | none | unused content markers; preserved/diagnosed |
| `taxist` (1) | `comment` | none | unused content marker; preserved/diagnosed |

Legacy DotScene tags described by `DotSceneLoader` but absent from this variant
are still recognized by the definition schema: `integratedSequence`,
`externals`, `camera`, `lookTarget`, `trackTarget`, `physbox`, `physcyl`,
`blockbox`, `ragdoll`, `tree`, `mirror`, `billboardSet`, `plane`, `water`,
`skyx`, `skyDome`, `skyPlane`, `music`, `sound`, `terrain`, `portals`, `zone`,
`userDataReference`, and `octree`. Their legacy attributes/children remain
generic preserved data. Status is **unused** until another content variant or
fixture proves use; visual implementations belong to Step 9, physics forms to
Step 6/8C, audio to Step 7, and integrated sequence ingestion is already
supported by the Step 8B fixture.

Exact attributes on every observed disabled/custom DotScene tag are:

| Tag | Exact observed attributes |
|---|---|
| `ambientd` | `distance, id, loop, maxDistance, minGain, name, objname, x, y, z` |
| `clippingd` | `far, near` |
| `colourAmbientd`, `colourAmbientf` | `b, g, r` |
| `nodes1`, `ohrana`, `rocketworker`, `rocketworkers`, `suicide` | none |
| `nodev` | `ParentSceneNode, PositionX, PositionY, PositionZ, SceneNode, Visible, id, mass, name, p_shiftX, p_shiftY, p_shiftZ, p_sizeX, p_sizeY, p_sizeZ` |
| `npcnoded` | `drawNPCNode, x, y, z` |
| `playerd` | `fov, m, x, y, z` |
| `soundd` | `distance, id, loop, maxDistance, name, rollOff, x, y, z` |
| `taxist` | `comment` |
| `waterv` | `fileName` |

For absent legacy tags, the audited legacy contract is: `integratedSequence`
contains `sequence` or direct `adents/events`; `camera` uses identity and
transform/target children; `physbox/physcyl/blockbox` use identity, mesh,
material, mass, and transform data; `ragdoll` uses `name, meshFile,
scriptFile`; `tree` uses `name, treeFile`; `mirror` uses `metalTexture`;
`particleSystem` uses `name, file`; `billboardSet/plane` preserve their Ogre
definition attributes; `water/skyx` use a configuration filename; sky tags use
material plus distance/curvature/tiling/plane values; `sound/ambient/music`
use file/name, loop, gain/distance, and position/object binding values;
`zone/portal/portals` use bounds or far-clip and embedded entities; and
`userDataReference/octree/terrain/externals` retain generic child data. Since
none occurs in the selected variant, there is no invented attribute count.

## Sequence observed schema

### Direct declaration ownership

| Direct tag | Count | Implementing owner | Status/evidence |
|---|---:|---|---|
| `button` | 48 | SequenceRuntime + Step 6C | required/deferred 8C |
| `computer` | 31 | Computer system | required/deferred 8E |
| `cutscene` | 23 | Cutscene system | required/deferred 8E |
| `darkzone` | 10 | SequenceRuntime/presentation | required/deferred 8C/9B |
| `door` | 171 | SequenceRuntime + Step 6C | required/deferred 8C |
| `ladder` | 4 | SequenceRuntime/player | required/deferred 8C |
| `lua` | 18 | SequenceRuntime/ScriptEngine | required/deferred 8C |
| `npc` | 154 | NpcSystem | required/deferred 8D |
| `onexit` | 5 | SequenceRuntime/ScriptEngine | required/deferred 8C |
| `pendulum` | 68 | SequenceRuntime + Step 6C | required/deferred 8C |
| `rot` | 166 | SequenceRuntime + Step 6C | required/deferred 8C |
| `timer` | 70 | SequenceRuntime | required/deferred 8C |
| `train` | 80 | SequenceRuntime + Step 6C | required/deferred 8C |
| `trigger` | 91 | SequenceRuntime + Step 6C | required/deferred 8C |
| `blockboxes` | 1 | none | retired unknown legacy-dispatch tag |
| `buttonv`, `cutscened`, `doord`, `npcd`, `npcold`, `npcv`, `pendulumd`, `rodt`, `rotd`, `timerv`, `traind`, `trainv`, `triggerd`, `triggerv` | 38 total | none | retired disabled/alternate spellings; preserved, never auto-aliased |

Legacy direct tags with zero selected-variant declarations are `pickup`,
`event`, `flare`, `fire`, `npcgroup`, `seqscript`, and `fuzzy`. They are
**unused**, not required merely because `Sequence.cpp` contains a processor.
If another selected variant proves them live, ownership is respectively 8C,
8C, 8E/9A, 8E/9A, 8D, 8C, and explicit review for the experimental fuzzy
machinery.

### Declaration elements and attributes

This table includes direct declarations and all nested definition elements.

| Tag (count) | Observed attributes |
|---|---|
| `button` (48) | `buttonName, id, luaScript, meshName, name` |
| `computer` (31) | `allowVirtualDisplay, cNearScript, cShutScript, dispMat, materialFile, meshFile, name, script` |
| `cutscene` (23) | `inf, length, music, musicFile, musicLength, name, unfreezea, x, y, z` |
| `darkzone` (10) | `darken, name` |
| `door` (171) | `allowPReg, box, closeSound, dirX, dirY, dirZ, distance, lLocked, lOnClosed, lOnOpen, materialFile, mesh, mul, name, openSound, parent, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z, z2` |
| `ladder` (4) | `luaScript, meshName, name` |
| `lua` (18), `onexit` (5) | `script` |
| `npc` (154) | `animated, applyGravity, attackAnimDist, cNearScript, className, fac_anim, handBone, headAxis, headBone, headshot, materialFile, meshFile, meshFiled, name, ragdoll, renderDist, rotateSpeed, sounds, sounds2, stopAtDist, stopDist, strange_look, velocity, yShift` |
| `pendulum` (68) | `dirX, dirY, dirZ, materialFile, mesh, name, parent, pitch, positionPend, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z` |
| `rot` (166) | `dirX, dirY, dirZ, materialFile, mesh, name, parent, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z` |
| `timer` (70) | `lua, name, noiseAmount, period` |
| `train` (80) | `dist, inf, materialFile, movingSound, name, setor, setor2, simple, soundDuration, speed, start, startSound, stopSound, yaw` |
| `trigger` (91) | `footstep, luaOnEnter, luaOnLeave, multiple, name, show, sw` |
| `angle` (170) | `f` |
| `axis` (170) | `x, y, z` |
| `farFind` (170) | `dist` |
| `frame` (210) | `mul, or, orient, p, pos, second, x, y, z` |
| `frames` (16) | `or, orient, second, x, y, z` |
| `keyPoint` (342) | `mul, script, scriptd, x, y, z` |
| `lightoff` (6), `lighton` (4) | `name` |
| `entity` (155) | `castShadows, meshFile, name, run3batcher` |
| `nocollide` (36) | `castShadows, meshFile, name, run3batcher` |
| `object` (39) | none |
| `physPosit` (167), `physSize` (170), `position` (597), `scale` (597) | vector fields `x, y, z`; position/scale may have `mul`, position also `z2` |
| `rotate` (89), `rotation` (354) | `qw, qx, qy, qz`; rotation may have `mul` |
| `psys` (86) | `psysName` |
| `subnocollide` (2) | `index, materialName` |
| `blockboxes`, `framefix`, `rocketflight`, `rocketpart`, `vodnikipart` | no attributes observed; preserved content-specific markers |
| disabled variants `buttonv, cutscened, doord, entityd, keyPointd, npcd, npcold, npcv, objectd, pendulumd, physPositd, positiond, psysd, rodt, rotated, rotater, rotd, timerv, traind, trainv, triggerd, triggerv` | see exact table below; status retired |

Exact attributes for every observed variant and custom marker are:

| Tag | Exact observed attributes |
|---|---|
| `buttonv` | `luaScript, meshName` |
| `cutscened` | `length, name, unfreezea, x, y, z` |
| `doord` | `box, closeSound, dirX, dirY, dirZ, distance, mesh, name, openSound, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z, z2` |
| `entityd` | `meshFile` |
| `framed` | `or, orient, p, pos, second` |
| `framefix`, `rocketflight`, `rocketpart`, `vodnikipart`, `blockboxes`, `object`, `objectd` | none |
| `keyPointd` | `script, x, y, z` |
| `npcd`, `npcold` | `animated, attackAnimDist, className, headshot, meshFile, name, renderDist, sounds, sounds2, velocity, yShift` |
| `npcv` | `animated, attackAnimDist, cNearScript, className, fac_anim, headAxis, headBone, headshot, meshFile, name, renderDist, sounds, sounds2, stopAtDist, stopDist, strange_look, velocity, yShift` |
| `pendulumd` | `mesh, name, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z` |
| `physPositd`, `positiond` | `x, y, z` |
| `psysd` | `psysName` |
| `rodt` | `mesh, name, parent, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z` |
| `rotated`, `rotater` | `qw, qx, qy, qz` |
| `rotd` | `mesh, name, parent, pitch, qw, qx, qy, qz, roll, rotating, rotspeed, sX, sY, sZ, speed, useInteract, x, y, yaw, z` |
| `timerv` | `lua, name, period` |
| `traind` | `dist, movingSound, name, setor, setor2, simple, soundDuration, speed, start, startSound, stopSound, yaw` |
| `trainv` | `movingSound, name, setor, simple, soundDuration, speed, start, startSound, stopSound` |
| `triggerd` | `luaOnEnter, luaOnLeave, multiple, name, show, sw` |
| `triggerv` | `name, show, sw` |

### Event elements and attributes

Direct event bindings are `trigger=65` and `cutscene=20`; `seqscript=0`.
Renamed `triggerd=1` and `triggerv=2` are retired. Nested event actions are:

| Tag (count) | Observed attributes | Owner/status |
|---|---|---|
| `trigger` (65) | `name, sec` | 8C required/deferred |
| `cutscene` (20) | `name, wait` | 8E required/deferred |
| `run` (112) | `script, sec` | ScriptEngine/8C-8E required/deferred |
| `lua` (61) | `script` | ScriptEngine/8C required/deferred |
| `changelevel` (3) | `map` | map transition/8E required/deferred |
| `hurt` (4) | `damage` | player gameplay/8C required/deferred |
| `rudn` (1) | `script, sec` | none; preserved unknown content typo |
| `changeleveld` (1) | `map` | retired disabled variant |
| `rund` (6) | `script, sec` | retired disabled variant |
| `triggerd` (1), `triggerv` (2) | `name, sec` | retired disabled variants |

Legacy `Sequence` also accepts nested `entc`, `player`, and `door` actions and
`seqscript/run` bindings. They have zero selected-variant event occurrences and
remain **unused** until proven by another content variant; the parser already
preserves them.

## Duplicate, reference, and unknown policy

The legacy code stored many objects in vectors and commonly searched by name.
Changing duplicates into an unordered last-write-wins map would change content
semantics. Step 8B therefore:

1. keeps all duplicates in source order;
2. emits a duplicate diagnostic with the first declaration location;
3. resolves a name to the authored first declaration and reports ambiguity;
4. lets later behavior steps tighten a specific schema only after a fixture or
   campaign trace proves its intended fan-out behavior.

An unknown exact tag is never executed as a similarly named live tag. It is
retained in the definition, included in the inventory, and logged with its
source location; validation can enable strict rejection. This is essential for
the numerous deliberate `*d`/`*v` disabled records.

## Legacy defaults retained for behavior steps

Definitions preserve the distinction between an omitted attribute and an
authored empty/value attribute. Adapters must use the legacy defaults below;
they must not serialize defaults into or rewrite source XML.

| Entity | Audited legacy defaults |
|---|---|
| Scene/node transforms | `multiplier=1`; position `0,0,0`; rotation identity; scale `1,1,1` |
| Trigger | `name=undefined`, `show=false`, `sw=false`, `enabled=true`, `multiple=false`; legacy bounds `x1=0,x2=0,y1=0,y2=10,z1=10,z2=10` |
| Timer | `period=1`, `noiseAmount=0`, `start=false`, empty name/script |
| Train | `speed=30`, `dist=-1`, `setor=true`, `setor2=true`, `inf=false`, `yaw=180`, `simple=true`, `start=false`, default legacy train sounds |
| Button/ladder | mesh `box.mesh`; button `movable=false`, `mass=40`, `oncont=false` |
| NPC | mesh `ninja.mesh`, name `unnamed`, class `npc_enemy`, health `30`, velocity `1`, animated/gravity/landed `true`, stop distance `1`, rotate speed `50`, render distance `10000`, attack distance `130`, ragdoll/facial/headshot/exploding/strange-look `false` |
| Cutscene/seqscript | `length=20`, name `undefined`, `inf=false`; cutscene freeze/hide-HUD `true`, unfreeze/music/spline/from-camera `false`, skip speed `10` |
| Dark zone | name `undefined`, `darken=0.5`, `exp=0.1` |
| Door/rotator/pendulum | mesh `box.mesh`, scale `1,1,1`, distance `90`, speed `10`, direction `0,0,1`, axis `1,0,0`, rotating/use interaction `false`, sounds `nosound` |
| Pickup | mesh `box.mesh`, scale `1,1,1` |
| Computer | name `undefined`, mesh `pcomputer_01.mesh`, script `run3/lua/c64.lua`, display material `BLACK`, virtual display allowed |

The Step 8B StaticMap adapter already applies the scene/transform and physical
mass fallbacks at construction. Steps 8C-8E must test the remaining rows as
each behavior becomes live.

## Step boundary

Step 8B proves parsing, inventory, construction identity, cross-reference
resolution, and map-scoped lifetime only. Doors do not open, triggers do not
fire, NPCs do not think, cutscenes do not take the camera, and computers do not
capture input here. Those behaviors remain Steps 8C, 8D, and 8E.
