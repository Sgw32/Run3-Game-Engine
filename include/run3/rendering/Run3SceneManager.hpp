#pragma once

#include <OgreSceneManager.h>

#include <cstdint>

namespace run3::rendering {

// The default Ogre scene manager redraws every shadow texture for every main
// viewport render. Run3 maps mostly use fixed spotlights and static world
// geometry, so refreshing those textures at a small, explicit frame interval
// saves the shadow-caster batches while preserving the normal per-frame scene
// render. A changed shadow-light set always forces an immediate refresh.
class Run3SceneManager final : public Ogre::SceneManager {
public:
  Run3SceneManager(const Ogre::String &name, unsigned shadowUpdateInterval);

  const Ogre::String &getTypeName() const override;
  void updateShadowTextures(Ogre::Camera *, Ogre::Viewport *,
                            const Ogre::LightList *lightList = nullptr) override;

  unsigned shadowUpdateInterval() const noexcept;
  std::uint64_t shadowTextureUpdates() const noexcept;
  std::uint64_t shadowTextureUpdatesSkipped() const noexcept;
  std::uint64_t shadowBatchesRendered() const noexcept;
  std::uint64_t shadowBatchesAvoidedEstimate() const noexcept;

private:
  std::size_t shadowLightSignature(const Ogre::LightList &) const;

  unsigned shadowUpdateInterval_{1};
  unsigned long lastShadowUpdateFrame_{};
  std::size_t lastShadowLightSignature_{};
  bool hasShadowCache_{};
  std::uint64_t shadowTextureUpdates_{};
  std::uint64_t shadowTextureUpdatesSkipped_{};
  std::uint64_t shadowBatchesRendered_{};
  std::uint64_t shadowBatchesAvoidedEstimate_{};
  std::uint64_t lastShadowBatchCount_{};
};

class Run3SceneManagerFactory final : public Ogre::SceneManagerFactory {
public:
  explicit Run3SceneManagerFactory(unsigned shadowUpdateInterval);

  const Ogre::String &getTypeName() const override;
  Ogre::SceneManager *createInstance(const Ogre::String &instanceName) override;

private:
  unsigned shadowUpdateInterval_{1};
};

} // namespace run3::rendering
