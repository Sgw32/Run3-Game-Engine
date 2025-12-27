# Building Run3 on Windows

This guide outlines the basic steps to compile the engine using **Visual Studio**.

## Prerequisites

Before opening the project you should install the following SDKs or libraries and set up their environment variables where appropriate:

- **Ogre3D SDK** (`OGRE_HOME`)
- **OIS** (`OIS_HOME`)
- **CEGUI** (`CEGUI_HOME`)
- **OpenAL** (`OPENAL_SDK`)
- **OgreNewt** (`OGRENEWT_HOME`)

Other dependencies (Lua, Newton, etc.) may be required depending on the configuration of `Run3.vcproj`.

## Loading the Solution

1. Open `Run3.sln` in Visual Studio. Older versions of Visual Studio may instead load `Run3.vcproj` directly.
2. Ensure the include and library directories for the above dependencies are reachable. This is typically handled via the environment variables shown above.

## Building

Select either the *Debug* or *Release* configuration and choose **Build Solution**. Visual Studio will compile the `run3` executable and accompanying DLLs inside the configured output directory.

After a successful build you can run the engine from the build folder.
