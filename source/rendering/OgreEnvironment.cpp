#include <run3/rendering/Environment.hpp>

#include <OgreEntity.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgrePass.h>
#include <OgrePixelFormat.h>
#include <OgrePlane.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreTextureUnitState.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

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
  // These portable materials are immutable. Replacing their source techniques
  // after RTSS has registered them leaves dangling pointers on map reload.
  if (material) return material;
  material = materials.create(fallbackSky, groupName);
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
  if (material) return material;
  material = materials.create(waterMaterial, groupName);
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

std::uint32_t ddsU32(const std::vector<unsigned char> &bytes,
                     const std::size_t offset) {
  std::uint32_t value{};
  std::memcpy(&value, bytes.data() + offset, sizeof(value));
  return value;
}

struct DdsFace {
  std::vector<unsigned char> bytes;
  unsigned width{};
  unsigned height{};
  std::uint32_t fourCC{};
};

// Ogre's separateUV cube loader requires all six DDS faces to have the same
// compressed format.  Some shipped TLW skies violate that requirement (the
// sn2morning back face is DXT1 while the other five are DXT5).  Normalize that
// legacy combination in memory, without rewriting game content.
void normalizeLegacyCube(Ogre::Material &material) {
  if (material.getNumTechniques() == 0 ||
      material.getTechnique(0)->getNumPasses() == 0) return;
  Ogre::Pass *pass = material.getTechnique(0)->getPass(0);
  if (pass->getNumTextureUnitStates() == 0) return;
  Ogre::TextureUnitState *unit = pass->getTextureUnitState(0);
  if (unit->getTextureType() != Ogre::TEX_TYPE_CUBE_MAP) return;

  const std::string cubeName = unit->getTextureName();
  const auto dot = cubeName.find_last_of('.');
  if (dot == std::string::npos) return;
  const std::string group = material.getGroup();
  auto &resources = Ogre::ResourceGroupManager::getSingleton();
  static constexpr std::array<const char *, 6> suffixes{
      "_rt", "_lf", "_up", "_dn", "_fr", "_bk"};
  std::array<DdsFace, 6> faces;
  bool mixedDxt1Dxt5 = false;
  std::uint32_t firstFormat{};
  for (std::size_t face = 0; face < faces.size(); ++face) {
    const std::string resource = cubeName.substr(0, dot) + suffixes[face] +
                                 cubeName.substr(dot);
    Ogre::DataStreamPtr stream = resources.openResource(
        resource, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
        nullptr, false);
    if (!stream || stream->size() < 128) return;
    faces[face].bytes.resize(stream->size());
    stream->read(faces[face].bytes.data(), faces[face].bytes.size());
    if (std::memcmp(faces[face].bytes.data(), "DDS ", 4) != 0) return;
    faces[face].height = ddsU32(faces[face].bytes, 12);
    faces[face].width = ddsU32(faces[face].bytes, 16);
    faces[face].fourCC = ddsU32(faces[face].bytes, 84);
    if (face == 0) firstFormat = faces[face].fourCC;
    else mixedDxt1Dxt5 = mixedDxt1Dxt5 || faces[face].fourCC != firstFormat;
    if (faces[face].width != faces[0].width ||
        faces[face].height != faces[0].height) return;
  }
  if (!mixedDxt1Dxt5) return;

  constexpr std::uint32_t dxt1 = 0x31545844U; // "DXT1"
  constexpr std::uint32_t dxt5 = 0x35545844U; // "DXT5"
  for (const DdsFace &face : faces)
    if (face.fourCC != dxt1 && face.fourCC != dxt5) return;

  const std::size_t blocks =
      ((faces[0].width + 3U) / 4U) * ((faces[0].height + 3U) / 4U);
  std::array<std::vector<unsigned char>, 6> pixels;
  for (std::size_t face = 0; face < faces.size(); ++face) {
    const unsigned char *source = faces[face].bytes.data() + 128;
    const std::size_t sourceSize = faces[face].bytes.size() - 128;
    if (faces[face].fourCC == dxt5) {
      if (sourceSize < blocks * 16U) return;
      pixels[face].assign(source, source + blocks * 16U);
      continue;
    }
    if (sourceSize < blocks * 8U) return;
    pixels[face].resize(blocks * 16U);
    for (std::size_t block = 0; block < blocks; ++block) {
      unsigned char *target = pixels[face].data() + block * 16U;
      // Opaque DXT5 alpha followed by the unchanged DXT1 colour block.
      target[0] = 255;
      target[1] = 255;
      std::fill(target + 2, target + 8, 0);
      std::memcpy(target + 8, source + block * 8U, 8U);
    }
  }

  auto &textures = Ogre::TextureManager::getSingleton();
  const std::string normalizedName =
      "Run3/NormalizedCube/" + material.getName();
  if (Ogre::TexturePtr existing = textures.getByName(normalizedName, group))
    textures.remove(existing);
  Ogre::TexturePtr cube = textures.createManual(
      normalizedName, group, Ogre::TEX_TYPE_CUBE_MAP, faces[0].width,
      faces[0].height, 0, Ogre::PF_DXT5, Ogre::TU_DEFAULT);
  for (std::size_t face = 0; face < faces.size(); ++face) {
    Ogre::PixelBox box(faces[0].width, faces[0].height, 1, Ogre::PF_DXT5,
                       pixels[face].data());
    cube->getBuffer(face, 0)->blitFromMemory(box);
  }
  // StaticMap publishes compatibility aliases for legacy material names.  The
  // source compatibility material remains registered too, so retarget every
  // pass that references this exact broken cube; otherwise eager resource
  // preparation still attempts (and logs) the invalid six-file upload.
  auto materials = Ogre::MaterialManager::getSingleton().getResourceIterator();
  while (materials.hasMoreElements()) {
    Ogre::MaterialPtr candidate = Ogre::static_pointer_cast<Ogre::Material>(
        materials.getNext());
    for (Ogre::Technique *technique : candidate->getTechniques())
      for (Ogre::Pass *candidatePass : technique->getPasses())
        for (Ogre::TextureUnitState *candidateUnit :
             candidatePass->getTextureUnitStates())
          if (candidateUnit->getTextureType() == Ogre::TEX_TYPE_CUBE_MAP &&
              candidateUnit->getTextureName() == cubeName)
            candidateUnit->setTextureName(normalizedName,
                                           Ogre::TEX_TYPE_CUBE_MAP);
  }
  Ogre::LogManager::getSingleton().logMessage(
      "Step 9A normalized mixed DXT1/DXT5 sky cube '" + cubeName +
      "' as '" + normalizedName + "'");
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
    if (Ogre::MaterialPtr material = materials.getByName(effective))
      normalizeLegacyCube(*material);
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
    if (Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(groupName) &&
        meshes.resourceExists(waterMesh, groupName)) meshes.remove(waterMesh, groupName);
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
