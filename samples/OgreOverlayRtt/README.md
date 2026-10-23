# Ogre native-overlay RTT sample

This executable is intentionally independent of the Run3 engine. It creates a
plain Ogre scene containing `amiga1000.mesh`, renders an `Ogre::Overlay` and
`Ogre::TextAreaOverlayElement` into a 1024x768 render texture, and assigns that
texture to the mesh subentity authored as `AMIGA1000AmigaMon`.

The main window viewport has Ogre overlays disabled. Only the RTT viewport has
them enabled, so any visible UI must have passed through the render texture.

Build, install beside the Ogre runtime, and run on Windows:

```bat
cmake --build --preset windows-msvc-x64-release --target run3_ogre_overlay_rtt_demo
cmake --install build/windows-msvc-x64-release --prefix build/install/windows-release
build\install\windows-release\bin\run3_ogre_overlay_rtt_demo.exe
```

The first optional argument is the game content root. The second is the output
PNG path. By default the sample writes `build/repro/ogre-overlay-rtt-demo.png`
and a second `-rtt.png` image containing the raw native-overlay render target.

The sample materials use small explicit D3D11 shaders. RTSS is not involved in
the model, overlay, text, or RTT presentation path.
