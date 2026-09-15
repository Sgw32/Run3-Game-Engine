#include <run3/gameplay/StaticMap.hpp>

#include <run3/core/Log.hpp>
#include <run3/gameplay/LegacyMaterialCatalog.hpp>

#include <OgreAxisAlignedBox.h>
#include <OgreEntity.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreLogManager.h>
#include <OgreLog.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreMeshSerializer.h>
#include <OgrePass.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreShaderGenerator.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace fs = std::filesystem;
namespace {

class OgreConsoleSilencer final {
public:
  OgreConsoleSilencer() : log_(Ogre::LogManager::getSingleton().getDefaultLog()) {
    if (log_ != nullptr) {
      previous_ = log_->isDebugOutputEnabled();
      log_->setDebugOutputEnabled(false);
    }
  }
  ~OgreConsoleSilencer() {
    if (log_ != nullptr) {
      log_->setDebugOutputEnabled(previous_);
    }
  }

private:
  Ogre::Log *log_{};
  bool previous_{};
};

std::string readText(const fs::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("cannot read map file: " + path.string());
  }
  return {std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>()};
}

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::unordered_map<std::string, std::string> attributes(std::string_view text) {
  static const std::regex expression(
      R"ATTR(([A-Za-z_][A-Za-z0-9_]*)\s*=\s*["']([^"']*)["'])ATTR");
  std::unordered_map<std::string, std::string> result;
  const std::string copy(text);
  for (std::sregex_iterator it(copy.begin(), copy.end(), expression), end;
       it != end; ++it) {
    result[(*it)[1].str()] = (*it)[2].str();
  }
  return result;
}

double number(const std::unordered_map<std::string, std::string> &values,
              const char *key, double fallback) {
  const auto found = values.find(key);
  if (found == values.end()) {
    return fallback;
  }
  try {
    return std::stod(found->second);
  } catch (...) {
    return fallback;
  }
}

physics::Vec3 vector(const std::unordered_map<std::string, std::string> &values,
                     physics::Vec3 fallback = {}) {
  const double multiplier = number(values, "m", 1.0);
  return {number(values, "x", fallback.x) * multiplier,
          number(values, "y", fallback.y) * multiplier,
          number(values, "z", fallback.z) * multiplier};
}

physics::Quaternion quaternion(
    const std::unordered_map<std::string, std::string> &values) {
  return {number(values, "qw", 1.0), number(values, "qx", 0.0),
          number(values, "qy", 0.0), number(values, "qz", 0.0)};
}

Ogre::Vector3 toOgre(physics::Vec3 value) {
  return {static_cast<Ogre::Real>(value.x), static_cast<Ogre::Real>(value.y),
          static_cast<Ogre::Real>(value.z)};
}

Ogre::Quaternion toOgre(physics::Quaternion value) {
  return {static_cast<Ogre::Real>(value.w), static_cast<Ogre::Real>(value.x),
          static_cast<Ogre::Real>(value.y), static_cast<Ogre::Real>(value.z)};
}

physics::Vec3 fromOgre(const Ogre::Vector3 &value) {
  return {value.x, value.y, value.z};
}

