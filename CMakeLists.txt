cmake_minimum_required(VERSION 3.10)
project(Run3GameEngine)

# This is a draft CMake build configuration for Run3

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Ogre3D 2 package
find_package(OGRE 2 REQUIRED CONFIG)

# Add source files (this list is incomplete and provided as a starting point)
file(GLOB_RECURSE RUN3_SOURCES
    *.cpp
    *.h
)

add_executable(run3 ${RUN3_SOURCES})

# Link against Ogre libraries
target_link_libraries(run3
    OgreMain
    OgreOverlay
    OgreBites
)

# Additional include directories can be added as needed
# include_directories(${OGRE_INCLUDE_DIRS})
