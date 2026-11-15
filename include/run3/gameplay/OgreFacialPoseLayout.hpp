#pragma once

#include <OgreAnimation.h>
#include <OgreAnimationTrack.h>
#include <OgreKeyFrame.h>
#include <OgreMesh.h>
#include <OgrePose.h>

#include <set>

namespace run3::gameplay {

// Ogre allocates per-target blend buffers when an Entity is constructed.
// Declaring a new pose target later only refreshes animation states, leaving
// those buffers null on meshes whose authored animations omit that target.
inline void prepareFacialPoseLayout(Ogre::Mesh &mesh) {
  constexpr const char *name = "Run3/Facial/Layout";
  if (mesh.getPoseList().empty() || mesh.hasAnimation(name)) return;
  auto *animation = mesh.createAnimation(name, 1.0F);
  std::set<unsigned short> targets;
  for (const auto *pose : mesh.getPoseList()) targets.insert(pose->getTarget());
  for (const auto target : targets) {
    // A disabled, empty track declares the layout without changing the mesh.
    animation->createVertexTrack(target, Ogre::VAT_POSE)
        ->createVertexPoseKeyFrame(0.0F);
  }
}

} // namespace run3::gameplay
