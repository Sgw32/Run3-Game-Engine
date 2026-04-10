# DotScene XML Entities Reference

This section documents the DotScene XML blocks consumed by `DotSceneLoader.cpp` before `Sequence` processing.

---

## High-level structure

```xml
<scene formatVersion="..." multiplier="1" glass="false" glow="true" tLod="-1" mLod="-1">
  <integratedSequence>...</integratedSequence>
  <nodes>...</nodes>
  <aiNodes>...</aiNodes>
  <environment>...</environment>
  <sounds>...</sounds>
  <zone ...>...</zone>
</scene>
```

`DotSceneLoader::parseDotScene` validates `<scene>`, applies scene-level attributes, then dispatches into specialized `process*` methods (`processNodes`, `processEnvironment`, `processSounds`, etc.).

---

## Scene-level blocks

| Tag | What it does | Main attributes / children | Minimal XML example |
|---|---|---|---|
| `<scene>` | Root DotScene entry. Applies level multipliers and visual toggles before dispatching sub-blocks. | Attrs: `multiplier`, `glass`, `glow`, `tLod`, `mLod`; children: `<integratedSequence>`, `<nodes>`, `<aiNodes>`, `<environment>`, `<sounds>`, `<zone>`, `<light>`, `<camera>` | <pre><code>&lt;scene multiplier="1" glass="false" glow="true"&gt;
  &lt;nodes&gt;...&lt;/nodes&gt;
  &lt;environment&gt;...&lt;/environment&gt;
&lt;/scene&gt;</code></pre> |
| `<integratedSequence>` | Hands embedded sequence XML to `Sequence::SetSceneSeq`. | Child content: Sequence XML payload | <pre><code>&lt;integratedSequence&gt;
  &lt;sequence&gt;...&lt;/sequence&gt;
&lt;/integratedSequence&gt;</code></pre> |
| `<nodes>` | Container for world scene graph nodes. | Children: repeated `<node>` | <pre><code>&lt;nodes&gt;
  &lt;node name="room_A"&gt;...&lt;/node&gt;
&lt;/nodes&gt;</code></pre> |
| `<aiNodes>` | AI waypoint/group definitions. | Children: repeated `<npcnode>` with transform attrs | <pre><code>&lt;aiNodes&gt;
  &lt;npcnode name="patrol_01" x="0" y="0" z="0"/&gt;
&lt;/aiNodes&gt;</code></pre> |
| `<environment>` | Global rendering/physics/player setup. | Children: `<fog>`, `<skyBox>`, `<newtonWorld>`, `<player>`, `<hud>`, `<water>`, `<skyx>`, `<skyDome>`, `<skyPlane>`, `<clipping>`, `<fade>`, `<colourAmbient>`, `<farClip>` | <pre><code>&lt;environment&gt;
  &lt;fog mode="exp2" expDensity="0.001"/&gt;
  &lt;player x="0" y="2" z="0"/&gt;
&lt;/environment&gt;</code></pre> |
| `<sounds>` | Audio definitions loaded during map load. | Children: repeated `<sound>`, repeated `<ambient>`, optional `<music>` | <pre><code>&lt;sounds&gt;
  &lt;sound name="hum.wav" x="0" y="1" z="0" loop="true"/&gt;
  &lt;music name="chapter1.ogg" loop="true"/&gt;
&lt;/sounds&gt;</code></pre> |
| `<zone>` | Registers zone volume and optional embedded entities. | Attrs: `x`,`y`,`z`,`sX`,`sY`,`sZ`; children: repeated `<entity>` | <pre><code>&lt;zone x="0" y="0" z="0" sX="50" sY="20" sZ="50"&gt;
  &lt;entity name="zone_prop" meshFile="crate.mesh"/&gt;
&lt;/zone&gt;</code></pre> |

---

## `<node>` content entities

`processNode` reads transform helpers (`<position>`, `<rotation>`, `<scale>`) and then parses nested content.

