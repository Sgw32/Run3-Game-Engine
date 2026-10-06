#include <run3/rendering/Run3SceneManager.hpp>

#include <OgreCamera.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreLight.h>
#include <OgreRenderTarget.h>
#include <OgreRenderTexture.h>
#include <OgreRoot.h>
#include <OgreSceneNode.h>
#include <OgreTexture.h>

#include <algorithm>
#include <cstddef>
#include <functional>

namespace run3::rendering {
namespace {
const Ogre::String typeName = "Run3SceneManager";

void combine(std::size_t &seed, const std::size_t value) {
  seed ^= value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
}
} // namespace

Run3SceneManager::Run3SceneManager(const Ogre::String &name,
                                   const unsigned shadowUpdateInterval)
    : Ogre::SceneManager(name),
      shadowUpdateInterval_(std::max(1U, shadowUpdateInterval)) {}

const Ogre::String &Run3SceneManager::getTypeName() const { return typeName; }

std::size_t
Run3SceneManager::shadowLightSignature(const Ogre::LightList &lights) const {
  std::size_t result = lights.size();
  const auto hashReal = std::hash<Ogre::Real>{};
  for (const Ogre::Light *light : lights) {
    if (light == nullptr || !light->getCastShadows()) continue;
    combine(result, reinterpret_cast<std::size_t>(light));
    combine(result, static_cast<std::size_t>(light->getType()));
    combine(result, static_cast<std::size_t>(light->getLightMask()));
    combine(result, static_cast<std::size_t>(light->isVisible()));
    const Ogre::Vector3 position = light->getDerivedPosition();
    const Ogre::Vector3 direction = light->getDerivedDirection();
    for (const Ogre::Real component : {position.x, position.y, position.z,
                                       direction.x, direction.y, direction.z})
      combine(result, hashReal(component));
  }
  return result;
}

void Run3SceneManager::updateShadowTextures(
    Ogre::Camera *camera, Ogre::Viewport *viewport,
    const Ogre::LightList *lightList) {
  const Ogre::LightList &activeLights =
      lightList == nullptr ? mLightsAffectingFrustum : *lightList;
  const std::size_t signature = shadowLightSignature(activeLights);
  const unsigned long frame = Ogre::Root::getSingleton().getNextFrameNumber();
  const bool lightSetChanged = !hasShadowCache_ || signature != lastShadowLightSignature_;
  const bool intervalElapsed = !hasShadowCache_ ||
      frame - lastShadowUpdateFrame_ >= shadowUpdateInterval_;

  if (lightSetChanged || intervalElapsed) {
    Ogre::SceneManager::updateShadowTextures(camera, viewport, lightList);
    lastShadowBatchCount_ = 0;
    for (std::size_t index = 0; index < getShadowTextureConfigList().size();
         ++index) {
      const Ogre::TexturePtr &texture = getShadowTexture(index);
      const std::size_t faces = texture->getNumFaces();
      const std::size_t layers = std::max<std::size_t>(1, texture->getDepth());
      for (std::size_t face = 0; face < faces; ++face)
        for (std::size_t layer = 0; layer < layers; ++layer)
          lastShadowBatchCount_ += texture->getBuffer(face)->getRenderTarget(
              layer)->getStatistics().batchCount;
    }
    shadowBatchesRendered_ += lastShadowBatchCount_;
    lastShadowUpdateFrame_ = frame;
    lastShadowLightSignature_ = signature;
    hasShadowCache_ = true;
    ++shadowTextureUpdates_;
    return;
  }
  ++shadowTextureUpdatesSkipped_;
  shadowBatchesAvoidedEstimate_ += lastShadowBatchCount_;
}

unsigned Run3SceneManager::shadowUpdateInterval() const noexcept {
  return shadowUpdateInterval_;
}

std::uint64_t Run3SceneManager::shadowTextureUpdates() const noexcept {
  return shadowTextureUpdates_;
}

std::uint64_t Run3SceneManager::shadowTextureUpdatesSkipped() const noexcept {
  return shadowTextureUpdatesSkipped_;
}

std::uint64_t Run3SceneManager::shadowBatchesRendered() const noexcept {
  return shadowBatchesRendered_;
}

std::uint64_t Run3SceneManager::shadowBatchesAvoidedEstimate() const noexcept {
  return shadowBatchesAvoidedEstimate_;
}

Run3SceneManagerFactory::Run3SceneManagerFactory(
    const unsigned shadowUpdateInterval)
    : shadowUpdateInterval_(std::max(1U, shadowUpdateInterval)) {}

const Ogre::String &Run3SceneManagerFactory::getTypeName() const {
  return typeName;
}

Ogre::SceneManager *Run3SceneManagerFactory::createInstance(
    const Ogre::String &instanceName) {
  return OGRE_NEW Run3SceneManager(instanceName, shadowUpdateInterval_);
}

} // namespace run3::rendering
