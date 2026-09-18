#include <run3/gameplay/StaticMap.hpp>

#include <run3/content/MapDefinition.hpp>
#include <run3/core/Log.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/LegacyMaterialCatalog.hpp>
#include <run3/gameplay/MapRuntimeAdapter.hpp>

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
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
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

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::unordered_map<std::string, std::string>
attributes(const content::AuthoredElement &element) {
  std::unordered_map<std::string, std::string> result;
  for (const auto &[name, value] : element.attributes) {
    result[name] = value;
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
    if (options.paths == nullptr) {
      throw std::invalid_argument("StaticMapOptions.paths is required");
    }
    definition_ = content::loadMapDefinition(*options.paths, options.mapName,
                                             options.quality);
    for (const content::AuthoredElement *element :
         activeMapRenderables(*definition_)) {
      activeRenderables_.insert(element);
    }
    const std::string &map = definition_->mapName;
    const RegistryPopulationResult population =
        populateEntityRegistry(*definition_, registry_, false);
    for (const content::DefinitionIssue &issue : definition_->issues) {
      Ogre::LogManager::getSingleton().logMessage(
          issue.source.file.generic_string() + ":" +
          std::to_string(issue.source.line) + ": " + issue.message);
    }
    for (const content::DefinitionIssue &issue : population.issues) {
      Ogre::LogManager::getSingleton().logMessage(
          issue.source.file.generic_string() + ":" +
          std::to_string(issue.source.line) + ": " + issue.message);
    }

    registerResources(options.paths->contentRoot(), options.resourceProfile);
    materialCatalog_.scan(options.paths->contentRoot());
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
    rootNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode(
        "Run3Step6BMapRoot");
    const double sceneMultiplier =
        number(attributes(definition_->scene), "multiplier", 1.0);
    bool firstPlayer = true;
    sequence_ = 0;
    processSceneElement(definition_->scene, rootNode_, "Run3Step6BMapRoot",
                        sceneMultiplier, firstPlayer);
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

  std::optional<EntityHandle>
  registryHandleFor(const content::AuthoredElement &element) const {
    const std::string *name = element.attribute("name");
    if (name == nullptr) {
      return std::nullopt;
    }
    for (const EntityHandle handle : registry_.findAll(*name)) {
      const EntityRecord &record = registry_.get(handle);
      if (record.descriptor.authoredOrder == element.order &&
          record.descriptor.source.file == element.source.file) {
        return handle;
      }
    }
    return std::nullopt;
  }

  void processSceneElement(const content::AuthoredElement &element,
                           Ogre::SceneNode *parent,
                           const std::string &parentName,
                           const double sceneMultiplier, bool &firstPlayer) {
    if (element.tag == "integratedSequence") {
      return;
    }
    const auto values = attributes(element);
    if (element.tag == "player" && firstPlayer) {
      spawn_ = vector(values);
      spawn_.x *= sceneMultiplier;
      spawn_.y *= sceneMultiplier;
      spawn_.z *= sceneMultiplier;
      firstPlayer = false;
      return;
    }
    if (element.tag == "node") {
      const auto name = values.find("name");
      const std::string nodeName =
          name == values.end() ? "Run3MapNode" : name->second;
      Ogre::SceneNode *node = parent->createChildSceneNode(
          "Run3Step8B/" + std::to_string(sequence_++) + "/" + nodeName);
      if (const content::AuthoredElement *position =
              element.firstChild("position")) {
        physics::Vec3 value = vector(attributes(*position));
        value.x *= sceneMultiplier;
        value.y *= sceneMultiplier;
        value.z *= sceneMultiplier;
        node->setPosition(toOgre(value));
      }
      if (const content::AuthoredElement *rotation =
              element.firstChild("rotation")) {
        node->setOrientation(toOgre(quaternion(attributes(*rotation))));
      }
      if (const content::AuthoredElement *scale = element.firstChild("scale")) {
        const physics::Vec3 value =
            vector(attributes(*scale), {1.0, 1.0, 1.0});
        node->setScale(toOgre(physics::Vec3{value.x * sceneMultiplier,
                                           value.y * sceneMultiplier,
                                           value.z * sceneMultiplier}));
      }
      for (const content::AuthoredElement &child : element.children) {
        if (child.tag != "position" && child.tag != "rotation" &&
            child.tag != "scale") {
          processSceneElement(child, node, nodeName, sceneMultiplier,
                              firstPlayer);
        }
      }
      return;
    }

    if (isMapRenderableTag(element.tag)) {
      if (activeRenderables_.count(&element) != 0) {
        processRenderable(element, parent, parentName);
      }
      return;
    }
    for (const content::AuthoredElement &child : element.children) {
      processSceneElement(child, parent, parentName, sceneMultiplier,
                          firstPlayer);
    }
  }

  void processRenderable(const content::AuthoredElement &element,
                         Ogre::SceneNode *node, const std::string &nodeName) {
    // A dynamic body bound here would make syncDynamicTransforms move the
    // complete map.  activeMapRenderables() should make this unreachable, but
    // keep the invariant local to the unsafe operation as a second defence.
    if (node == rootNode_) {
      Ogre::LogManager::getSingleton().logMessage(
          "Step 8B ignored root-level renderable at " +
          element.source.file.generic_string() + ":" +
          std::to_string(element.source.line));
      return;
    }
    const auto values = attributes(element);
    const auto meshIt = values.find("meshFile");
    if (meshIt == values.end() || meshIt->second.empty()) {
      ++stats_.skippedSections;
      return;
    }
    const std::string &tag = element.tag;
    try {
      const std::uint64_t objectKey = sequence_++;
      Ogre::Entity *entity = sceneManager_->createEntity(
          "Run3Step8BEntity/" + std::to_string(objectKey), meshIt->second,
          resourceGroup_);
      const auto entityMaterial = values.find("materialFile");
      if (entityMaterial != values.end()) {
        for (unsigned subIndex = 0; subIndex < entity->getNumSubEntities();
             ++subIndex) {
          assignCompatibleMaterial(entity->getSubEntity(subIndex),
                                   entityMaterial->second);
        }
      }
      for (const content::AuthoredElement &child : element.children) {
        if (child.tag != "subentity" && child.tag != "subnocollide") {
          continue;
        }
        const auto childValues = attributes(child);
        const auto material = childValues.find("materialName");
        const auto index = childValues.find("index");
        if (material == childValues.end() || index == childValues.end()) {
          continue;
        }
        try {
          const auto subIndex = static_cast<unsigned>(std::stoul(index->second));
          if (subIndex < entity->getNumSubEntities()) {
            assignCompatibleMaterial(entity->getSubEntity(subIndex),
                                     material->second);
          }
        } catch (const std::exception &) {
          Ogre::LogManager::getSingleton().logMessage(
              "Step 8B ignored invalid subentity index at " +
              child.source.file.generic_string() + ":" +
              std::to_string(child.source.line));
        }
      }
      node->attachObject(entity);
      if (tag == "pblock" || tag == "blockbox") {
        entity->setVisible(false);
      }
      entities_.push_back(entity);
      debugNodes_.push_back(node);
      ++stats_.visualSections;
      if (const auto handle = registryHandleFor(element)) {
        registry_.bindPresentation(*handle, objectKey);
      }

      if (tag != "nocollide") {
        node->_update(true, true);
        const Ogre::Vector3 derivedScale = node->_getDerivedScale();
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
          const Ogre::Quaternion orientation = node->_getDerivedOrientation();
          const Ogre::Vector3 localCenter = bounds.getCenter() * derivedScale;
          physics::BodyDesc body(physics::Shape::box(fromOgre(half)));
          body.motion = physics::BodyMotion::Dynamic;
          body.massKg =
              number(values, "mass", tag == "breakable" ? 40.0 : 10.0);
          if (body.massKg <= 0.0) {
            body.massKg = 10.0;
          }
          body.group = physics::CollisionGroup::Dynamic;
          body.mask = physics::collisionMask(physics::CollisionGroup::All);
          body.transform.position = fromOgre(node->_getDerivedPosition() +
                                             orientation * localCenter);
          body.transform.rotation = fromOgre(orientation);
          body.metadata.entityId = objectKey;
          body.metadata.type = tag == "breakable"
                                   ? physics::BodyType::Breakable
                                   : physics::BodyType::PhysicalObject;
          BodyBinding binding;
          binding.node = node;
          binding.localCenter = localCenter;
          binding.body = world_->createBody(body);
          if (const auto handle = registryHandleFor(element)) {
            registry_.bindPhysics(*handle, binding.body.id());
          }
          bodies_.push_back(std::move(binding));
          ++stats_.collisionSections;
        } else {
          MeshGeometry geometry = extractMesh(mesh, derivedScale);
          if (geometry.indices.empty()) {
            ++stats_.skippedSections;
            return;
          }
          physics::BodyDesc body(physics::Shape::triangleMesh(
              std::move(geometry.vertices), std::move(geometry.indices)));
          body.motion = physics::BodyMotion::Static;
          body.group = physics::CollisionGroup::World;
          body.mask = physics::collisionMask(physics::CollisionGroup::Player) |
                      physics::collisionMask(physics::CollisionGroup::Dynamic) |
                      physics::collisionMask(physics::CollisionGroup::Npc) |
                      physics::collisionMask(
                          physics::CollisionGroup::Projectile);
          body.transform.position = fromOgre(node->_getDerivedPosition());
          body.transform.rotation = fromOgre(node->_getDerivedOrientation());
          body.metadata.entityId = objectKey;
          body.metadata.type = physics::BodyType::World;
          BodyBinding binding;
          binding.body = world_->createBody(body);
          if (const auto handle = registryHandleFor(element)) {
            registry_.bindPhysics(*handle, binding.body.id());
          }
          bodies_.push_back(std::move(binding));
          stats_.triangles += body.shape.indices().size() / 3;
          ++stats_.collisionSections;
        }
      }

      if (lower(nodeName).find("ladder") != std::string::npos) {
        const Ogre::AxisAlignedBox bounds = entity->getWorldBoundingBox(true);
        if (!bounds.isNull() && !bounds.isInfinite()) {
          ladders_.push_back({fromOgre(bounds.getMinimum()),
                              fromOgre(bounds.getMaximum()), nodeName});
        }
      }
    } catch (const Ogre::Exception &error) {
      ++stats_.skippedSections;
      Ogre::LogManager::getSingleton().logMessage(
          "Step 8B skipped '" + meshIt->second + "' at " +
          element.source.file.generic_string() + ":" +
          std::to_string(element.source.line) + ": " +
          error.getDescription());
    }
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
    registry_.clear();
    activeRenderables_.clear();
    definition_.reset();
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
      if (binding.node == nullptr || binding.node == rootNode_ ||
          !binding.body.valid()) {
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
  std::optional<content::MapDefinition> definition_;
  std::unordered_set<const content::AuthoredElement *> activeRenderables_;
  EntityRegistry registry_;
  std::uint64_t sequence_{};
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