physics::Quaternion fromOgre(const Ogre::Quaternion &value) {
  return {value.w, value.x, value.y, value.z};
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string normalizedMapName(std::string name) {
  name = lower(std::move(name));
  if (name == "tlwhome2") {
    return "tlwhome02";
  }
  if (name != "tlwhome02" && name != "tlwcao") {
    throw std::invalid_argument(
        "Run3 supports --map tlwhome02 (alias tlwhome2) or --map tlwcao");
  }
  return name;
}

struct MeshGeometry {
  std::vector<physics::Vec3> vertices;
  std::vector<std::uint32_t> indices;
};

MeshGeometry extractMesh(const Ogre::MeshPtr &mesh, const Ogre::Vector3 &scale) {
  MeshGeometry result;
  for (unsigned subIndex = 0; subIndex < mesh->getNumSubMeshes(); ++subIndex) {
    Ogre::SubMesh *subMesh = mesh->getSubMesh(subIndex);
    Ogre::VertexData *vertexData =
        subMesh->useSharedVertices ? mesh->sharedVertexData : subMesh->vertexData;
    if (vertexData == nullptr || subMesh->indexData == nullptr) {
      continue;
    }
    const Ogre::VertexElement *positionElement =
        vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
    if (positionElement == nullptr) {
      continue;
    }
    const Ogre::HardwareVertexBufferSharedPtr vertexBuffer =
        vertexData->vertexBufferBinding->getBuffer(positionElement->getSource());
    auto *vertexBytes = static_cast<unsigned char *>(
        vertexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
    const std::uint32_t base = static_cast<std::uint32_t>(result.vertices.size());
    for (std::size_t index = 0; index < vertexData->vertexCount; ++index) {
      float *position{};
      positionElement->baseVertexPointerToElement(
          vertexBytes + (vertexData->vertexStart + index) *
                            vertexBuffer->getVertexSize(),
          &position);
      result.vertices.push_back({position[0] * scale.x, position[1] * scale.y,
                                 position[2] * scale.z});
    }
    vertexBuffer->unlock();

    const Ogre::HardwareIndexBufferSharedPtr indexBuffer =
        subMesh->indexData->indexBuffer;
    const bool use32 = indexBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT;
    const void *raw = indexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
    const bool mirrored = scale.x * scale.y * scale.z < 0;
    for (std::size_t index = 0; index + 2 < subMesh->indexData->indexCount;
         index += 3) {
      const std::size_t offset = subMesh->indexData->indexStart + index;
      const auto read = [&](std::size_t at) {
        return use32 ? static_cast<const std::uint32_t *>(raw)[at]
                     : static_cast<std::uint32_t>(
                           static_cast<const std::uint16_t *>(raw)[at]);
      };
      std::uint32_t first = base + read(offset);
      std::uint32_t second = base + read(offset + 1);
      std::uint32_t third = base + read(offset + 2);
      if (mirrored) {
        std::swap(second, third);
      }
      result.indices.insert(result.indices.end(), {first, second, third});
    }
    indexBuffer->unlock();
  }
  return result;
}

} // namespace

bool AxisAlignedVolume::contains(physics::Vec3 point) const noexcept {
  return point.x >= minimum.x && point.x <= maximum.x &&
         point.y >= minimum.y && point.y <= maximum.y &&
         point.z >= minimum.z && point.z <= maximum.z;
}

class StaticMap::Impl {
public:
  Impl(Ogre::SceneManager &sceneManager, physics::PhysicsWorld &world)
      : sceneManager_(&sceneManager), world_(&world) {}

  StaticMapStats load(const StaticMapOptions &options) {
    unload();
    OgreConsoleSilencer silenceBulkLoad;
    const std::string map = normalizedMapName(options.mapName);
    const fs::path mapDirectory =
        options.contentRoot / "run3" / "maps" / options.quality / map;
    const fs::path sceneConfig = mapDirectory / "scene.cfg";
    std::ifstream config(sceneConfig);
    if (!config) {
      throw std::runtime_error("missing map scene.cfg: " + sceneConfig.string());
    }
    std::string sceneFile;
    std::string line;
    while (std::getline(config, line)) {
      line = trim(line);
      if (line.rfind("Scene=", 0) == 0) {
        sceneFile = trim(line.substr(6));
        break;
      }
    }
    if (sceneFile.empty()) {
      throw std::runtime_error("scene.cfg has no Scene entry: " +
                               sceneConfig.string());
    }

    registerResources(options.contentRoot, options.resourceProfile);
    materialCatalog_.scan(options.contentRoot);
    class CompatibilityListener final : public Ogre::MeshSerializerListener {
    public:
      explicit CompatibilityListener(Impl &owner) : owner_(&owner) {}
      void processMaterialName(Ogre::Mesh *, Ogre::String *name) override {
        if (const Ogre::MaterialPtr material = owner_->compatibleMaterial(*name)) {
          *name = material->getName();
        }
      }
      void processSkeletonName(Ogre::Mesh *, Ogre::String *) override {}
      void processMeshCompleted(Ogre::Mesh *) override {}

    private:
      Impl *owner_{};
    } materialListener(*this);
    Ogre::MeshManager &meshManager = Ogre::MeshManager::getSingleton();
    Ogre::MeshSerializerListener *previousListener = meshManager.getListener();
    meshManager.setListener(&materialListener);
    struct ListenerRestore final {
      Ogre::MeshManager *manager{};
      Ogre::MeshSerializerListener *listener{};
      ~ListenerRestore() { manager->setListener(listener); }
    } restoreListener{&meshManager, previousListener};
    const std::string xml = readText(mapDirectory / sceneFile);
    rootNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode(
        "Run3Step6BMapRoot");

    static const std::regex tagExpression(
        R"TAG(<\s*(/?)\s*([A-Za-z_][A-Za-z0-9_]*)\b([^>]*)>)TAG");
    std::vector<Ogre::SceneNode *> nodes{rootNode_};
    std::vector<std::string> nodeNames{"Run3Step6BMapRoot"};
    double sceneMultiplier = 1.0;
    bool firstPlayer = true;
    std::size_t sequence = 0;
    Ogre::Entity *activeEntity{};
    std::string activeEntityTag;

    for (std::sregex_iterator it(xml.begin(), xml.end(), tagExpression), end;
         it != end; ++it) {
      const bool closing = !(*it)[1].str().empty();
      const std::string tag = lower((*it)[2].str());
      const std::string rawAttributes = (*it)[3].str();
      const auto values = attributes(rawAttributes);
      const bool selfClosing = rawAttributes.find('/') != std::string::npos &&
                               rawAttributes.find_last_not_of(" \t\r\n") !=
                                   std::string::npos &&
                               rawAttributes[rawAttributes.find_last_not_of(
                                   " \t\r\n")] == '/';
      if (closing) {
        if (activeEntity != nullptr && tag == activeEntityTag) {
          activeEntity = nullptr;
          activeEntityTag.clear();
        }
        if (tag == "node" && nodes.size() > 1) {
          nodes.pop_back();
          nodeNames.pop_back();
        }
        continue;
      }
      if (tag == "scene") {
        sceneMultiplier = number(values, "multiplier", 1.0);
      } else if (tag == "player" && firstPlayer) {
        spawn_ = vector(values);
        spawn_.x *= sceneMultiplier;
        spawn_.y *= sceneMultiplier;
        spawn_.z *= sceneMultiplier;
        firstPlayer = false;
      } else if (tag == "node") {
        const auto nameIt = values.find("name");
        const std::string nodeName =
            nameIt == values.end() ? "Run3MapNode" : nameIt->second;
        Ogre::SceneNode *node = nodes.back()->createChildSceneNode(
            "Run3Step6B/" + std::to_string(sequence++) + "/" + nodeName);
        nodes.push_back(node);
        nodeNames.push_back(nodeName);
        if (selfClosing) {
          nodes.pop_back();
          nodeNames.pop_back();
        }
      } else if (tag == "position" && nodes.size() > 1) {
        physics::Vec3 position = vector(values);
        position.x *= sceneMultiplier;
        position.y *= sceneMultiplier;
        position.z *= sceneMultiplier;
        nodes.back()->setPosition(toOgre(position));
      } else if (tag == "rotation" && nodes.size() > 1) {
        nodes.back()->setOrientation(toOgre(quaternion(values)));
      } else if (tag == "scale" && nodes.size() > 1) {
        physics::Vec3 scale = vector(values, {1.0, 1.0, 1.0});
        nodes.back()->setScale(toOgre(physics::Vec3{
            scale.x * sceneMultiplier, scale.y * sceneMultiplier,
            scale.z * sceneMultiplier}));
      } else if ((tag == "entity" || tag == "nocollide" || tag == "phys" ||
                  tag == "breakable" || tag == "pblock" ||
                  tag == "blockbox") &&
                 nodes.size() > 1) {
        const auto meshIt = values.find("meshFile");
        if (meshIt == values.end() || meshIt->second.empty()) {
          ++stats_.skippedSections;
          continue;
        }
        try {
          const std::string entityName =
              "Run3Step6BEntity/" + std::to_string(sequence++);
          Ogre::Entity *entity = sceneManager_->createEntity(
              entityName, meshIt->second, resourceGroup_);
          const auto entityMaterial = values.find("materialFile");
          if (entityMaterial != values.end()) {
            for (unsigned subIndex = 0;
                 subIndex < entity->getNumSubEntities(); ++subIndex) {
              assignCompatibleMaterial(entity->getSubEntity(subIndex),
                                       entityMaterial->second);
            }
          }
          nodes.back()->attachObject(entity);
          if (tag == "pblock" || tag == "blockbox") {
            entity->setVisible(false);
          }
          entities_.push_back(entity);
          activeEntity = entity;
          activeEntityTag = tag;
          if (selfClosing) {
            activeEntity = nullptr;
            activeEntityTag.clear();
          }
          debugNodes_.push_back(nodes.back());
          ++stats_.visualSections;
          if (tag != "nocollide") {
            nodes.back()->_update(true, true);
            const Ogre::Vector3 derivedScale = nodes.back()->_getDerivedScale();
            const Ogre::MeshPtr mesh = entity->getMesh();
            const bool dynamic = tag == "phys" || tag == "breakable";
            if (dynamic) {
              const Ogre::AxisAlignedBox bounds = entity->getBoundingBox();
              Ogre::Vector3 half = bounds.getHalfSize();
              half.x = std::abs(half.x * derivedScale.x);
              half.y = std::abs(half.y * derivedScale.y);
              half.z = std::abs(half.z * derivedScale.z);
              constexpr Ogre::Real minimumHalfExtent = 0.01F;
              half.makeCeil(Ogre::Vector3(minimumHalfExtent));
              const Ogre::Quaternion orientation =
                  nodes.back()->_getDerivedOrientation();
              const Ogre::Vector3 localCenter = bounds.getCenter() * derivedScale;
              physics::BodyDesc body(physics::Shape::box(fromOgre(half)));
              body.motion = physics::BodyMotion::Dynamic;
              body.massKg = number(values, "mass", tag == "breakable" ? 40.0 : 10.0);
              if (body.massKg <= 0.0) {
                body.massKg = 10.0;
              }
              body.group = physics::CollisionGroup::Dynamic;
              body.mask = physics::collisionMask(physics::CollisionGroup::All);
              body.transform.position = fromOgre(
                  nodes.back()->_getDerivedPosition() + orientation * localCenter);
              body.transform.rotation = fromOgre(orientation);
              body.metadata.entityId = sequence;
              body.metadata.type = tag == "breakable"
                                       ? physics::BodyType::Breakable
                                       : physics::BodyType::PhysicalObject;
              BodyBinding binding;
              binding.node = nodes.back();
              binding.localCenter = localCenter;
              binding.body = world_->createBody(body);
              bodies_.push_back(std::move(binding));
              ++stats_.collisionSections;
            } else {
              MeshGeometry geometry = extractMesh(mesh, derivedScale);
              if (geometry.indices.empty()) {
                ++stats_.skippedSections;
                continue;
              }
              physics::BodyDesc body(physics::Shape::triangleMesh(
                  std::move(geometry.vertices), std::move(geometry.indices)));
              body.motion = physics::BodyMotion::Static;
              body.group = physics::CollisionGroup::World;
              body.mask =
                  physics::collisionMask(physics::CollisionGroup::Player) |
                  physics::collisionMask(physics::CollisionGroup::Dynamic) |
                  physics::collisionMask(physics::CollisionGroup::Npc) |
                  physics::collisionMask(physics::CollisionGroup::Projectile);
              body.transform.position =
                  fromOgre(nodes.back()->_getDerivedPosition());
              body.transform.rotation =
                  fromOgre(nodes.back()->_getDerivedOrientation());
              body.metadata.entityId = sequence;
              body.metadata.type = physics::BodyType::World;
              const std::size_t triangleCount =
                  entity->getMesh()->getNumSubMeshes();
              static_cast<void>(triangleCount);
              BodyBinding binding;
              binding.body = world_->createBody(body);
              bodies_.push_back(std::move(binding));
              stats_.triangles += body.shape.indices().size() / 3;
              ++stats_.collisionSections;
            }
          }

          if (lower(nodeNames.back()).find("ladder") != std::string::npos) {
            const Ogre::AxisAlignedBox bounds = entity->getWorldBoundingBox(true);
            if (!bounds.isNull() && !bounds.isInfinite()) {
              ladders_.push_back({fromOgre(bounds.getMinimum()),
                                  fromOgre(bounds.getMaximum()),
                                  nodeNames.back()});
            }
          }
        } catch (const Ogre::Exception &error) {
          ++stats_.skippedSections;
          Ogre::LogManager::getSingleton().logMessage(
              "Step 6C skipped '" + meshIt->second + "': " +
              error.getDescription());
        }
      } else if ((tag == "subentity" || tag == "subnocollide") &&
                 activeEntity != nullptr) {
        const auto material = values.find("materialName");
        const auto index = values.find("index");
        if (material != values.end() && index != values.end()) {
          try {
            const auto subIndex =
                static_cast<unsigned>(std::stoul(index->second));
            if (subIndex < activeEntity->getNumSubEntities()) {
              assignCompatibleMaterial(activeEntity->getSubEntity(subIndex),
                                       material->second);
            }
          } catch (const std::exception &) {
            Ogre::LogManager::getSingleton().logMessage(
                "Step 6C ignored invalid subentity index: " + index->second);
          }
        }
      }
    }
    Ogre::LogManager::getSingleton().logMessage(
        "Step 6C map " + map + ": visuals=" +
        std::to_string(stats_.visualSections) + " collision=" +
        std::to_string(stats_.collisionSections) + " triangles=" +
        std::to_string(stats_.triangles) + " skipped=" +
        std::to_string(stats_.skippedSections) + " textured-materials=" +
        std::to_string(compatibleMaterials_.size()) +
        " unresolved-materials=" +
        std::to_string(unresolvedMaterials_.size()));
    return stats_;
  }

  Ogre::MaterialPtr compatibleMaterial(const std::string &legacyName) {
    if (legacyName.empty() || legacyName == "BaseWhite") {
      return {};
    }
    if (const auto cached = compatibleMaterials_.find(legacyName);
        cached != compatibleMaterials_.end()) {
      return cached->second;
    }
    const std::optional<LegacyMaterialInfo> legacy =
        materialCatalog_.find(legacyName);
    if (!legacy) {
      if (unresolvedMaterials_.insert(legacyName).second) {
        Ogre::LogManager::getSingleton().logMessage(
            "Step 6C texture fallback: no diffuse texture for material '" +
            legacyName + "'");
      }
      return {};
    }

    const std::string generatedName =
        "Run3/CompatTexture/" + std::to_string(compatibleMaterials_.size());
    Ogre::MaterialPtr generated = Ogre::MaterialManager::getSingleton().create(
        generatedName, resourceGroup_);
    Ogre::Pass *pass = generated->getTechnique(0)->getPass(0);
    pass->setLightingEnabled(true);
    pass->setAmbient(1.0F, 1.0F, 1.0F);
    pass->setDiffuse(1.0F, 1.0F, 1.0F, 1.0F);
    Ogre::TextureUnitState *texture =
        pass->createTextureUnitState(legacy->texture);
    texture->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
    texture->setTextureAnisotropy(8);
    if (legacy->transparent) {
      pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
      pass->setDepthWriteEnabled(false);
    }
    if (legacy->doubleSided) {
      pass->setCullingMode(Ogre::CULL_NONE);
    }
    if (Ogre::RTShader::ShaderGenerator *shaderGenerator =
            Ogre::RTShader::ShaderGenerator::getSingletonPtr()) {
      static_cast<void>(shaderGenerator->createShaderBasedTechnique(
          *generated, Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
          Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME));
    }
    compatibleMaterials_[legacyName] = generated;
    return generated;
  }

  void assignCompatibleMaterial(Ogre::SubEntity *subEntity,
                                const std::string &legacyName) {
    const Ogre::MaterialPtr generated = compatibleMaterial(legacyName);
    if (generated) {
      subEntity->setMaterial(generated);
    } else {
      subEntity->setMaterialName("BaseWhite");
    }
  }

  void registerResources(const fs::path &contentRoot,
                         const std::string &profile) {
    Ogre::ResourceGroupManager &manager =
        Ogre::ResourceGroupManager::getSingleton();
    if (manager.resourceGroupExists(resourceGroup_)) {
      manager.destroyResourceGroup(resourceGroup_);
    }
    manager.createResourceGroup(resourceGroup_);
    const fs::path configPath = contentRoot / profile;
    std::ifstream stream(configPath);
    if (!stream) {
      throw std::runtime_error("missing resource profile: " +
                               configPath.string());
    }
    std::set<fs::path> added;
    std::string line;
    while (std::getline(stream, line)) {
      line = trim(line);
      if (line.empty() || line.front() == '#' || line.front() == '[') {
        continue;
      }
      const auto equals = line.find('=');
      if (equals == std::string::npos) {
        continue;
      }
      const std::string type = trim(line.substr(0, equals));
      fs::path path = (contentRoot / trim(line.substr(equals + 1))).lexically_normal();
      if ((type != "FileSystem" && type != "Zip") || !fs::exists(path) ||
          !added.insert(path).second) {
        continue;
      }
      try {
        manager.addResourceLocation(path.string(), type, resourceGroup_, false,
                                    true);
      } catch (const Ogre::Exception &error) {
        Ogre::LogManager::getSingleton().logMessage(
            "Step 6C resource skipped: " + path.string() + ": " +
            error.getDescription());
      }
    }
    // Do not initialise the full legacy script set: many programs target
    // D3D9-era profiles. Static-map materials are rebuilt as texture-preserving
    // RTSS materials instead.
  }

  void unload() noexcept {
    bodies_.clear();
    ladders_.clear();
    debugNodes_.clear();
    if (sceneManager_ != nullptr) {
      for (Ogre::Entity *entity : entities_) {
        try {
          sceneManager_->destroyEntity(entity);
        } catch (...) {
        }
      }
      entities_.clear();
      if (rootNode_ != nullptr) {
        try {
          rootNode_->removeAndDestroyAllChildren();
          sceneManager_->destroySceneNode(rootNode_);
        } catch (...) {
        }
        rootNode_ = nullptr;
      }
    }
    stats_ = {};
    compatibleMaterials_.clear();
    unresolvedMaterials_.clear();
  }

  void syncDynamicTransforms() {
    for (auto &binding : bodies_) {
      if (binding.node == nullptr || !binding.body.valid()) {
        continue;
      }
      const physics::Transform transform =
          world_->interpolatedTransform(binding.body);
      const Ogre::Quaternion orientation = toOgre(transform.rotation);
      const Ogre::Vector3 derivedPosition =
          toOgre(transform.position) - orientation * binding.localCenter;
      Ogre::Node *parent = binding.node->getParent();
      if (parent != nullptr) {
        const Ogre::Quaternion parentOrientation =
            parent->_getDerivedOrientation();
        binding.node->setPosition(parentOrientation.Inverse() *
                                  (derivedPosition - parent->_getDerivedPosition()) /
                                  parent->_getDerivedScale());
        binding.node->setOrientation(parentOrientation.Inverse() * orientation);
      } else {
        binding.node->setPosition(derivedPosition);
        binding.node->setOrientation(orientation);
      }
    }
  }

  struct BodyBinding {
    Ogre::SceneNode *node{};
    Ogre::Vector3 localCenter{Ogre::Vector3::ZERO};
    physics::BodyHandle body;
  };

  Ogre::SceneManager *sceneManager_{};
  physics::PhysicsWorld *world_{};
  Ogre::SceneNode *rootNode_{};
  std::vector<Ogre::Entity *> entities_;
  std::vector<Ogre::SceneNode *> debugNodes_;
  std::vector<BodyBinding> bodies_;
  std::vector<AxisAlignedVolume> ladders_;
  LegacyMaterialCatalog materialCatalog_;
  std::unordered_map<std::string, Ogre::MaterialPtr> compatibleMaterials_;
  std::set<std::string> unresolvedMaterials_;
  physics::Vec3 spawn_{};
  StaticMapStats stats_;
  const Ogre::String resourceGroup_{"Run3Step6BContent"};
};

StaticMap::StaticMap(Ogre::SceneManager &sceneManager,
                     physics::PhysicsWorld &world)
    : implementation_(std::make_unique<Impl>(sceneManager, world)) {}
StaticMap::~StaticMap() = default;
StaticMap::StaticMap(StaticMap &&) noexcept = default;
StaticMap &StaticMap::operator=(StaticMap &&) noexcept = default;
StaticMapStats StaticMap::load(const StaticMapOptions &options) {
  return implementation_->load(options);
}
void StaticMap::unload() noexcept { implementation_->unload(); }
void StaticMap::setDebugDraw(bool enabled) {
  for (Ogre::SceneNode *node : implementation_->debugNodes_) {
    node->showBoundingBox(enabled);
  }
}
void StaticMap::syncDynamicTransforms() {
  implementation_->syncDynamicTransforms();
}
physics::Vec3 StaticMap::spawnPosition() const noexcept {
  return implementation_->spawn_;
}
const std::vector<AxisAlignedVolume> &StaticMap::ladderVolumes() const {
  return implementation_->ladders_;
}
const StaticMapStats &StaticMap::stats() const noexcept {
  return implementation_->stats_;
}

} // namespace run3::gameplay
