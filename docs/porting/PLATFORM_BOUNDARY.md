# Step 4 platform boundary

The supported executable is `run3_shell`. Its thin `main` constructs
`run3::Run3App`; the application owns Ogre initialization, event polling,
clock advancement, input dispatch, `renderOneFrame`, and orderly shutdown.
The historical `main.cpp`/`Run3Application.h` entrypoint remains excluded from
the target and is not a second runtime path.

## Paths and configuration

`AppPaths` anchors all relative paths at the executable directory. It exposes
one read-only `contentRoot` and separate writable `config`, `saves`, `logs`,
and `cache` directories below `userRoot`. Callers cannot use `contentPath` or
`userPath` to escape their owning root with `..`.

Configuration layers are applied in this order, from lowest to highest:

1. `<content-root>/config/run3.cfg` content defaults
2. `<user-root>/config/run3.cfg` user settings
3. command-line arguments

Files use `key=value` lines with `#` or `;` comments. Step 4 recognizes
`renderer`, `frames`, `content-root`, and `user-root`. The CLI equivalents are
`--renderer`, `--frames`, `--content-root`, and `--user-dir`.

## Input ownership

`Key`, `MouseButton`, `InputEvent`, `InputState`, and `IInput` are Run3-owned
types with no Ogre, SDL, OIS, or operating-system declarations. The only
OgreBites-to-Run3 translation lives in `OgreBitesInputAdapter`; gameplay and UI
receive the translated event once. `InputState` clears held keys/buttons on
focus loss and records resize, mouse delta, wheel, and quit state.

`NullInput` supports headless execution. `ReplayInput` consumes deterministic
per-frame event batches for tests. `EventQueueInput` is the live queue used by
`Run3App`. The transitional `InputManager2` now fans out `IInput` events and no
longer owns or captures platform devices.

The following legacy-facing batch no longer exposes OIS types: `Run3Input`,
`Player`, `OgreConsole`, `HUD`, `buttonGUI`, `Display`, `MagicManager`, and the
weapon input chain. `buttonGUI.cpp`, `ogreconsole.cpp`, and
`InputManager2.cpp` compile in `run3_legacy`; consumers still blocked by
Newton, CEGUI, Lua, or other later-step dependencies remain source-migrated but
are not falsely added to the target.

## Optional devices and diagnostics

`RUN3_ENABLE_OPTIONAL_DEVICES=OFF` selects logging no-op serial and named-pipe
objects on every host. `ON` selects Win32 implementations only on Windows;
non-Windows builds continue to use the no-op backend. No Windows type appears
in a public header. Legacy `CSerial` and `NamedPipeServer` are portable facades
over these interfaces.

Fatal startup failures and retired console presentation now use logging. There
are no `MessageBox`, console-colour API calls, OIS declarations, or Windows
headers in the root project's public headers. The separately versioned AIR3
submodule was not modified in this step.
