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

Additional non-Ogre portability fixes in the same compile batches were limited
to the case-correct `Tokenizer.h` include, an explicit `<iostream>` include,
and a fixed underlying type for the legal forward declaration of
`CaduneTree::ShapeEnum`. No files were moved or mass-formatted.

## Deferred Ogre API families

The following are not marked migrated because their owning translation units
are still outside the compiled subset:

- legacy sample-framework startup in `main.cpp`, `Run3Application.h`, and
  `Run3FrameListener.h`;
- custom scene-manager/PCZ/octree factory ownership;
- compositor chains and render-target APIs in `DeferredShading.cpp`,
  `Run3Shadowing.cpp`, `StereoManager.cpp`, and post-processing managers;
- remaining shared-pointer casts, resource lookups, iterators, statistics,
  mesh-buffer access, and shadow APIs in dependency-coupled gameplay sources.

Each future row must name its validation. Compilation alone is sufficient only
for an API-mechanical batch; resource or renderer behavior requires a focused
runtime test and must not be hidden by a catch-all exception handler.
