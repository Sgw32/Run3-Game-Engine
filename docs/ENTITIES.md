# Sequence XML Entities Reference

This document describes the XML entities parsed by `Sequence.cpp` under the root `<sequence>` node.

## High-level structure

```xml
<sequence>
  <adents>
    <!-- world entities and scripted objects -->
  </adents>
  <events>
    <!-- runtime event bindings -->
  </events>
</sequence>
```

`Sequence::SetSceneSeq` looks for `<adents>` and `<events>`, then dispatches to specialized `process*` functions for each supported tag.

---

## `<adents>` entities

| Tag | What it does | Main attributes / children | Minimal XML example |
|---|---|---|---|
| `<trigger>` | Creates a trigger volume (`processTrigger`). Supports AABB mode and explicit position/scale mode (`sw=true`). | Attrs: `name`, `enabled`, `multiple`, `luaOnEnter`, `luaOnLeave`, box attrs (`x1..z2`) or `sw`; children: `<position>`, `<scale>`, `<lighton>`, `<lightoff>` | ```xml\n<trigger name="t_door" x1="0" y1="0" z1="0" x2="100" y2="40" z2="100" multiple="true" luaOnEnter="run3/lua/on_enter.lua"/>\n``` |
| `<pickup>` | Spawns a pickup object (`processPickup`). | Attrs: `mesh`, `event`, `object`, `value`, transform attrs (`x,y,z,qw,qx,qy,qz,sX,sY,sZ`) | ```xml\n<pickup mesh="crate.mesh" event="ammo" object="pistol_ammo" value="20" x="0" y="1" z="0"/>\n``` |
| `<computer>` | Creates an interactable computer terminal (`processComputer`). | Attrs: `name`, `meshFile`, `script`, `cShutScript`, `cNearScript`, `dispMat`, `allowVirtualDisplay`; children: `<position>`, `<rotate>`, `<scale>` | ```xml\n<computer name="pc_lab" meshFile="pcomputer_01.mesh" script="run3/lua/terminal.lua" dispMat="BLACK">\n  <position x="10" y="0" z="5"/>\n  <rotate qw="1" qx="0" qy="0" qz="0"/>\n  <scale x="1" y="1" z="1"/>\n</computer>\n``` |
| `<event>` | Defines a reusable event container (`processEvent`) that can spawn entities, move player, run Lua, drive doors/cutscenes, etc. | Attr: `name`; children: `<entc>`, `<player>`, `<changelevel>`, `<door>`, `<lua>`, `<cutscene>` | ```xml\n<event name="ev_alarm">\n  <lua script="run3/lua/alarm.lua" secs="0"/>\n  <door name="door_main" event="open" secs="0.2"/>\n</event>\n``` |
| `<flare>` | Adds a lens flare (`processLensFlare`). | Attrs: vector attrs for position + `scale` | ```xml\n<flare x="50" y="40" z="-20" scale="180"/>\n``` |
| `<timer>` | Creates named repeating Lua timer (`processTimer`). | Attrs: `name`, `lua`, `period`, `noiseAmount`, `start` | ```xml\n<timer name="tm_blink" lua="run3/lua/blink.lua" period="1.5" noiseAmount="0.2" start="true"/>\n``` |
| `<darkzone>` | Adds local darkening/exposure zone (`processDarkZone`). | Attrs: `name`, `darken`, `exp`; children: `<position>`, `<scale>`, optional `<light name="..."/>` links | ```xml\n<darkzone name="dz_hall" darken="0.6" exp="0.2">\n  <position x="0" y="0" z="0"/>\n  <scale x="20" y="10" z="20"/>\n  <light name="hall_light_01"/>\n</darkzone>\n``` |
| `<ladder>` | Creates ladder object (`processLadder`). | Attrs: `name`, `meshName`; children: `<position>`, `<rotation>`, `<scale>` | ```xml\n<ladder name="ladder_A" meshName="ladder.mesh">\n  <position x="0" y="0" z="0"/>\n  <rotation qw="1" qx="0" qy="0" qz="0"/>\n  <scale x="1" y="1" z="1"/>\n</ladder>\n``` |
| `<fire>` | Spawns fire particle effect (`processFire`). | Attrs: `name`, `pSys`, `renderDist`; children: `<position>`, `<rotation>`, `<scale>` | ```xml\n<fire name="fire_barrel" pSys="Examples/Fire" renderDist="3000">\n  <position x="2" y="0" z="3"/>\n</fire>\n``` |
| `<npc>` | Spawns one NPC through `NPCSpawnProps` (`processNPC`). | Attrs: many gameplay fields (`name`, `className`, `meshFile`, `health`, `velocity`, `renderDist`, `scriptOnDeath`, etc.); children include `<position>`, `<rotate>`, `<scale>`, optional `<sounds>` and physics helpers (`<physPosit>`, `<physSize>`, `<axis>`, `<angle>`) | ```xml\n<npc name="guard_01" className="npc_enemy" meshFile="guard.mesh" health="50" velocity="1.2">\n  <position x="5" y="0" z="8"/>\n  <rotate qw="1" qx="0" qy="0" qz="0"/>\n  <scale x="1" y="1" z="1"/>\n</npc>\n``` |
| `<npcgroup>` | Creates AI element group (`processNPCGroup`) from nested NPC markers. | Attrs: `name`, `type`; child list: `<npc name="..." type="..." className="...">...` | ```xml\n<npcgroup name="grp_patrol" type="0">\n  <npc name="pt1" type="0" className="npc_enemy"><position x="0" y="0" z="0"/></npc>\n  <npc name="pt2" type="0" className="npc_enemy"><position x="6" y="0" z="0"/></npc>\n</npcgroup>\n``` |
| `<button>` | Creates static or physics-movable button (`processButton`). | Attrs: `buttonName`, `meshName`, `movable`, `mass`, `oncont`, `luaScript`; children: `<position>`, `<rotation>`, `<scale>` | ```xml\n<button buttonName="btn_door" meshName="button.mesh" movable="false" luaScript="run3/lua/btn.lua">\n  <position x="1" y="1" z="1"/>\n</button>\n``` |
| `<cutscene>` | Declares a cutscene path/timeline (`processCutScene`). | Attrs: `name`, `length`, `hideHUD`, `freezeb`, `unfreezea`, `music`, `musicFile`, etc.; children: repeated `<frame .../>` | ```xml\n<cutscene name="cs_intro" length="8" hideHUD="true">\n  <frame x="0" y="2" z="-5" qw="1" qx="0" qy="0" qz="0" second="0"/>\n  <frame x="0" y="2" z="0" qw="1" qx="0" qy="0" qz="0" second="8"/>\n</cutscene>\n``` |
| `<seqscript>` | Declares timed sequence script container (`processSeqScript`). | Attrs: `name`, `length`, `inf` | ```xml\n<seqscript name="ss_alarm" length="5" inf="false"/>\n``` |
| `<train>` | Creates train with key points and optional attached details (`processTrain`). | Attrs: `name`, `speed`, `simple`, `start`, sound attrs; children: `<entity>`, `<position>`, `<rotation>`, `<scale>`, repeated `<keyPoint>`, optional `<object>`, `<nocollide>`, `<psys>` | ```xml\n<train name="tram_1" speed="25" simple="true" start="true">\n  <entity meshFile="tram.mesh"/>\n  <position x="0" y="0" z="0"/>\n  <rotation qw="1" qx="0" qy="0" qz="0"/>\n  <scale x="1" y="1" z="1"/>\n  <keyPoint x="0" y="0" z="0"/>\n  <keyPoint x="100" y="0" z="0"/>\n</train>\n``` |
| `<pendulum>` | Creates pendulum-style rotating actor (`processPendulum`). | Attrs similar to rotating entity: `name`, `mesh`, `rotating`, `pitch/yaw/roll/rotspeed`; optional child `<psys>` | ```xml\n<pendulum name="pend_01" mesh="weight.mesh" rotating="true" rotspeed="1.5" pitch="0" yaw="0" roll="35">\n  <position x="0" y="4" z="0"/>\n</pendulum>\n``` |
| `<fuzzy>` | Creates fuzzy object controller (`processFuzzyObject`), supports type `test` and `test2`. | Attrs: `fuzzyName`, `type`; children: `<position>`, `<rotation>`, `<scale>`, optional repeated `<object>` for `test2` | ```xml\n<fuzzy fuzzyName="fz_arm" type="test">\n  <position x="0" y="0" z="0"/>\n</fuzzy>\n``` |
| `<door>` | Creates standard door (`processDoor`). | Attrs: `name`, `mesh`, `distance`, `speed`, direction (`dirX/Y/Z`), `useInteract`, open/close sounds, script hooks (`lOnOpen`, `lOnClosed`, `lOnStarted`, `lLocked`) | ```xml\n<door name="door_main" mesh="door.mesh" distance="120" speed="15" dirX="1" dirY="0" dirZ="0" useInteract="true"/>\n``` |
| `<rot>` | Creates rotator variant via `func_door` (`processRot`). | Similar to `<door>`; intended as rotating moving object entity. | ```xml\n<rot name="rotor_01" mesh="fan.mesh" rotating="true" pitch="0" yaw="0" roll="90" rotspeed="2"/>\n``` |
| `<lua>` | Runs Lua script immediately when sequence loads (`processLuaScript`). | Attr: `script` | ```xml\n<lua script="run3/lua/init_level.lua"/>\n``` |
| `<onexit>` | Registers Lua script to run on map exit (`processLuaScriptOnExit`). | Attr: `script` | ```xml\n<onexit script="run3/lua/on_exit.lua"/>\n``` |

