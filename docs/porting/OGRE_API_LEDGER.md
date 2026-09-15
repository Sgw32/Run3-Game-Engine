# Ogre 14 API migration ledger

This ledger records only reviewed mechanical compatibility changes made while
building `run3_legacy`. It is not permission to change gameplay semantics.

| Legacy API or assumption | Ogre 14.5.2 replacement | Affected files | Validation |
|---|---|---|---|
| `Ogre::Singleton<T>::ms_Singleton` static member | `Ogre::Singleton<T>::msSingleton` initialized with `nullptr` | `Run3Batcher.cpp`, `EventEntC.cpp`, `SceneLoadOverlay.cpp` | `run3_legacy` and compile smoke link in MSVC/GCC Debug and Release. |
| `Camera::getDirection()` on the lens-flare camera | `Camera::getDerivedDirection()` to preserve the original world-space calculation | `LensFlare.cpp` | Compiles in MSVC/GCC Debug and Release; no scene/runtime behavior test yet. |
| `OgreMeshManager.h` transitively supplied the complete `Mesh` declaration | Include `OgreMesh.h` explicitly | `GeomUtils.cpp` | Sphere/quad helper compiles in both compilers and configurations. |
| `OgreSimpleRenderable.h` transitively supplied render-queue IDs | Include `OgreRenderQueue.h` explicitly | `AmbientLight.cpp`, `MLight.cpp` | Deferred-render helper batch compiles in both compilers and configurations. |
| `Ogre.h`/old GUI headers exposed Overlay classes and `OverlayManager` | Link imported target `OgreOverlay`; include `OgreOverlay.h`, `OgreOverlayContainer.h`, and `OgreOverlayManager.h` explicitly | `SceneLoadOverlay.h`, root `CMakeLists.txt` | Overlay unit and archive link in both compilers and configurations. |
| Removed OIS/OgreConsole headers incidentally imported Ogre/std names | Remove unused retired headers and qualify `Ogre`/`std` types | `EventEntC.h/.cpp`, `SceneLoadOverlay.h/.cpp` | Both units compile without OIS on Windows and Linux. |
| Ogre sample-framework `Root::startRendering()` owned the process loop | `Run3App` explicitly pumps OgreBites events, advances `EngineClock`, dispatches Run3 input, calls `Root::renderOneFrame`, and closes in one exception-safe owner | `source/app/Run3App.cpp`, `source/shell_main.cpp`, `Run3FrameListener.h` | Installed five-frame smoke passes with D3D11 and GL3+ in MSVC/GCC Debug and Release. |
| `Ogre::UTFString` and transitive Overlay declarations | UTF-8 `Ogre::String` plus explicit `OgreOverlay.h`/`OgreOverlayManager.h` includes | `buttonGUI.h/.cpp` | `buttonGUI.cpp` compiles in the 26-unit legacy target on both compilers. |
| `Root::getSceneManagerIterator()` | `Root::getSceneManagers()` with an explicit empty check | `ogreconsole.cpp` | Console unit compiles on both compilers; no content-dependent visual test in Step 4. |
| `SimpleRenderable::setMaterial(String)` | Resolve the `MaterialPtr` with `MaterialManager::getByName` before assignment | `ogreconsole.cpp` | Console unit compiles on both compilers; runtime material validation remains a Step 9 task. |
| Legacy `DotSceneLoader` coupled scene creation to `OgreNewt::TreeCollision`/`PhysObject` | `run3::gameplay::StaticMap` creates Ogre entities, extracts indexed mesh buffers, and passes Run3-owned triangle data to `PhysicsWorld` | `source/gameplay/StaticMap.cpp`, `include/run3/gameplay/StaticMap.hpp` | Fixture collision tests pass on MSVC/GCC; `tlwcao` and `tlwhome02` load with zero skipped sections on D3D11. |
| Old mesh extraction assumed local transforms and Newton handled node scale | Read `VertexData`/`IndexData`, honor `vertexStart`/`indexStart`, apply derived nonuniform scale to vertices, preserve source winding (swap on negative scale determinant), and put derived position/orientation on the body | `source/gameplay/StaticMap.cpp` | 30/60/144 real-map runs create stable section/triangle counts and identical final transforms. |
| Fixed-function map materials were accepted by D3D9 | Read legacy material aliases without executing old shaders; use `MeshSerializerListener` to retain original submesh assignments before missing-material substitution; generate simple Ogre 14 RTSS materials retaining diffuse textures, transparency, culling, and normal depth writes; do not treat an opaque multipass material's additive lighting pass as whole-material transparency; keep assets untouched | `source/gameplay/LegacyMaterialCatalog.cpp`, `source/gameplay/StaticMap.cpp` | Catalog fixtures distinguish opaque additive lighting from alpha/depth-write-off materials on MSVC/GCC. D3D11 generates 205 textured materials for `tlwcao` and 375 for `tlwhome02`; GL3+ independently loads the 205-material `tlwcao` set. Bounded runs shut down cleanly. Advanced shader parity remains Step 9. |
| Ogre nodes were attached directly to raw OgreNewt bodies and updated by a Newton transform callback | Dynamic `<phys>`/`<breakable>` sections create Run3 bodies from scaled mesh bounds; rendering pulls interpolated transforms after fixed stepping and converts derived transforms through the node parent | `source/gameplay/StaticMap.cpp`, `source/app/Run3App.cpp` | MSVC/GCC fixtures compile; installed D3D11 `tlwcao` loaded 368 visuals/166 collision sections, skipped zero, rendered, and shut down cleanly. |

Additional non-Ogre portability fixes in the same compile batches were limited
to the case-correct `Tokenizer.h` include, an explicit `<iostream>` include,
and a fixed underlying type for the legal forward declaration of
`CaduneTree::ShapeEnum`. No files were moved or mass-formatted.

## Deferred Ogre API families

The following are not marked migrated because their owning translation units
are still outside the compiled subset:

- the historical, unbuilt gameplay entrypoint in `main.cpp` and
  `Run3Application.h`; the supported executable now uses `run3::Run3App`;
- custom scene-manager/PCZ/octree factory ownership;
- compositor chains and render-target APIs in `DeferredShading.cpp`,
  `Run3Shadowing.cpp`, `StereoManager.cpp`, and post-processing managers;
- remaining shared-pointer casts, resource lookups, iterators, statistics,
  mesh-buffer access, and shadow APIs in dependency-coupled gameplay sources.

Each future row must name its validation. Compilation alone is sufficient only
for an API-mechanical batch; resource or renderer behavior requires a focused
runtime test and must not be hidden by a catch-all exception handler.