| Tag | What it does | Main attributes / children | Minimal XML example |
|---|---|---|---|
| `<entity>` | Static render entity; supports batching/material overrides and optional collision ignore tuning. | Attrs: `name`, `meshFile`, `materialFile`, `run3batcher`, `castShadows`, etc. | <pre><code>&lt;entity name="wall_01" meshFile="wall.mesh" run3batcher="true" castShadows="false"/&gt;</code></pre> |
| `<phys>` | Dynamic physics entity. | Attrs: `name`, `meshFile`, `mass`, etc. | <pre><code>&lt;phys name="barrel" meshFile="barrel.mesh" mass="40"/&gt;</code></pre> |
| `<blockbox>` | Static collision entity using mesh bounds. | Attrs: `name`, `meshFile`, etc. | <pre><code>&lt;blockbox name="collider_A" meshFile="box.mesh"/&gt;</code></pre> |
| `<pblock>` | Transparent static blocking volume. | Attrs: `name` | <pre><code>&lt;pblock name="blocker_01"/&gt;</code></pre> |
| `<nocollide>` | Registers mesh/object pairs for collision exclusion. | Children include buffers + object refs | <pre><code>&lt;nocollide object1="door_01" object2="frame_01"/&gt;</code></pre> |
| `<light>` | Light with attenuation/range/dynamics. | Attrs + children nodes | <pre><code>&lt;light name="lamp_01" type="point" power="1.2"&gt;
  &lt;colourDiffuse r="1" g="0.9" b="0.8"/&gt;
&lt;/light&gt;</code></pre> |
| `<particleSystem>` | Creates Ogre particle system. | Attrs: `name`, `file` | <pre><code>&lt;particleSystem name="steam_01" file="Examples/Smoke"/&gt;</code></pre> |
| `<fire>` | Fire system helper. | Attrs: `pSys`, `renderDist` | <pre><code>&lt;fire pSys="Examples/Fire" renderDist="3000"/&gt;</code></pre> |
| `<ragdoll>` | Spawns ragdoll. | Attrs: `name`, `meshFile`, `scriptFile` | <pre><code>&lt;ragdoll name="corpse_01" meshFile="enemy.mesh"/&gt;</code></pre> |
| `<tree>` | Procedural tree. | Attrs: `name`, `treeFile` | <pre><code>&lt;tree name="tree01" treeFile="cadune01.mtd"/&gt;</code></pre> |
| `<mirror>` | Adds mirror surface. | Attr: `metalTexture` | <pre><code>&lt;mirror metalTexture="mirror_metal"/&gt;</code></pre> |
| `<breakable>` | Breakable physics object. | Many attrs | <pre><code>&lt;breakable name="crate_break" meshFile="crate.mesh" health="30"/&gt;</code></pre> |

---

## Environment and audio entries

| Tag | What it does | Main attributes / children | Minimal XML example |
|---|---|---|---|
| `<player>` | Player spawn config | Position + look | <pre><code>&lt;player x="0" y="2" z="0"/&gt;</code></pre> |
| `<newtonWorld>` | Physics bounds | Bounds attrs | <pre><code>&lt;newtonWorld x1="-5000" y1="-5000" z1="-5000" x2="5000" y2="5000" z2="5000"/&gt;</code></pre> |
| `<fog>` | Scene fog | Mode + color | <pre><code>&lt;fog mode="exp2" expDensity="0.001"/&gt;</code></pre> |
| `<skyBox>` | Skybox | Material + distance | <pre><code>&lt;skyBox material="Examples/CloudyNoonSkyBox"/&gt;</code></pre> |
| `<skyDome>` | Skydome | Curvature + tiling | <pre><code>&lt;skyDome material="Examples/CloudySky"/&gt;</code></pre> |
| `<skyPlane>` | Skyplane | Plane params | <pre><code>&lt;skyPlane material="Examples/SpaceSkyPlane"/&gt;</code></pre> |
| `<water>` | Water system | File + sun | <pre><code>&lt;water fileName="water.cfg"/&gt;</code></pre> |
| `<hud>` | HUD visibility | `show` | <pre><code>&lt;hud show="false"/&gt;</code></pre> |
| `<fade>` | Fade config | Duration/speed | <pre><code>&lt;fade duration="1.5" speed="1"/&gt;</code></pre> |
| `<clipping>` | Camera clipping | near/far | <pre><code>&lt;clipping near="1" far="20000"/&gt;</code></pre> |
| `<sound>` | Positional sound | Position + loop | <pre><code>&lt;sound name="machinery.wav" x="3" y="2" z="-1"/&gt;</code></pre> |
| `<ambient>` | Ambient sound | Bound to object | <pre><code>&lt;ambient name="wind.wav" objname="wind_zone_1"/&gt;</code></pre> |
| `<music>` | Background music | Name + loop | <pre><code>&lt;music name="chapter1.ogg" loop="true"/&gt;</code></pre> |

---

## Summary

- Avoid triple backticks inside tables
- Use `<pre><code>` blocks inside table cells


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
