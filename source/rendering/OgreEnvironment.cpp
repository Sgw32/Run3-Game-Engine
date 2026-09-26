#include <run3/rendering/Environment.hpp>

#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgrePass.h>
#include <OgrePlane.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>

#include <algorithm>
#include <stdexcept>

namespace run3::rendering {
namespace {

constexpr const char *groupName = "Run3Step9A";
constexpr const char *fallbackSky = "Run3/PortableSkyFallback";
constexpr const char *waterMaterial = "Run3/PortableWater";
constexpr const char *waterMesh = "Run3/PortableWaterPlane";
constexpr const char *waterEntity = "Run3PortableWaterEntity";
constexpr const char *waterNode = "Run3PortableWaterNode";

void ensureGroup() {
  auto &groups = Ogre::ResourceGroupManager::getSingleton();
  if (!groups.resourceGroupExists(groupName)) groups.createResourceGroup(groupName);
}

Ogre::MaterialPtr makeFallbackSky() {
  ensureGroup();
  auto &materials = Ogre::MaterialManager::getSingleton();
  Ogre::MaterialPtr material = materials.getByName(fallbackSky, groupName);
  if (!material) material = materials.create(fallbackSky, groupName);
  material->removeAllTechniques();
  Ogre::Pass *pass = material->createTechnique()->createPass();
  pass->setLightingEnabled(false);
  pass->setDepthWriteEnabled(false);
  pass->setDiffuse(0.10F, 0.18F, 0.30F, 1.0F);
  pass->setSelfIllumination(0.10F, 0.18F, 0.30F);
  material->load();
  return material;
}

Ogre::MaterialPtr makeWaterMaterial() {
  ensureGroup();
  auto &materials = Ogre::MaterialManager::getSingleton();
  Ogre::MaterialPtr material = materials.getByName(waterMaterial, groupName);
  if (!material) material = materials.create(waterMaterial, groupName);
  material->removeAllTechniques();
  Ogre::Pass *pass = material->createTechnique()->createPass();
  pass->setLightingEnabled(true);
  pass->setDiffuse(0.05F, 0.22F, 0.32F, 0.72F);
  pass->setSpecular(0.6F, 0.8F, 0.9F, 0.72F);
  pass->setShininess(48.0F);
  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  pass->setDepthCheckEnabled(true);
  pass->setDepthWriteEnabled(false);
  material->setReceiveShadows(true);
  material->load();
  return material;
}

} // namespace

class PortableOgreEnvironment final : public IEnvironment {
public:
  explicit PortableOgreEnvironment(Ogre::SceneManager &sceneManager)
      : sceneManager_(&sceneManager) {}
  ~PortableOgreEnvironment() override { clear(); }

  void setSky(const SkySettings &settings) override {
    if (!settings.enabled) {
      sceneManager_->setSkyBox(false, {});
      skyEnabled_ = false;
      return;
    }
    std::string effective = settings.material;
    auto &materials = Ogre::MaterialManager::getSingleton();
    if (effective.empty() || !materials.resourceExists(effective)) {
      const std::string requested = effective;
      effective = makeFallbackSky()->getName();
      Ogre::LogManager::getSingleton().logMessage(
          "Step 9A sky material '" + requested +
          "' is unavailable; using visible portable fallback");
    }
    sceneManager_->setSkyBox(true, effective, std::max(100.0F, settings.distance),
                             true);
    skyEnabled_ = true;
  }

  void setWater(const WaterSettings &settings) override {
    destroyWater();
    if (!settings.enabled) return;
    if (settings.width <= 0.0F || settings.depth <= 0.0F)
      throw std::invalid_argument("portable water dimensions must be positive");
    ensureGroup();
    Ogre::MeshManager::getSingleton().createPlane(
        waterMesh, groupName,
        Ogre::Plane(Ogre::Vector3::UNIT_Y,
                    Ogre::Vector3(0.0F, settings.height, 0.0F)),
        settings.width, settings.depth, 1, 1, true, 1, 1.0F, 1.0F,
        Ogre::Vector3::UNIT_Z);
    waterEntity_ = sceneManager_->createEntity(waterEntity, waterMesh, groupName);
    waterEntity_->setMaterial(makeWaterMaterial());
    waterNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode(waterNode);
    waterNode_->attachObject(waterEntity_);
  }

  void clear() noexcept override {
    try { destroyWater(); } catch (...) {}
    if (skyEnabled_) {
      try { sceneManager_->setSkyBox(false, {}); } catch (...) {}
      skyEnabled_ = false;
    }
  }

private:
  void destroyWater() {
    if (waterEntity_ != nullptr) {
      if (waterNode_ != nullptr) waterNode_->detachObject(waterEntity_);
      sceneManager_->destroyEntity(waterEntity_);
      waterEntity_ = nullptr;
    }
    if (waterNode_ != nullptr) {
      sceneManager_->destroySceneNode(waterNode_);
      waterNode_ = nullptr;
    }
    auto &meshes = Ogre::MeshManager::getSingleton();
    if (meshes.resourceExists(waterMesh, groupName)) meshes.remove(waterMesh, groupName);
  }

  Ogre::SceneManager *sceneManager_{};
  Ogre::Entity *waterEntity_{};
  Ogre::SceneNode *waterNode_{};
  bool skyEnabled_{};
};

std::unique_ptr<IEnvironment>
createPortableOgreEnvironment(Ogre::SceneManager &sceneManager) {
  return std::make_unique<PortableOgreEnvironment>(sceneManager);
}

} // namespace run3::rendering
