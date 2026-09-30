# Step 9A UI runtime

## Reproducible backend boundary

Run3 builds MyGUI from the `mygui` gitlink at
`8629ea76896fba2d837cffde9fce9f32935a1d11`. Both the root build and
`Demos/MyGUIOgre` reject a different checkout. CMake builds only the MyGUI
engine and Ogre-classic platform targets and links them through
`MyGUI::MyGUI` and `MyGUI::OgrePlatform`. Gameplay-facing headers contain only
Run3 `IUiSystem`, `IScriptUiFacade`, context, event, and generational handle
types; MyGUI ownership stays in `source/ui/MyGuiUiSystem.cpp`.

There are three independently scoped contexts:

| Context | Owner | Contents | Lifetime |
|---|---|---|---|
| `main` | application window | Continue, New game, Chapters, Options, Quit | application |
| `hud` | main viewport | console, loading, inventory | application; state resets per map |
| `computer` | one typed map entity handle | legacy `buttonGUI` and optional typed MyGUI widgets | active computer only |

The main menu uses a dark charcoal/red/black theme with orange text and
accents. Root panels in the HUD and computer contexts are transparent; an
inactive context therefore cannot paint a full-screen rectangle. Gameplay
crosshair and `gameText`/subtitle presentation remain Ogre Overlays for legacy
layout compatibility and do not keep MyGUI rendering active during ordinary
first-person play.

Widget handles encode context, monotonically assigned id, and context
generation. Re-entering a computer invalidates every old widget and callback.
Duplicate names, cross-context parents, stale handles, unapproved layout keys,
unsafe properties, and callbacks that exceed the Lua instruction budget fail
with contextual errors.

## Input and virtual computers

SDL/OgreBites input is translated into Run3 `InputEvent` once. The UI adapter
translates that event to MyGUI only while the menu, console, or a computer owns
focus. Gameplay mouse capture is disabled for those states and restored after
exit. Focus loss clears MyGUI keyboard/mouse focus; resize recomputes the
logical 1280x720 layout.

SDL's native pointer is hidden for the whole application lifetime. MyGUI's
pointer is shown only while the main menu owns input, or temporarily inside a
computer's off-screen render pass. Loading a map always closes the menu and a
normal gameplay frame has neither an OS cursor nor a MyGUI cursor. This avoids
the former double-cursor state and keeps relative mouse look unbounded.

Entering a computer permits only one map-scoped owner, creates a 1024x768
MyGUI render texture, and substitutes a generated material on the matching
computer screen submesh. Each off-screen pass temporarily hides the main menu
and HUD roots, shows only the computer root, renders the texture, and restores
all previous visibility in an RAII guard. Exit, script failure propagation,
map change, and unload invalidate callbacks, restore original submesh
materials, remove the generated material, clear MyGUI focus, and release the
computer context. The existing Step 8E camera/player freeze owner remains the
authority for camera and movement restoration.

The computer context is deliberately empty when entered. Existing
`buttonGUI_*` content is the primary authoring API and creates widgets in that
isolated context; the typed `mygui` facade can be used beside it. The engine no
longer creates a generic full-screen "computer connected" page, and no
computer widget is rendered directly into the main viewport.

## Lua facade

The versioned snapshot is `tests/golden/mygui-exported-api.txt`. The exposed
namespace is intentionally smaller than native MyGUI:

| Function | Purpose |
|---|---|
| `mygui.load_layout(scope, key)` | obtain an approved context root (`default`, `main`, `hud`, or `computer` as applicable) |
| `mygui.create(...)` | create a named panel, text, button, edit, checkbox, or list child |
| `mygui.find(scope, name)` | find a widget only in the named scope |
| `mygui.destroy(handle)` | destroy the widget subtree and its callbacks |
| `mygui.set_text`, `set_visible`, `set_enabled` | safe common widget state |
| `mygui.set_property` | set one allow-listed property; arbitrary renderer/resource access is rejected |
| `mygui.focus` | give a live widget keyboard focus |
| `mygui.on`, `clear_callback` | bounded click/change/submit callbacks |

Lua receives opaque string handles, never a `MyGUI::Widget*`. Paths cannot be
loaded through this API. The retained `buttonGUI_*` globals create the same
scoped widgets and are the primary legacy-computer compatibility facade, not a
second renderer.

## Verification

The focused test command is:

```text
cmake --build --preset <preset> --target run3_shell run3_step9a_tests
ctest --preset <preset> -R run3_step9a --output-on-failure
```

It covers handle generations, duplicate names, callback cleanup, path/layout
confinement, callback instruction budgets, and typed API snapshots.
`buttonGUI` now owns a separate facade, root widget tree, callback registry,
and teardown path from the typed `mygui` namespace. Both independent layers
are composited into the computer target so legacy scripts remain visible
without leaking button handles into MyGUI's typed API. The tests also cover
16:9, 16:10, 4:3, and high-DPI layout math. Installed shell
smokes exercise D3D11 and GL3+ on Windows and GL3+ on Linux. The standalone
rotating-cube demo and exact commands are in
`Demos/MyGUIOgre/README.md`.

For a manual computer check, load a map containing a `<computer>`, press `E`
while aiming at its use surface, interact with its MyGUI/buttonGUI controls,
and press Escape to disconnect. Confirm that the OS pointer/focus follows the
computer, the main HUD is absent from the computer texture, and the original
screen material and first-person capture return after exit and after a map
transition.
