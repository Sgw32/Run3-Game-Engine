# Step 8 XML and Lua compatibility

Step 8 replaces the live TinyXML 1 and Lua 5.0 boundaries without changing
The Long Way's file schemas or script-visible global names. TinyXML2, Lua, and
sol2 are private implementation dependencies: engine/game code consumes
`run3::content` and `run3::scripting` types only.

## XML baseline and adapter

The pre-migration semantic snapshots are under `tests/golden/xml/`. Their
inputs under `tests/fixtures/step8/xml/` represent the five live schema
families:

| Fixture | Representative content evidence | Schema rule |
|---|---|---|
| `representative.scene` | `maps/low/tlwcao/tlwcao.xml` and shipped `.scene` files | `<scene>` root, ordered child nodes |
| `representative-sequence.xml` | `maps/low/tlwcao/tlwcaos.xml` | `<sequence>` root; legacy unquoted numeric attributes accepted |
| `representative-save.xml` | `game/savefile.xml` | `<savefile>` root and unchanged attribute strings |
| `representative-facial.xml` | `sounds/pogran/document01_facial.xml` | `<facialanimation>` root, subtitle and timing data preserved |
| `representative-config.xml` | `core/start.xml` | config-adjacent XML accepts its own root |
| `malformed.xml` | deliberately mismatched tags | source, schema, line, column, and parser cause are reported |

`run3::content::parseXmlFile` owns the schema check and copies TinyXML2 data
into Run3 value types. The only compatibility normalization is performed in
memory after a strict parse fails: unquoted legacy attribute values are quoted,
and a missing separator after a quoted value is restored. Source files are
never rewritten. Attribute text, child order, and legacy number strings remain
unchanged. Both the asset checker and the Ogre render fixture now use this
adapter; only `source/content/XmlParser.cpp` includes TinyXML2.

Bundled `tinyxml.cpp`, `tinyxmlerror.cpp`, and `tinyxmlparser.cpp` are retained
as historical evidence but retired from every target. `tinystr.cpp` was already
an incomplete, unused historical file. A CTest boundary rejects TinyXML headers
outside the adapter.

## Lua 5.0 inventory

Regenerate the read-only inventory, binding catalog, and golden API snapshot:

```text
python tools/script_api_inventory.py . \
  --content-root "Games/The Long Way/TheLongWay/run3" \
  --json docs/porting/LUA_API_INVENTORY.json \
  --catalog source/scripting/LegacyBindings.inc \
  --snapshot tests/golden/lua-exported-api.txt
```

The captured baseline is:

- 296 Git-tracked Run3 legacy C/C++ source/header files inspected (AIR3 has no
  Lua binding surface and is outside this inventory);
- 23 distinct Lua 5.0 C API operations across 1,016 call sites, each with its
  source and line as well as per-operation counts;
- 202 unique `lua_register` exports, each recorded with group, exported name,
  `int(lua_State*)` signature, callback, source, and line;
- zero luabind registration/include sites in the tracked tree (the vcproj
  mentions legacy external Lua libraries, but no tracked luabind use exists);
- exactly 955 attached content scripts and their calls to exported globals;
- inventory payload SHA-256
  `8bab86590fd2ad00ffb7c462864500f569689ba5cbd83dcac6d0a961073ae661`.

`source/scripting/LegacyBindings.inc` and
`tests/golden/lua-exported-api.txt` are generated artifacts and must not be
hand-edited. The compatibility test rejects renamed, duplicated, regrouped, or
signature-changed globals.

## ScriptEngine policy

`ScriptEngine` uses manifest-pinned Lua 5.4.8 and sol2 3.5.0 port revision 1.
Bindings are grouped as audio, devices, NPC, physics, player, rendering,
sequence, UI, and world. The compatibility catalog forwards calls to an
injected Run3 dispatcher and records stable value arguments for tests; no
gameplay header includes Lua or sol2.

Security and failure behavior are deliberate:

- C++ may load script files only below the configured read-only content root
  or writable user root; canonical paths outside both are rejected;
- Lua `io`, `os`, `package`, `debug`, `dofile`, and `loadfile` are absent;
- base, coroutine, string, math, table, and UTF-8 libraries remain available;
- protected calls use a retained traceback handler, and an instruction-count
  hook terminates runaway scripts at the configured budget;
- parse, sandbox, binding-dispatch, and runtime failures become contextual
  `ScriptError` exceptions; none is caught and discarded.

The attached content needs one narrowly scoped Lua 5.0 compatibility rule.
`lua/computers/demo_comp5.lua` contains the formerly tolerated unknown string
escape `\s`. Only after Lua 5.4 reports that exact invalid-escape category,
the loader retries an in-memory copy with the escape interpreted as Lua 5.0
did. Originals are not changed. Searches found no shipped use of `setfenv`,
`getfenv`, `loadstring`, global `unpack`, `table.getn`, `math.mod`, `module`,
`newproxy`, or `gcinfo`, so no speculative shims were added.

Three shipped scripts are syntactically broken in both the baseline intent and
Lua 5.4 and are explicitly left unchanged:

- `lua/chapters/aeroxo/timer.lua`;
- `lua/chapters/aeroxo2/timer.lua`;
- `lua/chapters/manip/timer.lua`.

Each has missing closing parentheses near the beginning and is not referenced
by the active exported-call inventory. The full-content test requires exactly
these three failures, exactly the one compatibility-shim use above, and no
others. It also executes representative `tlwcao` startup, NPC/trigger/music,
subtitle, and map-change scripts through the protected compatibility bindings.

There is no vendored Lua 5.0 interpreter or luabind implementation in the
tracked repository to delete. `lua/LuaHelperFunctions.h` and `lua/Scriptor.h`
are unbuilt historical Run3 wrappers referenced by the old vcproj, not the Lua
runtime; they remain behavior evidence beside the other retired sources. The
live target graph neither includes them nor links `lua.lib`/`lualib.lib`.

## Focused verification

After configuring any preset:

```text
cmake --build --preset <preset> --target run3_step8_tests
ctest --preset <preset> -L step8
```

If The Long Way is not attached, content-specific cases skip; fixture,
sandbox, traceback, budget, golden XML, API snapshot, and dependency-boundary
tests still run. With the authorized local content attached, the test requires
all 955 files and does not silently weaken its expected failure list.