---

## `<events>` entities

These tags attach runtime behavior to already-declared objects.

| Tag | What it does | Main attributes / children | Minimal XML example |
|---|---|---|---|
| `<trigger>` | Binds callbacks to an existing trigger by name (`processTriggerEvent`). | Attrs: `name`, `sec`; children: `<entc>`, `<player>`, `<lua>`, `<hurt>`, `<changelevel>`, `<door>` | ```xml\n<trigger name="t_door" sec="0.1">\n  <lua script="run3/lua/triggered.lua"/>\n  <door name="door_main" event="open"/>\n</trigger>\n``` |
| `<cutscene>` | Starts/configures existing cutscene (`processCutSceneEvent`). | Attrs: `name`, `wait`; children: repeated `<run script="..." sec="..."/>` | ```xml\n<cutscene name="cs_intro" wait="0">\n  <run script="run3/lua/cs_step.lua" sec="1.5"/>\n</cutscene>\n``` |
| `<seqscript>` | Starts/configures existing seqscript (`processSeqScriptEvent`). | Attrs: `name`, `wait`; children: repeated `<run script="..." sec="..."/>` | ```xml\n<seqscript name="ss_alarm" wait="0">\n  <run script="run3/lua/alarm_loop.lua" sec="0.5"/>\n</seqscript>\n``` |

---

## Notes

- Most tags tolerate missing fields by falling back to defaults in `getAttrib*` calls.
- Transform input styles are mixed in this file: some entities read transform from direct attributes on the tag (`x/y/z`, quaternion, `sX/sY/sZ`), while others require nested `<position>`, `<rotation>`/`<rotate>`, and `<scale>` children.
- `<event>` (in `<adents>`) defines an event object, while `<events>` section configures runtime trigger/cutscene/seqscript callbacks.
