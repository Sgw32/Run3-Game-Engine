# Run3 Engine Overview

Run3 is an experimental open-source game engine built around the **Ogre3D** rendering library. It aims to provide Source-engine style gameplay features while remaining fully moddable. The code base is written in C++ and integrates several middleware libraries for graphics, physics and sound.

## Key Dependencies

- **Ogre3D** – primary rendering engine
- **OIS** – input handling
- **CEGUI** – graphical user interface system
- **OpenAL** – audio playback
- **OgreNewt** – physics and collision

## Architecture

At a high level the engine is organized into a few major modules:

- **Rendering** – custom scene managers and deferred shading built on top of Ogre3D.
- **Physics** – simulation using OgreNewt, with helpers for ragdolls and rigid bodies.
- **Sound** – OpenAL based sound manager for in‑game audio.
- **Input & UI** – OIS for keyboard/mouse and CEGUI for menus and HUDs.
- **Game Systems** – gameplay logic, AI, scripting and other utilities.

This documentation section will grow with additional guides and references.
