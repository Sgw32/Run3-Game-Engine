# Step 5 content validation and conversion boundary

`run3_asset_check` validates content without modifying it. The installed
program uses Ogre classic 14.5.2, Lua 5.4.8, TinyXML2 11.0.0, and the versioned
manifest in `content/manifests/the-long-way-v1.json`. That manifest selects
`resources_high_high.cfg` as the active Ogre parsing profile while structural
checks cover both `run3/` and `media/` and all listed resource configurations.
The local game content remains untracked and is never installed or copied.

## Checks and output

The validator performs these checks:

- every manifest scan root and `resource.cfg` FileSystem/Zip path exists with
  exact component casing;
- each scanned file receives a size and SHA-256 value;
- physical case-colliding paths and duplicate logical resource names within a
  configuration/group are reported;
- filename-like references in XML attributes, Lua strings, and Ogre scripts
  resolve exactly as Linux would see them;
- XML is parsed through the Run3/TinyXML2 schema adapter and Lua is loaded, but
  not executed, by `ScriptEngine` on exact Lua 5.4.8 (including the single
  documented Lua 5.0 `\s` compatibility rule);
- mesh and skeleton serializer headers are checked, then every binary is
  imported through the Ogre 14.5.2 serializers;
- active-profile `.program`, `.material`, and `.compositor` resources are
  parsed through Ogre in dependency order;
- the installed `step5.scene` fixture is loaded through a deliberately small
  render-only loader into Ogre's standard scene manager. It creates a light
  and built-in cube and has no physics, gameplay, or external asset dependency.

The console ends with a short category summary. `--report PATH` selects the
machine-readable JSON report; otherwise it is written to
`<user-root>/logs/asset-report.json`. Exit code `0` means every check passed,
`2` means validation completed and found content errors, and `1` means startup
or validation could not complete. A failing full legacy-content baseline is
therefore expected until its recorded compatibility backlog is addressed.

The fixture used by CTest is distributable and self-contained. Run only the
asset tests with:

```text
ctest --preset <preset> -R "^run3_assets\\.|^run3_asset_check\\."
```

## Validate The Long Way on Windows

From an x64 Developer PowerShell, after building Release as described in
`docs/BUILDING.md`:

```powershell
$installRoot = Join-Path $PWD 'build\install\windows-release'
$userRoot = Join-Path $PWD 'build\user\the-long-way-check'
$contentRoot = 'C:\Run3-Game-Engine\Games\The Long Way\TheLongWay'
$manifest = Join-Path $PWD 'content\manifests\the-long-way-v1.json'
$report = Join-Path $PWD 'build\reports\the-long-way.json'

cmake --install build/windows-msvc-x64-release `
  --prefix $installRoot --config Release
& (Join-Path $installRoot 'bin\run3_asset_check.exe') `
  --renderer d3d11 --frames 1 --user-dir $userRoot `
  --content-root $contentRoot --manifest $manifest --report $report
```

The same validation path is available through the shell by adding
`--validate-content` to `run3_shell` and supplying the same paths.

## Validate The Long Way on Linux

Use native Linux content paths. This WSL example deliberately launches outside
the repository to demonstrate working-directory independence:

```bash
install_root="$PWD/build/install/linux-release"
user_root="$PWD/build/user/the-long-way-check"
content_root="/mnt/c/Run3-Game-Engine/Games/The Long Way/TheLongWay"
manifest="$PWD/content/manifests/the-long-way-v1.json"
report="$PWD/build/reports/the-long-way-linux.json"

cmake --install build/linux-ninja-release --prefix "$install_root" --config Release
cd /tmp
"$install_root/bin/run3_asset_check" \
  --renderer gl3plus --frames 1 --user-dir "$user_root" \
  --content-root "$content_root" --manifest "$manifest" --report "$report"
```

A graphical X11/Wayland session is currently required because Ogre serializer
and script checks share the installed renderer application boundary.

## Non-destructive conversion wrapper

The vcpkg manifest enables Ogre's official `tools` feature, so both utilities
come from the pinned Ogre 14.5.2 package. Do not run either utility directly on
a source asset without a destination: both upstream tools can otherwise infer
or overwrite a path. Use `cmake/ConvertOgreAsset.cmake` for one reviewed file at
a time.

Windows mesh upgrade example:

```powershell
$converter = Join-Path $PWD 'build\windows-msvc-x64-release\vcpkg_installed\x64-windows\tools\ogre\OgreMeshUpgrader.exe'
$inputAsset = 'C:\path\to\reviewed.mesh'
cmake "-DRUN3_CONVERTER=$converter" `
  -DRUN3_CONVERTER_MODE=mesh-upgrader `
  "-DRUN3_INPUT=$inputAsset" `
  "-DRUN3_OUTPUT_ROOT=$PWD\converted-content" `
  -P cmake/ConvertOgreAsset.cmake
```

Linux XML conversion example:

```bash
converter="$PWD/build/linux-ninja-release/vcpkg_installed/run3-x64-linux/tools/ogre/OgreXMLConverter"
cmake -DRUN3_CONVERTER="$converter" \
  -DRUN3_CONVERTER_MODE=xml-converter \
  -DRUN3_INPUT="/path/to/reviewed.mesh" \
  -DRUN3_OUTPUT_ROOT="$PWD/converted-content" \
  -P cmake/ConvertOgreAsset.cmake
```

Outputs are keyed by mode, input SHA-256, converter SHA-256, and invocation
SHA-256. Each leaf contains the converted file, converter log, and deterministic
`conversion.json` receipt. A repeated identical command verifies and reuses the
existing output. The wrapper always supplies a distinct destination, verifies
the source hash after execution, and refuses to overwrite incomplete or changed
outputs. `converted-content/` is ignored by Git. Review generated output before
changing any runtime reference; no mass-conversion command is provided.
