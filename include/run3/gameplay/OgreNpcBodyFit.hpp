#pragma once

#include <OgreEntity.h>
#include <OgreAnimationState.h>
#include <OgreMesh.h>
#include <OgreSkeletonInstance.h>
#include <OgreSubMesh.h>
#include <OgreVertexIndexData.h>
#include <OgreVector.h>
#include <OgreQuaternion.h>
#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace run3::gameplay::detail {

inline Ogre::AnimationState *findBoundsAnimation(Ogre::Entity &entity,
                                          const std::string &preferred) {
  if (!preferred.empty() && entity.hasAnimationState(preferred))
    return entity.getAnimationState(preferred);
  if (entity.hasAnimationState("Walk"))
    return entity.getAnimationState("Walk");
  if (entity.hasAnimationState("Walk1"))
    return entity.getAnimationState("Walk1");
  if (entity.getAllAnimationStates() == nullptr) return nullptr;

  Ogre::AnimationState *idleFallback = nullptr;
  auto iterator = entity.getAllAnimationStates()->getAnimationStateIterator();
  while (iterator.hasMoreElements()) {
    Ogre::AnimationState *state = iterator.getNext();
    std::string name = state->getAnimationName();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](const unsigned char character) {
                     return static_cast<char>(std::tolower(character));
                   });
    if (name.rfind("walk", 0) == 0) return state;
    if (idleFallback == nullptr && name.rfind("idle", 0) == 0)
      idleFallback = state;
  }
  return idleFallback;
}

inline std::optional<Ogre::AxisAlignedBox> animationPoseBounds(
    Ogre::Entity &entity, const std::string &preferred, Ogre::Real time = 0) {
  Ogre::AnimationState *animation = findBoundsAnimation(entity, preferred);
  if (animation == nullptr || !entity.hasSkeleton()) return std::nullopt;

  if (entity.getAllAnimationStates() != nullptr) {
    auto iterator = entity.getAllAnimationStates()->getAnimationStateIterator();
    while (iterator.hasMoreElements()) iterator.getNext()->setEnabled(false);
  }
  animation->setTimePosition(time);
  animation->setLoop(true);
  animation->setEnabled(true);
  Ogre::AnimationStateSet *states = entity.getAllAnimationStates();
  Ogre::SkeletonInstance *skeleton = entity.getSkeleton();
  skeleton->setAnimationState(*states);

  std::vector<Ogre::Affine3> boneMatrices(skeleton->getNumBones());
  skeleton->_getBoneMatrices(boneMatrices.data());
  const Ogre::MeshPtr mesh = entity.getMesh();
  Ogre::AxisAlignedBox bounds;
  auto mergeSkinnedVertices = [&](const Ogre::VertexData *source,
                                  const Ogre::Mesh::IndexMap &indexMap) {
    if (source == nullptr || source->vertexCount == 0 || indexMap.empty())
      return;
    std::unique_ptr<Ogre::VertexData> target(source->clone(true));
    std::vector<const Ogre::Affine3 *> blendMatrices(indexMap.size());
    Ogre::Mesh::prepareMatricesForVertexBlend(
        blendMatrices.data(), boneMatrices.data(), indexMap);
    Ogre::Mesh::softwareVertexBlend(source, target.get(),
                                    blendMatrices.data(),
                                    blendMatrices.size(), false);
    Ogre::AxisAlignedBox partBounds;
    Ogre::Real radius{};
    mesh->_calcBoundsFromVertexBuffer(target.get(), partBounds, radius);
    bounds.merge(partBounds);
  };
  mergeSkinnedVertices(mesh->sharedVertexData,
                       mesh->sharedBlendIndexToBoneIndexMap);
  for (Ogre::SubMesh *subMesh : mesh->getSubMeshes()) {
    if (!subMesh->useSharedVertices)
      mergeSkinnedVertices(subMesh->vertexData,
                           subMesh->blendIndexToBoneIndexMap);
  }
  if (bounds.isNull() || bounds.isInfinite()) return std::nullopt;
  return bounds;
}

struct NpcBodyFit {
  Ogre::Vector3 visualOffset;
  Ogre::Vector3 halfExtents;
};

inline Ogre::AxisAlignedBox scaledNpcPoseBounds(
    const Ogre::AxisAlignedBox &bounds, const Ogre::Quaternion &orientation,
    const Ogre::Vector3 &scale) {
  Ogre::AxisAlignedBox result;
  for (unsigned i = 0; i < 8; ++i)
    result.merge((orientation * bounds.getAllCorners()[i]) * scale);
  return result;
}

inline std::optional<NpcBodyFit> automaticNpcBodyFit(
    Ogre::Entity &entity, const std::string &preferred,
    const Ogre::Quaternion &orientation, const Ogre::Vector3 &scale,
    const Ogre::Vector3 &correction, Ogre::Real offsetY = 0) {
  const auto walking = animationPoseBounds(entity, preferred);
  if (!walking) return std::nullopt;
  const auto base = scaledNpcPoseBounds(*walking, orientation, scale);
  auto minimumY = base.getMinimum().y;
  auto maximumY = base.getMaximum().y;
  auto states = entity.getAllAnimationStates()->getAnimationStateIterator();
  while (states.hasMoreElements()) {
    auto *state = states.getNext();
    auto name = state->getAnimationName();
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (name.rfind("walk", 0) != 0 && name.rfind("idle", 0) != 0) continue;
    // One walking frame can have both feet raised. Cover the locomotion
    // cycle and standing poses without widening the navigation body for arms.
    for (unsigned sample = 0; sample < 32; ++sample) {
      const auto pose = animationPoseBounds(entity, state->getAnimationName(),
          state->getLength() * static_cast<Ogre::Real>(sample) / 32);
      if (!pose) continue;
      const auto bounds = scaledNpcPoseBounds(*pose, orientation, scale);
      minimumY = std::min(minimumY, bounds.getMinimum().y);
      maximumY = std::max(maximumY, bounds.getMaximum().y);
    }
  }
  auto half = base.getHalfSize() * correction;
  half.y = (maximumY - minimumY) * 0.5f * correction.y;
  const auto centre = base.getCenter();
  // The collision body stays centred on its physics transform. Anchor the
  // visible feet to its bottom even when physAutoCorrection shrinks its Y.
  const Ogre::Vector3 offset{-centre.x, -half.y - minimumY + offsetY, -centre.z};
  NpcBodyFit fit{Ogre::Vector3::ZERO, half};
  for (unsigned axis = 0; axis < 3; ++axis)
    if (scale[axis] != 0) fit.visualOffset[axis] = offset[axis] / scale[axis];
  // Preserve the original fitter's initial walking pose for startup.
  animationPoseBounds(entity, preferred);
  return fit;
}

} // namespace run3::gameplay::detail
