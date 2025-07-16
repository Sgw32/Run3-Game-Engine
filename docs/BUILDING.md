# Building Run3

This document describes how to build the Run3 Game Engine from source.

1. Ensure you have a C++ compiler and the required dependencies installed.
2. Generate the project files or makefiles using your preferred build system.
3. Compile the source code and link the engine binaries.

More detailed instructions will be provided in the future.

## Draft CMake Build (Ogre3D 2)

Run3 can be built using CMake. The provided `CMakeLists.txt` is a starting
point that assumes Ogre3D 2 is installed and available via CMake's
`find_package` command.

```
mkdir build
cd build
cmake ..
cmake --build .
```

This configuration is a work in progress and only covers the basic engine
sources. You may need to add additional include directories or libraries based
on your setup.
