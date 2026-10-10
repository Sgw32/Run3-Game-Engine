#include <run3/gameplay/OgreSequenceServices.hpp>

#include <run3/audio/MapAudio.hpp>
#include <run3/audio/SoundRuntime.hpp>
#include <run3/gameplay/DynamicPhysicsScene.hpp>
#include <run3/gameplay/NpcFacialAnimation.hpp>
#include <run3/gameplay/NpcSystem.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/StaticMap.hpp>
#include <run3/rendering/OgreLighting.hpp>
#include <run3/scripting/ScriptEngine.hpp>
#include <run3/ui/Ui.hpp>

#include <OgreAnimation.h>
#include <OgreAnimationState.h>
#include <OgreAnimationTrack.h>
#include <OgreAxisAlignedBox.h>
#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayManager.h>
#include <OgrePass.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreKeyFrame.h>
#include <OgreMesh.h>
#include <OgreParticleSystem.h>
#include <OgrePose.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreScriptCompiler.h>
#include <OgreShaderGenerator.h>
#include <OgreSkeletonInstance.h>
#include <OgreStringConverter.h>
#include <OgreDataStream.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace {

constexpr const char *computerResourceGroup = "Run3Step9A";
constexpr std::string_view facialAnimationPrefix = "Run3/Facial/";
constexpr std::size_t npcShadowCasterBudget = 4;
constexpr Ogre::Real npcShadowDistance = 2000.0F;
constexpr Ogre::Real npcFullRateAnimationDistance = 1000.0F;
constexpr Ogre::Real npcHalfRateAnimationDistance = 2500.0F;

struct FlashlightConfig {
  Ogre::Real innerDegrees{60.0F};
  Ogre::Real outerDegrees{80.0F};
  Ogre::Real range{3000.0F};
  bool allowed{true};
};

FlashlightConfig loadFlashlightConfig(const AppPaths &paths) {
  FlashlightConfig result;
  std::ifstream stream(paths.contentPath("run3/core/player.cfg"));
  std::string line;
  while (std::getline(stream, line)) {
    const auto separator = line.find(':');
    if (separator == std::string::npos) continue;
    const std::string key = line.substr(0, separator);
    const std::string value = line.substr(separator + 1);
    try {
      if (key == "flashConeI") result.innerDegrees = std::stof(value);
      else if (key == "flashConeO") result.outerDegrees = std::stof(value);
      else if (key == "Range") result.range = std::stof(value);
      else if (key == "allowFlashLight")
        result.allowed = value == "true" || value == "1";
    } catch (const std::exception &) {
      Ogre::LogManager::getSingleton().logMessage(
          "Step 8C: invalid flashlight setting '" + line +
          "'; using the previous value");
    }
  }
  result.innerDegrees = std::clamp(result.innerDegrees, 1.0F, 175.0F);
  result.outerDegrees =
      std::clamp(result.outerDegrees, result.innerDegrees, 175.0F);
  result.range = std::clamp(result.range, 1.0F, 100000.0F);
  return result;
}

void ensureComputerResourceGroup() {
  auto &groups = Ogre::ResourceGroupManager::getSingleton();
  if (!groups.resourceGroupExists(computerResourceGroup))
    groups.createResourceGroup(computerResourceGroup);
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

int legacyFacialPoseFromName(std::string name) {
  std::transform(name.begin(), name.end(), name.begin(),
                 [](const unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  name.erase(std::remove_if(name.begin(), name.end(),
                            [](const char character) {
                              return character == '_' || character == '-';
                            }),
             name.end());
  if (name == "sad") return 0;
  if (name == "angry") return 1;
  if (name == "lettera") return 2;
  if (name == "lettere") return 3;
  if (name == "lettero") return 4;
  if (name == "letteru") return 5;
  if (name == "letteri") return 6;
  if (name == "lettersogl1" || name == "lettersogl01") return 7;
  if (name == "lettersogl2" || name == "lettersogl02") return 8;
  return -1;
}

Ogre::AnimationState *findBoundsAnimation(Ogre::Entity &entity,
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

std::optional<Ogre::AxisAlignedBox> animationPoseBounds(
    Ogre::Entity &entity, const std::string &preferred) {
  Ogre::AnimationState *animation = findBoundsAnimation(entity, preferred);
  if (animation == nullptr || !entity.hasSkeleton()) return std::nullopt;

  if (entity.getAllAnimationStates() != nullptr) {
    auto iterator = entity.getAllAnimationStates()->getAnimationStateIterator();
    while (iterator.hasMoreElements()) iterator.getNext()->setEnabled(false);
  }
  animation->setTimePosition(0);
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

Ogre::Vector3 orientedHalfSize(const Ogre::Vector3 &half,
                               const Ogre::Quaternion &orientation) {
  Ogre::Vector3 result = Ogre::Vector3::ZERO;
  for (int x : {-1, 1}) {
    for (int y : {-1, 1}) {
      for (int z : {-1, 1}) {
        const Ogre::Vector3 corner = orientation * Ogre::Vector3{
            half.x * static_cast<Ogre::Real>(x),
            half.y * static_cast<Ogre::Real>(y),
            half.z * static_cast<Ogre::Real>(z)};
        result.makeCeil({std::abs(corner.x), std::abs(corner.y),
                         std::abs(corner.z)});
      }
    }
  }
  return result;
}

physics::BodyType bodyType(RuntimeEntityKind kind) {
  switch (kind) {
  case RuntimeEntityKind::Button: return physics::BodyType::Button;
  case RuntimeEntityKind::Door:
  case RuntimeEntityKind::Rotator:
  case RuntimeEntityKind::Pendulum: return physics::BodyType::Door;
  case RuntimeEntityKind::Train: return physics::BodyType::Train;
  case RuntimeEntityKind::Npc: return physics::BodyType::Npc;
  case RuntimeEntityKind::Computer: return physics::BodyType::PhysicalObject;
  case RuntimeEntityKind::Trigger: return physics::BodyType::Trigger;
  case RuntimeEntityKind::Pickup: return physics::BodyType::Pickup;
  case RuntimeEntityKind::Ladder:
  case RuntimeEntityKind::DarkZone: return physics::BodyType::Unknown;
  }
  return physics::BodyType::Unknown;
}

physics::CollisionGroup collisionGroup(RuntimeEntityKind kind) {
  switch (kind) {
  case RuntimeEntityKind::Button: return physics::CollisionGroup::Button;
  case RuntimeEntityKind::Door:
  case RuntimeEntityKind::Rotator:
  case RuntimeEntityKind::Pendulum: return physics::CollisionGroup::Door;
  case RuntimeEntityKind::Train: return physics::CollisionGroup::Train;
  case RuntimeEntityKind::Trigger: return physics::CollisionGroup::Trigger;
  case RuntimeEntityKind::Pickup: return physics::CollisionGroup::Pickup;
  case RuntimeEntityKind::Npc: return physics::CollisionGroup::Npc;
  case RuntimeEntityKind::Computer: return physics::CollisionGroup::Button;
  case RuntimeEntityKind::Ladder:
  case RuntimeEntityKind::DarkZone: return physics::CollisionGroup::None;
  }
  return physics::CollisionGroup::None;
}

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace

class OgreSequenceServices::Impl final {
public:
  struct Attachment {
    Ogre::Entity *object{};
    Ogre::SceneNode *originalNode{};
    std::string objectName;
  };
  struct Presentation {
    struct Part {
      RuntimeVisualPartSpec spec;
      Ogre::SceneNode *node{};
      Ogre::Entity *entity{};
      Ogre::Vector3 localCentre{Ogre::Vector3::ZERO};
      std::optional<PhysicsEntityId> physicsEntity;
    };
    RuntimeEntitySpec spec;
    Ogre::SceneNode *node{};
    Ogre::SceneNode *visualNode{};
    Ogre::Entity *entity{};
    Ogre::Vector3 localCentre{Ogre::Vector3::ZERO};
    physics::Vec3 collisionHalfExtents{1.0, 1.0, 1.0};
    std::optional<PhysicsEntityId> physicsEntity;
    std::vector<Part> parts;
    double pendingAnimationSeconds{};
  };
  struct ParticlePresentation {
    std::string name;
    std::uint64_t owner{};
    Ogre::SceneNode *node{};
    Ogre::ParticleSystem *system{};
  };
  struct FacialPresentation {
    FacialAnimationDefinition definition;
    std::string ownerName;
    physics::Vec3 position;
    Ogre::MeshPtr mesh;
    Ogre::Entity *entity{};
    std::string animationName;
    Ogre::AnimationState *state{};
    std::unordered_map<unsigned short, Ogre::VertexPoseKeyFrame *> keyframes;
    std::unordered_map<int, std::vector<unsigned short>> namedPoses;
    double leadInElapsed{};
    double nextProgressLog{};
    bool voiceStarted{};
    bool active{};
    bool softwareAnimationRequested{};
  };

  Impl(const AppPaths &appPaths, Ogre::SceneManager &manager,
       Ogre::Camera &gameCamera,
       physics::PhysicsWorld &world, StaticMap &loadedMap,
       PlayerController &playerController,
       audio::IAudioEngine &audioEngine, ui::IUiSystem &uiSystem,
       MapChangeRequest changeRequest,
       double lodBias, double configuredFovDegrees)
      : paths(appPaths), sceneManager(&manager), camera(&gameCamera),
        physicsWorld(&world),
        staticMap(&loadedMap),
        player(&playerController),
        audio(&audioEngine), ui(&uiSystem), oneShots(audioEngine), dynamicPhysics(world),
        mapChangeRequest(std::move(changeRequest)),
        scripts({paths.contentRoot(), paths.userRoot(), 1'000'000, &uiSystem,
                 &uiSystem.buttonGui()},
                [this](const scripting::ScriptCall &call) {
                  if (runtime == nullptr) {
                    throw std::runtime_error(
                        "Script command arrived before SequenceRuntime attach");
                  }
                  return runtime->dispatchScriptCall(call);
                }),
        flashlightConfig(loadFlashlightConfig(paths)),
        flashlightAllowed(flashlightConfig.allowed), meshLodBias(lodBias),
        defaultFovDegrees(configuredFovDegrees) {
    root = sceneManager->getRootSceneNode()->createChildSceneNode(
        "Run3Step8CSequenceRoot");
    setHudVisible(true);
  }

  ~Impl() { destroyAll(); }

  void log(const std::string &message) const {
    Ogre::LogManager::getSingleton().logMessage("Step 8C: " + message);
  }

  void destroyFlashlight() noexcept {
    if ((flashlight == nullptr && flashlightNode == nullptr) ||
        sceneManager == nullptr) return;
    try {
      if (flashlight != nullptr) {
        flashlight->detachFromParent();
        sceneManager->destroyLight(flashlight);
      }
      if (flashlightNode != nullptr)
        sceneManager->destroySceneNode(flashlightNode);
    } catch (...) {}
    flashlight = nullptr;
    flashlightNode = nullptr;
  }

  void setFlashlightAllowed(const bool allowed) {
    flashlightAllowed = allowed;
    if (!allowed) destroyFlashlight();
    log(std::string("flashlight ") + (allowed ? "allowed" : "blocked"));
  }

  void toggleFlashlight() {
    if (!flashlightAllowed) {
      log("flashlight toggle ignored while blocked by the map");
      return;
    }
    static_cast<void>(oneShots.emit(
        paths.contentPath("run3/sounds/flash01.wav"), 2.0F, false,
        audio::Bus::effects));
    if (flashlight != nullptr) {
      destroyFlashlight();
      log("player flashlight off");
      return;
    }
    Ogre::SceneNode *viewNode = camera->getParentSceneNode();
    if (viewNode == nullptr)
      throw std::logic_error("player camera has no scene node for flashlight");
    flashlight = sceneManager->createLight("Run3PlayerFlashlight");
    flashlight->setType(Ogre::Light::LT_SPOTLIGHT);
    flashlight->setDiffuseColour(0.5F, 0.5F, 0.5F);
    flashlight->setSpecularColour(0.0F, 0.0F, 0.0F);
    flashlight->setSpotlightRange(Ogre::Degree(flashlightConfig.innerDegrees),
                                  Ogre::Degree(flashlightConfig.outerDegrees));
    // The legacy shader ignored the linear/quadratic coefficients. Constant
    // attenuation retains its authored range in Ogre's modern light path.
    flashlight->setAttenuation(flashlightConfig.range, 1.0F, 0.0F, 0.0F);
    flashlight->setCastShadows(true);
    flashlightNode = viewNode->createChildSceneNode(
        "Run3PlayerFlashlightNode", Ogre::Vector3(0.0F, 0.0F, -20.0F));
    flashlightNode->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Z,
                                 Ogre::Node::TS_PARENT);
    flashlightNode->attachObject(flashlight);
    log("player flashlight on");
  }

  Presentation &require(EntityHandle handle) {
    const auto found = presentations.find(handle.id.value);
    if (found == presentations.end() ||
        found->second.spec.handle.generation != handle.generation) {
      throw std::runtime_error("stale or unknown runtime entity handle " +
                               std::to_string(handle.id.value));
    }
    return found->second;
  }

  void resolveParents() {
    for (auto &[id, presentation] : presentations) {
      static_cast<void>(id);
      if (presentation.spec.parent.empty() || presentation.node == nullptr) {
        continue;
      }
      const auto parentName = names.find(presentation.spec.parent);
      if (parentName == names.end()) continue;
      Presentation &parent = presentations.at(parentName->second);
      if (parent.node == nullptr || presentation.node->getParent() == parent.node) {
        continue;
      }
      Ogre::Node *oldParent = presentation.node->getParent();
      if (oldParent != nullptr) oldParent->removeChild(presentation.node);
      parent.node->addChild(presentation.node);
      presentation.node->setPosition(toOgre(presentation.spec.transform.position));
      presentation.node->setOrientation(toOgre(presentation.spec.transform.rotation));
      syncPresentationBody(presentation);
    }
  }

  void createPhysics(Presentation &presentation, physics::Vec3 halfExtents) {
    if (!presentation.spec.collision ||
        collisionGroup(presentation.spec.kind) == physics::CollisionGroup::None) {
      return;
    }
    halfExtents.x = std::max(0.01, std::abs(halfExtents.x));
    halfExtents.y = std::max(0.01, std::abs(halfExtents.y));
    halfExtents.z = std::max(0.01, std::abs(halfExtents.z));
    DynamicEntityDesc body{presentation.spec.name,
                           bodyType(presentation.spec.kind),
                           physics::Shape::box(halfExtents)};
    if (presentation.spec.kind == RuntimeEntityKind::Npc) {
      body.motion = physics::BodyMotion::Dynamic;
      body.massKg = 80.0;
      body.angularFactor = {0.0, 0.0, 0.0};
      body.gravityEnabled = presentation.spec.gravityEnabled;
    } else {
      body.motion = physics::BodyMotion::Kinematic;
    }
    body.transform = presentation.spec.transform;
    if (presentation.node != nullptr) {
      presentation.node->_update(true, true);
      body.transform.position = fromOgre(
          presentation.node->_getDerivedPosition() +
          presentation.node->_getDerivedOrientation() *
              presentation.localCentre);
      body.transform.rotation = fromOgre(presentation.node->_getDerivedOrientation());
      if (presentation.spec.kind == RuntimeEntityKind::Npc)
        body.transform = presentation.spec.transform;
    }
    body.group = collisionGroup(presentation.spec.kind);
    body.mask = physics::collisionMask(physics::CollisionGroup::Player);
    if (presentation.spec.kind == RuntimeEntityKind::Npc)
      body.mask |= physics::collisionMask(physics::CollisionGroup::World) |
                   physics::collisionMask(physics::CollisionGroup::Dynamic) |
                   physics::collisionMask(physics::CollisionGroup::Door);
    if (presentation.spec.kind == RuntimeEntityKind::Door ||
        presentation.spec.kind == RuntimeEntityKind::Rotator ||
        presentation.spec.kind == RuntimeEntityKind::Pendulum ||
        presentation.spec.kind == RuntimeEntityKind::Train) {
      body.mask |= physics::collisionMask(physics::CollisionGroup::Dynamic) |
                   physics::collisionMask(physics::CollisionGroup::Npc);
    }
    body.trigger = presentation.spec.kind == RuntimeEntityKind::Trigger ||
                   presentation.spec.kind == RuntimeEntityKind::Pickup ||
                   presentation.spec.kind == RuntimeEntityKind::Button;
    presentation.physicsEntity = dynamicPhysics.createEntity(std::move(body));
    physicsHandles[*presentation.physicsEntity] = presentation.spec.handle;
  }

  void syncNpcBodiesFromPhysics() {
    for (auto &[id, presentation] : presentations) {
      static_cast<void>(id);
      if (presentation.spec.kind != RuntimeEntityKind::Npc ||
          !presentation.physicsEntity || presentation.node == nullptr)
        continue;
      const physics::Transform body =
          dynamicPhysics.transform(*presentation.physicsEntity);
      presentation.spec.transform.position = body.position;
      // NPC angular motion is locked. Rotation continues to be authored by
      // navigation/scripts while Bullet owns the vertical position.
      presentation.node->setPosition(toOgre(body.position));
      presentation.node->setOrientation(
          toOgre(presentation.spec.transform.rotation));
    }
  }

  bool setNamedBodyFrozen(const std::string_view name, const bool frozen) {
    if (staticMap->setNamedObjectFrozen(name, frozen)) return true;
    const auto named = names.find(std::string(name));
    if (named != names.end()) {
      Presentation &presentation = presentations.at(named->second);
      if (presentation.physicsEntity) {
        try {
          dynamicPhysics.setEntityFrozen(*presentation.physicsEntity, frozen);
          return true;
        } catch (const std::invalid_argument &) {
        }
      }
    }
    for (auto &[id, presentation] : presentations) {
      static_cast<void>(id);
      for (Presentation::Part &part : presentation.parts) {
        if (part.spec.name != name || !part.physicsEntity) continue;
        try {
          dynamicPhysics.setEntityFrozen(*part.physicsEntity, frozen);
          return true;
        } catch (const std::invalid_argument &) {
          return false;
        }
      }
    }
    return false;
  }

  void createPartPhysics(Presentation &owner, Presentation::Part &part) {
    if (!part.spec.collision || part.entity == nullptr || part.node == nullptr)
      return;
    const Ogre::Vector3 meshHalf = part.entity->getBoundingBox().getHalfSize();
    const Ogre::Vector3 derivedScale = part.node->_getDerivedScale();
    physics::Vec3 half{
        std::max(0.01, static_cast<double>(
                           std::abs(meshHalf.x * derivedScale.x))),
        std::max(0.01, static_cast<double>(
                           std::abs(meshHalf.y * derivedScale.y))),
        std::max(0.01, static_cast<double>(
                           std::abs(meshHalf.z * derivedScale.z)))};
    DynamicEntityDesc body{
        part.spec.name.empty() ? owner.spec.name : part.spec.name,
        physics::BodyType::Train, physics::Shape::box(half)};
    body.motion = physics::BodyMotion::Kinematic;
    part.node->_update(true, true);
    body.transform.position = fromOgre(
        part.node->_getDerivedPosition() +
        part.node->_getDerivedOrientation() * part.localCentre);
    body.transform.rotation = fromOgre(part.node->_getDerivedOrientation());
    body.group = physics::CollisionGroup::Train;
    body.mask = physics::collisionMask(physics::CollisionGroup::Player) |
                physics::collisionMask(physics::CollisionGroup::Dynamic) |
                physics::collisionMask(physics::CollisionGroup::Npc);
    part.physicsEntity = dynamicPhysics.createEntity(std::move(body));
    physicsHandles[*part.physicsEntity] = owner.spec.handle;
  }

  void syncPartBody(Presentation::Part &part) {
    if (!part.physicsEntity || part.node == nullptr) return;
    part.node->_update(true, true);
    physics::Transform worldTransform;
    worldTransform.position = fromOgre(
        part.node->_getDerivedPosition() +
        part.node->_getDerivedOrientation() * part.localCentre);
    worldTransform.rotation = fromOgre(part.node->_getDerivedOrientation());
    dynamicPhysics.setEntityTransform(*part.physicsEntity, worldTransform);
  }

  void createParticle(const RuntimeParticleSpec &spec, Ogre::SceneNode &parent,
                      std::uint64_t owner) {
    if (spec.name.empty() || spec.templateName.empty()) return;
    if (particles.count(spec.name) != 0) {
      log("warning: duplicate particle name '" + spec.name + "' skipped");
      return;
    }
    try {
      const std::string objectName = "Run3Step8CParticle/" +
          std::to_string(particleSequence++) + "/" + spec.name;
      Ogre::ParticleSystem *system = sceneManager->createParticleSystem(
          objectName, spec.templateName);
      staticMap->prepareParticleSystem(*system);
      Ogre::SceneNode *node = parent.createChildSceneNode(
          objectName + "/Node", toOgre(spec.position));
      node->setScale(toOgre(spec.scale));
      node->attachObject(system);
      system->setVisible(spec.visible);
      particles.emplace(spec.name,
                        ParticlePresentation{spec.name, owner, node, system});
    } catch (const Ogre::Exception &error) {
      // The old loader caught missing/broken templates and continued the map.
      log("warning: particle '" + spec.name + "' template '" +
          spec.templateName + "' skipped: " + error.getDescription());
    }
  }

  void destroyParticle(std::string_view name) noexcept {
    const auto found = particles.find(std::string(name));
    if (found == particles.end()) return;
    try {
      if (found->second.system != nullptr)
        sceneManager->destroyParticleSystem(found->second.system);
      if (found->second.node != nullptr)
        sceneManager->destroySceneNode(found->second.node);
    } catch (...) {}
    particles.erase(found);
  }

  void spawn(const RuntimeEntitySpec &spec) {
    if (presentations.count(spec.handle.id.value) != 0) {
      throw std::runtime_error("duplicate runtime presentation handle");
    }
    Presentation presentation;
    presentation.spec = spec;
    physics::Vec3 half = spec.halfExtents;
    const auto authoredButton = spec.kind == RuntimeEntityKind::Button
        ? staticMap->namedObjectBounds(spec.name) : std::nullopt;
    if (spec.kind == RuntimeEntityKind::Button && !authoredButton) {
      log("button '" + spec.name +
          "' has no presented scene object; using authored sequence mesh");
    }
    if (authoredButton) {
      presentation.spec.transform.position = authoredButton->centre;
      presentation.spec.transform.rotation = {};
      half = authoredButton->halfExtents;
    } else if (!spec.mesh.empty()) {
      const std::string objectName =
          "Run3Step8CEntity/" + std::to_string(spec.handle.id.value);
      try {
        presentation.entity = sceneManager->createEntity(
            objectName, spec.mesh,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
      } catch (const Ogre::Exception &error) {
        throw std::runtime_error("entity '" + spec.name + "' mesh '" + spec.mesh +
                                 "': " + error.getDescription());
      }
      // Sequence-spawned meshes use the same compatibility material path as
      // static map sections. Legacy materials are not valid RTSS materials on
      // modern D3D11/GL3+, which previously left doors/trains/buttons white.
      staticMap->applyCompatibleMaterials(*presentation.entity, spec.material);
      presentation.entity->setMeshLodBias(
          static_cast<Ogre::Real>(meshLodBias));
      presentation.entity->setVisible(spec.visible);
      presentation.entity->setCastShadows(spec.castShadows);
      presentation.node = root->createChildSceneNode(
          "Run3Step8CNode/" + std::to_string(spec.handle.id.value));
      presentation.node->setPosition(toOgre(spec.transform.position));
      presentation.node->setOrientation(toOgre(spec.transform.rotation));
      if (spec.kind == RuntimeEntityKind::Npc) {
        presentation.visualNode = presentation.node->createChildSceneNode(
            "Run3Step8DVisual/" + std::to_string(spec.handle.id.value));
        presentation.visualNode->setPosition(toOgre(spec.visualOffset));
        const Ogre::Vector3 axis = toOgre(spec.visualRotationAxis);
        if (!axis.isZeroLength() && spec.visualRotationDegrees != 0.0) {
          presentation.visualNode->rotate(
              axis.normalisedCopy(),
              Ogre::Degree(static_cast<Ogre::Real>(
                  spec.visualRotationDegrees)),
              Ogre::Node::TS_LOCAL);
        }
        presentation.visualNode->yaw(Ogre::Degree(static_cast<Ogre::Real>(
            spec.visualYawDegrees)), Ogre::Node::TS_LOCAL);
        presentation.entity->setRenderingDistance(static_cast<Ogre::Real>(
            spec.renderDistance));
      }
      presentation.node->setScale(toOgre(spec.scale));
      (presentation.visualNode != nullptr ? presentation.visualNode
                                          : presentation.node)
          ->attachObject(presentation.entity);
      const Ogre::AxisAlignedBox meshBounds =
          presentation.entity->getBoundingBox();
      const Ogre::Vector3 meshHalf = meshBounds.getHalfSize();
      if (spec.kind == RuntimeEntityKind::Npc) {
        bool fittedFromAnimation = false;
        if (spec.autoPosition) {
          if (const auto posedBounds = animationPoseBounds(
                  *presentation.entity, spec.autoPositionAnimation)) {
            const Ogre::Vector3 posedCentre = posedBounds->getCenter();
            const Ogre::Vector3 posedHalf = orientedHalfSize(
                posedBounds->getHalfSize(),
                presentation.visualNode->getOrientation());
            const Ogre::Vector3 offset =
                -(presentation.visualNode->getOrientation() * posedCentre);
            presentation.visualNode->setPosition(offset);
            presentation.spec.visualOffset = fromOgre(offset);
            half = {
                std::abs(posedHalf.x * spec.scale.x *
                         spec.autoPositionCorrection.x),
                std::abs(posedHalf.y * spec.scale.y *
                         spec.autoPositionCorrection.y),
                std::abs(posedHalf.z * spec.scale.z *
                         spec.autoPositionCorrection.z)};
            fittedFromAnimation = true;
            log("NPC '" + spec.name +
                "' auto-fitted from walking pose: offset " +
                Ogre::StringConverter::toString(offset) +
                ", half extents " +
                Ogre::StringConverter::toString(toOgre(half)));
          } else {
            log("warning: NPC '" + spec.name +
                "' requested autoPosition but has no usable walking skeletal "
                "animation; retaining physPosit");
          }
        }
        if (!fittedFromAnimation) {
          half = {std::abs(meshHalf.x * spec.scale.x * spec.collisionScale.x),
                  std::abs(meshHalf.y * spec.scale.y * spec.collisionScale.y),
                  std::abs(meshHalf.z * spec.scale.z * spec.collisionScale.z)};
        }
      } else {
        half = {std::abs(meshHalf.x * spec.scale.x),
                std::abs(meshHalf.y * spec.scale.y),
                std::abs(meshHalf.z * spec.scale.z)};
      }
      presentation.localCentre = meshBounds.getCenter() * toOgre(spec.scale);
    }
    presentations.emplace(spec.handle.id.value, std::move(presentation));
    Presentation &stored = presentations.at(spec.handle.id.value);
    if (stored.node != nullptr) {
      for (const RuntimeVisualPartSpec &partSpec : spec.parts) {
        if (partSpec.mesh.empty()) continue;
        Presentation::Part part;
        part.spec = partSpec;
        try {
          const std::string suffix = std::to_string(stored.parts.size());
          part.entity = sceneManager->createEntity(
              "Run3Step8CPart/" + std::to_string(spec.handle.id.value) + "/" +
                  suffix,
              partSpec.mesh,
              Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
          staticMap->applyCompatibleMaterials(*part.entity, partSpec.material);
          part.entity->setMeshLodBias(static_cast<Ogre::Real>(meshLodBias));
          part.node = stored.node->createChildSceneNode(
              "Run3Step8CPartNode/" + std::to_string(spec.handle.id.value) +
                  "/" + suffix,
              toOgre(partSpec.transform.position),
              toOgre(partSpec.transform.rotation));
          part.node->setScale(toOgre(partSpec.scale));
          part.node->attachObject(part.entity);
          part.node->_update(true, true);
          part.localCentre = part.entity->getBoundingBox().getCenter() *
                             part.node->_getDerivedScale();
          if (!partSpec.name.empty())
            namedPartEntities.insert_or_assign(partSpec.name, part.entity);
          stored.parts.push_back(std::move(part));
          createPartPhysics(stored, stored.parts.back());
        } catch (const Ogre::Exception &error) {
          if (part.entity != nullptr) sceneManager->destroyEntity(part.entity);
          log("warning: train part '" + partSpec.name + "' mesh '" +
              partSpec.mesh + "' skipped: " + error.getDescription());
        }
      }
      for (const RuntimeParticleSpec &particle : spec.particles)
        createParticle(particle, *stored.node, spec.handle.id.value);
    }
    const auto previous = names.find(spec.name);
    if (previous == names.end() ||
        (presentations.at(previous->second).node == nullptr &&
         presentations.at(spec.handle.id.value).node != nullptr)) {
      names.insert_or_assign(spec.name, spec.handle.id.value);
    }
    resolveParents();
    presentations.at(spec.handle.id.value).collisionHalfExtents = half;
    createPhysics(presentations.at(spec.handle.id.value), half);
  }

  void setTransform(const SetRuntimeTransform &command) {
    Presentation &presentation = require(command.handle);
    presentation.spec.transform = command.transform;
    if (presentation.node != nullptr) {
      presentation.node->setPosition(toOgre(command.transform.position));
      presentation.node->setOrientation(toOgre(command.transform.rotation));
    }
    syncPresentationBody(presentation);
    for (Presentation::Part &part : presentation.parts) syncPartBody(part);
    // Parent motion also moves authored children. Keep each child collision
    // body in the same Bullet world as the visual hierarchy.
    if (presentation.spec.kind != RuntimeEntityKind::Npc) {
      for (auto &[id, child] : presentations) {
        static_cast<void>(id);
        if (child.node != nullptr && child.node != presentation.node &&
            presentation.node != nullptr &&
            child.node->isInSceneGraph() &&
            child.node->getParent() == presentation.node) {
          syncPresentationBody(child);
        }
      }
    }
  }

  void syncPresentationBody(Presentation &presentation) {
    if (presentation.physicsEntity) {
      physics::Transform worldTransform = presentation.spec.transform;
      if (presentation.node != nullptr &&
          presentation.spec.kind != RuntimeEntityKind::Npc) {
        presentation.node->_update(true, true);
        worldTransform.position = fromOgre(
            presentation.node->_getDerivedPosition() +
            presentation.node->_getDerivedOrientation() *
                presentation.localCentre);
        worldTransform.rotation = fromOgre(presentation.node->_getDerivedOrientation());
      }
      dynamicPhysics.setEntityTransform(*presentation.physicsEntity,
                                        worldTransform);
    }
  }

  void playSound(const PlayRuntimeSound &command) {
    if (command.path.empty() || command.path == "none" ||
        command.path == "nosound") return;
    audio::PlayOptions options;
    options.file = paths.contentPath(command.path);
    if (command.owner.id.value == 0 && mapAudio != nullptr) {
      if (!mapAudio->playMusic(options.file, command.loop)) {
        throw std::runtime_error("cannot play music '" + command.path.string() +
                                 "': " + audio->lastError());
      }
      return;
    }
    options.bus = command.owner.id.value == 0 ? audio::Bus::music
                                               : audio::Bus::effects;
    options.loop = command.loop;
    options.streaming = options.bus == audio::Bus::music;
    options.spatial = command.owner.id.value != 0;
    options.position = {static_cast<float>(command.position.x),
                        static_cast<float>(command.position.y),
                        static_cast<float>(command.position.z)};
    options.gain = command.gain;
    audio::SoundHandle sound = audio->play(options);
    if (!sound.valid()) {
      throw std::runtime_error("audio failed for '" + command.path.string() +
                               "': " + audio->lastError());
    }
    if (command.owner.id.value == 0) music = std::move(sound);
    else sounds.insert_or_assign(command.owner.id.value, std::move(sound));
  }

  void setComputerPresentation(const SetComputerPresentation &command) {
    Presentation &entry = require(command.owner);
    const std::string ownerKey = entry.spec.name + "#" +
                                 std::to_string(command.owner.id.value) + ":" +
                                 std::to_string(command.owner.generation);
    if (!command.focused) {
      ui->deactivateComputer(ownerKey);
      restoreComputerMaterial(command.owner.id.value);
      log("computer '" + entry.spec.name + "' released");
      return;
    }
    const bool virtualSurface = command.allowVirtualDisplay &&
                                staticMap->definition().quality == "high";
    const std::string textureName =
        ui->activateComputer(ownerKey, virtualSurface);
    if (virtualSurface)
      bindComputerMaterial(entry, command.owner.id.value, command.material,
                           textureName);
    else
      ui->setComputerDisplayMaterial(command.material);
    log("computer '" + entry.spec.name +
        (virtualSurface ? "' focused on computer RTT '" + textureName + "'"
                        : "' focused in direct-display mode"));
  }

  void sendComputerInput(const SendComputerInput &command) {
    const Presentation &entry = require(command.owner);
    log("computer input for '" + entry.spec.name + "': key=" +
        std::to_string(command.key) +
        (command.text.empty() ? "" : " text=" + command.text));
  }

  struct ComputerMaterialBinding {
    struct Item { Ogre::SubEntity *subEntity{}; std::string material; };
    std::vector<Item> items;
    std::string generatedMaterial;
  };

  void bindComputerMaterial(Presentation &entry, const std::uint64_t id,
                            const std::string &authoredMaterial,
                            const std::string &textureName) {
    restoreComputerMaterial(id);
    ensureComputerResourceGroup();
    const std::string materialName = "Run3/ComputerSurface/" + std::to_string(id);
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(
        materialName, computerResourceGroup);
    if (!material) {
      material = Ogre::MaterialManager::getSingleton().create(
          materialName, computerResourceGroup);
    } else {
      if (auto *generator = Ogre::RTShader::ShaderGenerator::getSingletonPtr())
        generator->removeAllShaderBasedTechniques(*material);
      material->removeAllTechniques();
    }
    Ogre::Pass *pass = material->createTechnique()->createPass();
    pass->setLightingEnabled(false);
    pass->setDepthCheckEnabled(true);
    pass->setDepthWriteEnabled(true);
    pass->setCullingMode(Ogre::CULL_NONE);
    pass->createTextureUnitState(textureName);
    material->load();

    ComputerMaterialBinding binding;
    binding.generatedMaterial = materialName;
    const auto visit = [&](Ogre::Entity *entity) {
      if (entity == nullptr) return;
      for (unsigned index = 0; index < entity->getNumSubEntities(); ++index) {
        Ogre::SubEntity *sub = entity->getSubEntity(index);
        if (authoredMaterial.empty() ||
            sub->getMaterialName() == authoredMaterial) {
          binding.items.push_back({sub, sub->getMaterialName()});
          sub->setMaterial(material);
        }
      }
    };
    visit(entry.entity);
    for (auto &part : entry.parts) visit(part.entity);
    if (binding.items.empty()) {
      log("warning: computer '" + entry.spec.name +
          "' has no renderable screen for material '" + authoredMaterial + "'");
      Ogre::MaterialManager::getSingleton().remove(materialName,
                                                   computerResourceGroup);
      return;
    }
    computerMaterialBindings.insert_or_assign(id, std::move(binding));
  }

  void restoreComputerMaterial(const std::uint64_t id) noexcept {
    const auto found = computerMaterialBindings.find(id);
    if (found == computerMaterialBindings.end()) return;
    for (const ComputerMaterialBinding::Item &item : found->second.items) {
      try {
        if (item.subEntity != nullptr) item.subEntity->setMaterialName(item.material);
      } catch (...) {}
    }
    try {
      Ogre::MaterialManager::getSingleton().remove(
          found->second.generatedMaterial, computerResourceGroup);
    } catch (...) {}
    computerMaterialBindings.erase(found);
  }

  void setEffectEnabled(const SetRuntimeEffectEnabled &command) {
    const bool enabled = command.enabled.value_or(!effectStates[command.name]);
    effectStates[command.name] = enabled;
    const auto particle = particles.find(command.name);
    if (particle != particles.end() && particle->second.system != nullptr) {
      particle->second.system->setVisible(enabled);
      return;
    }
    if (sceneManager->hasParticleSystem(command.name)) {
      sceneManager->getParticleSystem(command.name)->setVisible(enabled);
      return;
    }
    log("warning: effect controller '" + command.name +
        "' has no loaded particle presentation; state retained");
  }

  void parseHudScript(const std::filesystem::path &relative) {
    const std::filesystem::path path = paths.contentPath(relative);
    std::ifstream source(path, std::ios::binary);
    if (!source) {
      throw std::runtime_error("missing legacy HUD resource '" +
                               path.string() + "'");
    }
    Ogre::DataStreamPtr data(OGRE_NEW Ogre::FileStreamDataStream(
        path.filename().string(), &source, false));
    Ogre::ScriptCompilerManager::getSingleton().parseScript(
        data, "Run3Step6BContent");
  }

  void setHudVisible(const bool visible) {
    ui->setHudVisible(visible);
    try {
      if (crosshairOverlay == nullptr) {
        if (Ogre::OverlayManager::getSingleton().getByName(
                "Run3/CrosshairO") == nullptr) {
          if (!Ogre::MaterialManager::getSingleton().resourceExists(
                  "Run3/Crosshair", "Run3Step6BContent") ||
              !Ogre::MaterialManager::getSingleton().resourceExists(
                  "Run3/CrosshairRing", "Run3Step6BContent")) {
            parseHudScript("run3/game/player/crosshair.material");
          }
          parseHudScript("run3/game/player/crosshair.overlay");
        }
        crosshairOverlay = Ogre::OverlayManager::getSingleton().getByName(
            "Run3/CrosshairO");
      }
      if (crosshairOverlay == nullptr) {
        throw std::runtime_error(
            "legacy Run3/CrosshairO overlay was not created");
      }
      if (visible) crosshairOverlay->show();
      else crosshairOverlay->hide();
    } catch (const std::exception &error) {
      log("warning: crosshair overlay unavailable: " +
          std::string(error.what()));
    }
  }

  void setGameText(const std::string &text, const double seconds) {
    try {
      if (gameTextOverlay == nullptr) {
        if (Ogre::OverlayManager::getSingleton().getByName("Run3/GameText") ==
            nullptr) {
          parseHudScript("run3/fonts/console2.fontdef");
          parseHudScript("run3/game/message/game_text.overlay");
        }
        gameTextOverlay =
            Ogre::OverlayManager::getSingleton().getByName("Run3/GameText");
        gameTextElement = Ogre::OverlayManager::getSingleton().getOverlayElement(
            "Run3/TextArea", false);
      }
      if (gameTextOverlay == nullptr || gameTextElement == nullptr) {
        throw std::runtime_error("legacy Run3/GameText overlay was not created");
      }
      gameTextElement->setCaption(text);
      gameTextSeconds = std::max(0.0, seconds);
      if (text.empty()) gameTextOverlay->hide();
      else gameTextOverlay->show();
    } catch (const std::exception &error) {
      log("warning: gameText overlay unavailable: " +
          std::string(error.what()));
      gameTextSeconds = 0.0;
    }
  }

  void updateGameText(const float seconds) {
    if (gameTextOverlay == nullptr || gameTextSeconds <= 0.0) return;
    gameTextSeconds -= seconds;
    if (gameTextSeconds <= 0.0) gameTextOverlay->hide();
  }

  void destroyAll() noexcept {
    destroyFlashlight();
    if (ui != nullptr) ui->resetMapState();
    if (gameTextOverlay != nullptr) {
      try { gameTextOverlay->hide(); } catch (...) {}
    }
    if (crosshairOverlay != nullptr) {
      try { crosshairOverlay->hide(); } catch (...) {}
    }
    gameTextSeconds = 0.0;
    while (!computerMaterialBindings.empty())
      restoreComputerMaterial(computerMaterialBindings.begin()->first);
    for (auto &[id, attached] : attachments) {
      auto owner = presentations.find(id);
      for (auto &item : attached) {
        try {
          if (owner != presentations.end() && owner->second.entity)
            owner->second.entity->detachObjectFromBone(item.object);
          if (item.originalNode) item.originalNode->attachObject(item.object);
          static_cast<void>(staticMap->setNamedObjectPhysicsEnabled(
              item.objectName, true));
        } catch (...) {}
      }
    }
    attachments.clear();
    sounds.clear();
    std::vector<std::pair<Ogre::MeshPtr, std::string>> facialAnimations;
    facialAnimations.reserve(facials.size());
    for (auto &[id, facial] : facials) {
      static_cast<void>(id);
      try { resetFacialPose(facial); } catch (...) {}
      if (facial.mesh && !facial.animationName.empty())
        facialAnimations.emplace_back(facial.mesh, facial.animationName);
    }
    facials.clear();
    voices.clear();
    ragdolls.clear();
    music.reset();
    oneShots.clear();
    while (!particles.empty()) destroyParticle(particles.begin()->first);
    dynamicPhysics.unload();
    physicsHandles.clear();
    names.clear();
    namedPartEntities.clear();
    if (sceneManager != nullptr) {
      for (auto &[id, presentation] : presentations) {
        static_cast<void>(id);
        if (presentation.entity != nullptr) {
          try { sceneManager->destroyEntity(presentation.entity); }
          catch (...) {}
        }
        for (auto &part : presentation.parts) {
          if (part.entity != nullptr) {
            try { sceneManager->destroyEntity(part.entity); }
            catch (...) {}
          }
        }
      }
    }
    presentations.clear();
    for (auto &[mesh, animation] : facialAnimations) {
      try {
        if (mesh && mesh->hasAnimation(animation))
          mesh->removeAnimation(animation);
      } catch (...) {}
    }
    if (sceneManager != nullptr && root != nullptr) {
      try {
        root->removeAndDestroyAllChildren();
        sceneManager->destroySceneNode(root);
      } catch (...) {
      }
      root = nullptr;
    }
  }

  void destroy(EntityHandle handle) {
    const auto found = presentations.find(handle.id.value);
    // Bulk map teardown removes every presentation before individual map-owned
    // systems release their handles. Treat that second release as idempotent.
    if (found == presentations.end()) return;
    Presentation &entry = found->second;
    Ogre::MeshPtr facialMesh;
    std::string facialAnimation;
    const auto facial = facials.find(handle.id.value);
    if (facial != facials.end()) {
      resetFacialPose(facial->second);
      facialMesh = facial->second.mesh;
      facialAnimation = facial->second.animationName;
      facials.erase(facial);
    }
    auto attached = attachments.find(handle.id.value);
    if (attached != attachments.end()) {
      for (auto &item : attached->second) {
        if (entry.entity) entry.entity->detachObjectFromBone(item.object);
        if (item.originalNode) item.originalNode->attachObject(item.object);
        static_cast<void>(staticMap->setNamedObjectPhysicsEnabled(
            item.objectName, true));
      }
      attachments.erase(attached);
    }
    if (entry.physicsEntity) {
      physicsHandles.erase(*entry.physicsEntity);
      dynamicPhysics.destroyEntity(*entry.physicsEntity);
    }
    for (auto &part : entry.parts) {
      if (part.physicsEntity) {
        physicsHandles.erase(*part.physicsEntity);
        dynamicPhysics.destroyEntity(*part.physicsEntity);
      }
      if (!part.spec.name.empty()) namedPartEntities.erase(part.spec.name);
      if (part.entity) sceneManager->destroyEntity(part.entity);
    }
    std::vector<std::string> ownedParticles;
    for (const auto &[name, particle] : particles)
      if (particle.owner == handle.id.value) ownedParticles.push_back(name);
    for (const std::string &name : ownedParticles) destroyParticle(name);
    sounds.erase(handle.id.value);
    voices.erase(handle.id.value);
    const auto ragdoll = ragdolls.find(handle.id.value);
    if (ragdoll != ragdolls.end()) {
      dynamicPhysics.destroyEntity(ragdoll->second);
      ragdolls.erase(ragdoll);
    }
    names.erase(entry.spec.name);
    if (entry.entity) sceneManager->destroyEntity(entry.entity);
    if (entry.node) {
      entry.node->removeAndDestroyAllChildren();
      sceneManager->destroySceneNode(entry.node);
    }
    presentations.erase(handle.id.value);
    if (facialMesh && !facialAnimation.empty() &&
        facialMesh->hasAnimation(facialAnimation)) {
      facialMesh->removeAnimation(facialAnimation);
      for (auto &[id, presentation] : presentations) {
        static_cast<void>(id);
        if (presentation.entity != nullptr &&
            presentation.entity->getMesh() == facialMesh)
          presentation.entity->refreshAvailableAnimationState();
      }
    }
  }

  void setNpcAttachment(const SetNpcAttachment &command) {
    Presentation &owner = require(command.owner);
    if (owner.spec.kind != RuntimeEntityKind::Npc || owner.entity == nullptr)
      throw std::invalid_argument("attachment owner is not a presented NPC");
    auto &owned = attachments[command.owner.id.value];
    const auto previous = std::find_if(owned.begin(), owned.end(),
        [&command](const Attachment &item) {
          return item.objectName == command.object;
        });
    if (!command.attach) {
      if (previous == owned.end()) return;
      owner.entity->detachObjectFromBone(previous->object);
      if (previous->originalNode) previous->originalNode->attachObject(previous->object);
      static_cast<void>(staticMap->setNamedObjectPhysicsEnabled(
          previous->objectName, true));
      owned.erase(previous);
      return;
    }
    if (previous != owned.end()) return;
    Ogre::Entity *object = staticMap->namedObject(command.object);
    if (object == nullptr) {
      const auto named = names.find(command.object);
      if (named != names.end()) object = presentations.at(named->second).entity;
    }
    if (object == nullptr) {
      log("warning: NPC attachment '" + command.object + "' not presented");
      return;
    }
    if (!owner.entity->hasSkeleton() ||
        !owner.entity->getSkeleton()->hasBone(command.bone)) {
      log("warning: NPC '" + owner.spec.name + "' has no hand bone '" +
          command.bone + "' for attachment '" + command.object + "'");
      return;
    }
    Ogre::SceneNode *original = object->getParentSceneNode();
    static_cast<void>(staticMap->setNamedObjectPhysicsEnabled(command.object,
                                                                false));
    object->detachFromParent();
    try {
      owner.entity->attachObjectToBone(command.bone, object);
    } catch (...) {
      if (original) original->attachObject(object);
      static_cast<void>(staticMap->setNamedObjectPhysicsEnabled(command.object,
                                                                 true));
      throw;
    }
    owned.push_back({object, original, command.object});
  }

  void resetFacialPose(FacialPresentation &facial) {
    for (auto &[target, keyframe] : facial.keyframes) {
      static_cast<void>(target);
      std::vector<unsigned short> references;
      for (const auto &reference : keyframe->getPoseReferences())
        references.push_back(reference.poseIndex);
      for (const unsigned short pose : references)
        keyframe->updatePoseReference(pose, 0.0F);
    }
    if (facial.state != nullptr) {
      facial.state->setTimePosition(0.0F);
      facial.state->setEnabled(false);
      facial.state->getParent()->_notifyDirty();
    }
    if (facial.softwareAnimationRequested && facial.entity != nullptr) {
      facial.entity->removeSoftwareAnimationRequest(false);
      facial.softwareAnimationRequested = false;
    }
  }

  FacialPresentation &facialRig(Presentation &owner) {
    const std::uint64_t id = owner.spec.handle.id.value;
    const auto existing = facials.find(id);
    if (existing != facials.end()) return existing->second;
    if (owner.spec.kind != RuntimeEntityKind::Npc || owner.entity == nullptr)
      throw std::invalid_argument("facial animation owner is not a presented NPC");

    FacialPresentation facial;
    facial.ownerName = owner.spec.name;
    facial.mesh = owner.entity->getMesh();
    facial.entity = owner.entity;
    facial.animationName = std::string(facialAnimationPrefix) +
                           std::to_string(id) + "/" +
                           std::to_string(owner.spec.handle.generation) + "/" +
                           std::to_string(++facialSequence);
    const Ogre::PoseList &poses = facial.mesh->getPoseList();
    if (!poses.empty()) {
      Ogre::Animation *animation =
          facial.mesh->createAnimation(facial.animationName, 1.0F);
      for (std::size_t index = 0; index < poses.size(); ++index) {
        const unsigned short target = poses[index]->getTarget();
        const int legacyIndex =
            legacyFacialPoseFromName(poses[index]->getName());
        if (legacyIndex >= 0)
          facial.namedPoses[legacyIndex].push_back(
              static_cast<unsigned short>(index));
        auto found = facial.keyframes.find(target);
        if (found == facial.keyframes.end()) {
          Ogre::VertexAnimationTrack *track = animation->createVertexTrack(
              target, Ogre::VAT_POSE);
          found = facial.keyframes
                      .emplace(target, track->createVertexPoseKeyFrame(0.0F))
                      .first;
        }
        found->second->addPoseReference(static_cast<unsigned short>(index),
                                        0.0F);
      }
      owner.entity->refreshAvailableAnimationState();
      facial.state = owner.entity->getAnimationState(facial.animationName);
      facial.state->setLoop(true);
      facial.state->setTimePosition(0.0F);
      log("facial rig ready for NPC '" + owner.spec.name + "': mesh '" +
          facial.mesh->getName() + "', poses=" +
          std::to_string(poses.size()) + ", pose targets=" +
          std::to_string(facial.keyframes.size()) + ", named phoneme poses=" +
          std::to_string(facial.namedPoses.size()));
    } else {
      log("warning: NPC '" + owner.spec.name +
          "' mesh has no vertex poses for facial animation");
    }
    return facials.emplace(id, std::move(facial)).first->second;
  }

  void applyFacialPose(FacialPresentation &facial,
                       const FacialPoseSample &sample) {
    if (facial.state == nullptr) return;
    for (auto &[target, keyframe] : facial.keyframes) {
      static_cast<void>(target);
      std::vector<unsigned short> references;
      for (const auto &reference : keyframe->getPoseReferences())
        references.push_back(reference.poseIndex);
      for (const unsigned short pose : references)
        keyframe->updatePoseReference(pose, 0.0F);
    }

    const std::size_t submeshes =
        std::max<std::size_t>(1, facial.mesh->getNumSubMeshes());
    for (const FacialPoseInfluence &influence : sample.influences) {
      const auto named = facial.namedPoses.find(influence.poseIndex);
      if (named != facial.namedPoses.end()) {
        for (const unsigned short pose : named->second) {
          if (pose >= facial.mesh->getPoseList().size()) continue;
          const unsigned short target =
              facial.mesh->getPoseList()[pose]->getTarget();
          const auto keyframe = facial.keyframes.find(target);
          if (keyframe != facial.keyframes.end())
            keyframe->second->updatePoseReference(pose, influence.weight);
        }
        continue;
      }
      if (facial.definition.patched) {
        for (std::size_t ordinal = 0; ordinal < submeshes; ++ordinal) {
          const std::size_t pose =
              static_cast<std::size_t>(influence.poseIndex) * submeshes + ordinal;
          if (pose >= facial.mesh->getPoseList().size()) continue;
          const unsigned short target = facial.mesh->getPoseList()[pose]->getTarget();
          const auto keyframe = facial.keyframes.find(target);
          if (keyframe != facial.keyframes.end())
            keyframe->second->updatePoseReference(
                static_cast<unsigned short>(pose), influence.weight);
        }
      } else {
        const std::size_t pose = static_cast<std::size_t>(influence.poseIndex);
        if (pose >= facial.mesh->getPoseList().size()) continue;
        const unsigned short target = facial.mesh->getPoseList()[pose]->getTarget();
        const auto keyframe = facial.keyframes.find(target);
        if (keyframe != facial.keyframes.end())
          keyframe->second->updatePoseReference(
              static_cast<unsigned short>(pose), influence.weight);
      }
    }
    facial.state->setEnabled(sample.active);
    facial.state->setTimePosition(0.0F);
    facial.state->getParent()->_notifyDirty();
  }

  void startFacialVoice(const std::uint64_t owner,
                        FacialPresentation &facial) {
    audio::PlayOptions options;
    options.file = paths.contentPath(facial.definition.sound);
    if (!std::filesystem::exists(options.file)) {
      const std::filesystem::path sibling =
          facial.definition.source.parent_path() /
          facial.definition.sound.filename();
      if (std::filesystem::exists(sibling)) {
        log("facial voice '" + facial.definition.sound.generic_string() +
            "' resolved by legacy basename lookup to '" +
            sibling.generic_string() + "'");
        options.file = sibling;
      }
    }
    options.bus = audio::Bus::voice;
    options.spatial = true;
    options.position = {static_cast<float>(facial.position.x),
                        static_cast<float>(facial.position.y),
                        static_cast<float>(facial.position.z)};
    options.minDistance = 50.0F;
    options.maxDistance = 1200.0F;
    audio::SoundHandle sound = audio->play(options);
    if (!sound.valid())
      throw std::runtime_error("NPC voice '" + options.file.string() +
                               "' failed: " + audio->lastError());
    voices.insert_or_assign(owner, std::move(sound));
    facial.voiceStarted = true;
    log("facial voice started for NPC '" + facial.ownerName + "': '" +
        options.file.generic_string() + "'");
  }

  void playFacial(const PlayRuntimeFacial &command) {
    FacialAnimationDefinition definition = loadFacialAnimationDefinition(
        paths.contentPath(command.definition));
    Presentation &owner = require(command.owner);
    FacialPresentation &facial = facialRig(owner);
    resetFacialPose(facial);
    voices.erase(command.owner.id.value);
    facial.definition = std::move(definition);
    facial.position = command.position;
    facial.leadInElapsed = 0.0;
    facial.nextProgressLog = 0.0;
    facial.voiceStarted = false;
    facial.active = true;
    if (facial.state != nullptr && facial.entity != nullptr) {
      // RTSS skeletal programs do not advertise pose-animation inputs. Force
      // Ogre's software vertex-animation path while speaking so the facial
      // pose is applied before the ordinary skeletal skinning stage.
      facial.entity->addSoftwareAnimationRequest(false);
      facial.softwareAnimationRequested = true;
    }
    log("facial animation started for NPC '" + owner.spec.name + "': '" +
        facial.definition.source.generic_string() + "', phonemes=" +
        std::to_string(facial.definition.phonemes.size()) +
        ", duration=" +
        Ogre::StringConverter::toString(
            static_cast<Ogre::Real>(facial.definition.durationSeconds())) +
        "s, patched=" + (facial.definition.patched ? "true" : "false") +
        (facial.state == nullptr ? ", visual poses unavailable" :
                                   ", software pose blending enabled"));
    applyFacialPose(facial,
                    sampleFacialAnimation(facial.definition,
                                          -facialAnimationLeadInSeconds));
    if (!facial.definition.subtitle.empty()) {
      ui->setSubtitle(facial.definition.subtitle,
                      facialAnimationLeadInSeconds +
                          facial.definition.durationSeconds() +
                          facialAnimationLeadInSeconds);
    }
  }

  void updateFacials(const float seconds) {
    for (auto &[owner, facial] : facials) {
      if (!facial.active) continue;
      double soundSecond{};
      if (!facial.voiceStarted) {
        facial.leadInElapsed += seconds;
        soundSecond = facial.leadInElapsed - facialAnimationLeadInSeconds;
        if (facial.leadInElapsed >= facialAnimationLeadInSeconds) {
          startFacialVoice(owner, facial);
          soundSecond = 0.0;
        }
      } else {
        const auto voice = voices.find(owner);
        if (voice == voices.end()) {
          resetFacialPose(facial);
          facial.active = false;
          log("facial animation stopped for NPC '" + facial.ownerName +
              "': voice handle missing");
          continue;
        }
        soundSecond = audio->playbackSeconds(voice->second);
        if (audio->state(voice->second) == audio::SoundState::stopped &&
            soundSecond < facial.definition.durationSeconds()) {
          resetFacialPose(facial);
          facial.active = false;
          log("facial animation stopped early for NPC '" +
              facial.ownerName + "': voice playback stopped at " +
              Ogre::StringConverter::toString(
                  static_cast<Ogre::Real>(soundSecond)) + "s");
          continue;
        }
      }
      const FacialPoseSample sample =
          sampleFacialAnimation(facial.definition, soundSecond);
      applyFacialPose(facial, sample);
      if (sample.active && soundSecond >= facial.nextProgressLog) {
        std::string influences;
        for (const FacialPoseInfluence &influence : sample.influences) {
          if (!influences.empty()) influences += ", ";
          influences += std::to_string(influence.poseIndex) + "=" +
              Ogre::StringConverter::toString(
                  static_cast<Ogre::Real>(influence.weight));
        }
        log("facial animation NPC '" + facial.ownerName + "' at " +
            Ogre::StringConverter::toString(
                static_cast<Ogre::Real>(soundSecond)) +
            "s: pose weights [" + influences + "]");
        facial.nextProgressLog = soundSecond + 0.5;
      }
      if (!sample.active && facial.voiceStarted &&
          soundSecond > facial.definition.durationSeconds()) {
        resetFacialPose(facial);
        facial.active = false;
        log("facial animation completed for NPC '" + facial.ownerName +
            "' at " + Ogre::StringConverter::toString(
                static_cast<Ogre::Real>(soundSecond)) + "s");
      }
    }
  }

  AppPaths paths;
  Ogre::SceneManager *sceneManager{};
  Ogre::Camera *camera{};
  physics::PhysicsWorld *physicsWorld{};
  StaticMap *staticMap{};
  PlayerController *player{};
  audio::IAudioEngine *audio{};
  ui::IUiSystem *ui{};
  audio::SoundRuntime oneShots;
  DynamicPhysicsScene dynamicPhysics;
  MapChangeRequest mapChangeRequest;
  scripting::ScriptEngine scripts;
  SequenceRuntime *runtime{};
  NpcSystem *npcs{};
  audio::MapAudioRuntime *mapAudio{};
  rendering::OgreLighting *lighting{};
  Ogre::SceneNode *root{};
  std::unordered_map<std::uint64_t, Presentation> presentations;
  std::unordered_map<std::string, std::uint64_t> names;
  std::unordered_map<std::string, Ogre::Entity *> namedPartEntities;
  std::unordered_map<std::string, ParticlePresentation> particles;
  std::uint64_t particleSequence{};
  std::unordered_map<PhysicsEntityId, EntityHandle> physicsHandles;
  std::unordered_map<std::uint64_t, audio::SoundHandle> sounds;
  std::unordered_map<std::uint64_t, audio::SoundHandle> voices;
  std::unordered_map<std::uint64_t, FacialPresentation> facials;
  std::uint64_t facialSequence{};
  std::unordered_map<std::uint64_t, PhysicsEntityId> ragdolls;
  std::unordered_map<std::uint64_t, std::vector<Attachment>> attachments;
  audio::SoundHandle music;
  std::set<std::string> reportedDeferred;
  std::unordered_map<std::string, bool> effectStates;
  std::unordered_map<std::uint64_t, ComputerMaterialBinding> computerMaterialBindings;
  FlashlightConfig flashlightConfig;
  Ogre::Light *flashlight{};
  Ogre::SceneNode *flashlightNode{};
  bool flashlightAllowed{true};
  Ogre::Overlay *gameTextOverlay{};
  Ogre::OverlayElement *gameTextElement{};
  Ogre::Overlay *crosshairOverlay{};
  double gameTextSeconds{};
  double meshLodBias{1.0};
  double defaultFovDegrees{75.0};
  Ogre::ColourValue baseAmbient{0.25F, 0.25F, 0.25F};
  std::uint64_t npcPresentationFrame{};
  NpcPresentationStats npcStats;
};

OgreSequenceServices::OgreSequenceServices(
    const AppPaths &paths, Ogre::SceneManager &sceneManager,
    Ogre::Camera &camera,
    physics::PhysicsWorld &physicsWorld, StaticMap &staticMap,
    PlayerController &player,
    audio::IAudioEngine &audio, ui::IUiSystem &ui,
    MapChangeRequest mapChangeRequest,
    double meshLodBias, double defaultFovDegrees)
    : impl_(std::make_unique<Impl>(paths, sceneManager, camera, physicsWorld,
                                   staticMap,
                                   player,
                                   audio, ui, std::move(mapChangeRequest),
                                   meshLodBias, defaultFovDegrees)) {}
OgreSequenceServices::~OgreSequenceServices() = default;

void OgreSequenceServices::attach(SequenceRuntime &runtime) noexcept {
  impl_->runtime = &runtime;
}
void OgreSequenceServices::attachNpcSystem(NpcSystem &system) noexcept {
  impl_->npcs = &system;
}
void OgreSequenceServices::attachMapAudio(audio::MapAudioRuntime &mapAudio) noexcept {
  impl_->mapAudio = &mapAudio;
}
void OgreSequenceServices::attachLighting(
    rendering::OgreLighting &lighting) noexcept {
  impl_->lighting = &lighting;
}
void OgreSequenceServices::toggleFlashlight() { impl_->toggleFlashlight(); }
void OgreSequenceServices::setFlashlightAllowed(const bool allowed) {
  impl_->setFlashlightAllowed(allowed);
}
bool OgreSequenceServices::flashlightEnabled() const noexcept {
  return impl_->flashlight != nullptr;
}
bool OgreSequenceServices::flashlightAllowed() const noexcept {
  return impl_->flashlightAllowed;
}
void OgreSequenceServices::updateAudio(float seconds) {
  impl_->oneShots.update(seconds);
  impl_->updateGameText(seconds);
  impl_->updateFacials(seconds);
  ++impl_->npcPresentationFrame;
  impl_->npcStats = {};
  struct ShadowCandidate {
    Ogre::Real distanceSquared{};
    std::uint64_t id{};
  };
  std::vector<ShadowCandidate> shadowCandidates;
  const Ogre::Vector3 cameraPosition = impl_->camera->getDerivedPosition();
  for (auto &[id, presentation] : impl_->presentations) {
    if (presentation.spec.kind != RuntimeEntityKind::Npc ||
        presentation.entity == nullptr) continue;
    ++impl_->npcStats.total;
    const Ogre::Vector3 position = toOgre(presentation.spec.transform.position);
    const Ogre::Real distanceSquared = cameraPosition.squaredDistance(position);
    const Ogre::Real renderDistance = static_cast<Ogre::Real>(
        std::max(0.0, presentation.spec.renderDistance));
    const bool inRenderRange = renderDistance == 0.0F ||
        distanceSquared <= renderDistance * renderDistance;
    if (presentation.spec.castShadows && presentation.entity->getVisible() &&
        inRenderRange && distanceSquared <= npcShadowDistance * npcShadowDistance)
      shadowCandidates.push_back({distanceSquared, id});

    unsigned animationInterval{};
    if (inRenderRange) {
      if (distanceSquared <= npcFullRateAnimationDistance *
                                 npcFullRateAnimationDistance)
        animationInterval = 1;
      else if (distanceSquared <= npcHalfRateAnimationDistance *
                                      npcHalfRateAnimationDistance)
        animationInterval = 2;
      else
        animationInterval = 4;
    }
    Ogre::AnimationStateSet *animationStates =
        presentation.entity->getAllAnimationStates();
    if (animationStates == nullptr) {
      ++impl_->npcStats.pausedAnimations;
      continue;
    }
    presentation.pendingAnimationSeconds += seconds;
    const bool animationDue = animationInterval != 0 &&
        (impl_->npcPresentationFrame + id) % animationInterval == 0;
    if (animationInterval == 1) ++impl_->npcStats.fullRateAnimations;
    else if (animationInterval > 1) ++impl_->npcStats.throttledAnimations;
    else ++impl_->npcStats.pausedAnimations;
    if (animationDue) {
      auto iterator = animationStates->getAnimationStateIterator();
      while (iterator.hasMoreElements()) {
        Ogre::AnimationState *state = iterator.getNext();
        // Facial pose animations are sampled explicitly by updateFacials().
        // Advancing their synthetic timeline here can make Ogre wrap away
        // from the pose keyframe while an NPC is speaking.
        if (state->getEnabled() &&
            state->getAnimationName().rfind(facialAnimationPrefix, 0) != 0)
          state->addTime(static_cast<Ogre::Real>(
              presentation.pendingAnimationSeconds));
      }
      presentation.pendingAnimationSeconds = 0.0F;
    }
  }
  std::sort(shadowCandidates.begin(), shadowCandidates.end(),
            [](const ShadowCandidate &left, const ShadowCandidate &right) {
              if (left.distanceSquared != right.distanceSquared)
                return left.distanceSquared < right.distanceSquared;
              return left.id < right.id;
            });
  const std::size_t casterCount =
      std::min(npcShadowCasterBudget, shadowCandidates.size());
  std::set<std::uint64_t> shadowCasterIds;
  for (std::size_t index = 0; index < casterCount; ++index)
    shadowCasterIds.insert(shadowCandidates[index].id);
  for (auto &[id, presentation] : impl_->presentations) {
    if (presentation.spec.kind != RuntimeEntityKind::Npc ||
        presentation.entity == nullptr) continue;
    const bool shouldCast = shadowCasterIds.count(id) != 0;
    if (presentation.entity->getCastShadows() != shouldCast)
      presentation.entity->setCastShadows(shouldCast);
  }
  impl_->npcStats.shadowCasters = casterCount;
}

std::optional<double> OgreSequenceServices::runtimeMusicSeconds() const {
  if (impl_->mapAudio != nullptr) {
    if (const auto seconds = impl_->mapAudio->musicPlaybackSeconds())
      return static_cast<double>(*seconds);
    return std::nullopt;
  }
  if (!impl_->music.valid() ||
      impl_->audio->state(impl_->music) == audio::SoundState::stopped)
    return std::nullopt;
  return static_cast<double>(impl_->audio->playbackSeconds(impl_->music));
}

bool OgreSequenceServices::seekRuntimeMusicSeconds(const double seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0) return false;
  if (impl_->mapAudio != nullptr)
    return impl_->mapAudio->seekMusicSeconds(static_cast<float>(seconds));
  return impl_->music.valid() &&
         impl_->audio->seekSeconds(impl_->music, static_cast<float>(seconds));
}

void OgreSequenceServices::submit(const GameCommand &command) {
  std::visit(
      Overloaded{
          [this](const SpawnRuntimeEntity &value) { impl_->spawn(value.spec); },
          [this](const DestroyRuntimeEntity &value) { impl_->destroy(value.handle); },
          [this](const DestroyRuntimeEntities &) { impl_->destroyAll(); },
          [this](const SetRuntimeTransform &value) { impl_->setTransform(value); },
          [this](const SetRuntimeVisible &value) {
            Impl::Presentation &entry = impl_->require(value.handle);
            if (entry.entity != nullptr) entry.entity->setVisible(value.visible);
            else if (entry.spec.kind == RuntimeEntityKind::Button) {
              static_cast<void>(impl_->staticMap->setNamedObjectVisible(
                  entry.spec.name, value.visible));
            }
          },
          [this](const SetRuntimeCollision &value) {
            Impl::Presentation &entry = impl_->require(value.handle);
            if (entry.physicsEntity)
              impl_->dynamicPhysics.setEntityEnabled(*entry.physicsEntity,
                                                      value.enabled);
            for (auto &part : entry.parts) {
              if (part.physicsEntity)
                impl_->dynamicPhysics.setEntityEnabled(*part.physicsEntity,
                                                        value.enabled);
            }
          },
          [this](const SetRuntimeNamedVisible &value) {
            if (impl_->staticMap->setNamedObjectVisible(value.name,
                                                         value.visible)) return;
            const auto named = impl_->names.find(value.name);
            if (named != impl_->names.end()) {
              Impl::Presentation &entry = impl_->presentations.at(named->second);
              if (entry.entity != nullptr) {
                entry.entity->setVisible(value.visible.value_or(
                    !entry.entity->getVisible()));
                return;
              }
            }
            const auto part = impl_->namedPartEntities.find(value.name);
            if (part != impl_->namedPartEntities.end()) {
              part->second->setVisible(value.visible.value_or(
                  !part->second->getVisible()));
              return;
            }
            impl_->log("warning: authored entity '" + value.name +
                       "' has no loaded presentation");
          },
          [this](const SetRuntimeNamedMaterial &value) {
            if (impl_->staticMap->setNamedObjectMaterial(value.name,
                                                          value.material))
              return;
            const auto part = impl_->namedPartEntities.find(value.name);
            if (part != impl_->namedPartEntities.end()) {
              impl_->staticMap->applyCompatibleMaterials(*part->second,
                                                          value.material);
              return;
            }
            const auto named = impl_->names.find(value.name);
            if (named != impl_->names.end()) {
              Impl::Presentation &entry = impl_->presentations.at(named->second);
              if (entry.entity != nullptr) {
                impl_->staticMap->applyCompatibleMaterials(*entry.entity,
                                                            value.material);
                return;
              }
            }
            impl_->log("warning: materialEntity skipped missing entity '" +
                       value.name + "'");
          },
          [this](const SetRuntimeLightVisible &value) {
            if (!impl_->sceneManager->hasLight(value.name)) {
              impl_->log("authored light '" + value.name +
                         "' is not yet presented by the map renderer");
              return;
            }
            Ogre::Light *light = impl_->sceneManager->getLight(value.name);
            light->setVisible(value.visible.value_or(!light->getVisible()));
          },
          [this](const PlayRuntimeSound &value) { impl_->playSound(value); },
          [this](const StopRuntimeSound &value) {
            if (value.owner.id.value == 0 && impl_->mapAudio != nullptr) {
              impl_->mapAudio->stopMusic(0.25F);
            } else if (value.owner.id.value == 0) impl_->music.reset();
            else {
              impl_->sounds.erase(value.owner.id.value);
              impl_->voices.erase(value.owner.id.value);
              const auto facial = impl_->facials.find(value.owner.id.value);
              if (facial != impl_->facials.end()) {
                impl_->resetFacialPose(facial->second);
                facial->second.active = false;
                facial->second.voiceStarted = false;
              }
            }
          },
          [this](const PlayRuntimeEffect &value) {
            const std::filesystem::path path = impl_->paths.contentPath(value.path);
            const bool ok = value.spatial
                ? impl_->oneShots.emit3D(
                      path, value.durationSeconds,
                      {static_cast<float>(value.position.x),
                       static_cast<float>(value.position.y),
                       static_cast<float>(value.position.z)},
                      50.0F, 1200.0F)
                : impl_->oneShots.emit(path, value.durationSeconds);
            if (!ok) {
              throw std::runtime_error("cannot play effect '" + path.string() +
                                       "': " + impl_->audio->lastError());
            }
          },
          [this](const SetRuntimeMusicGain &value) {
            if (impl_->mapAudio != nullptr) impl_->mapAudio->setMusicVolume(value.gain);
            else impl_->audio->setBusGain(audio::Bus::music, value.gain);
          },
          [this](const SetRuntimeAmbientEnabled &value) {
            if (impl_->mapAudio == nullptr ||
                !impl_->mapAudio->setNamedAmbientEnabled(value.name, value.enabled)) {
              impl_->log("warning: missing named ambient sound '" +
                         value.name + "'; command skipped");
            }
          },
          [this](const RunRuntimeScript &value) {
            impl_->scripts.executeFile(value.path);
          },
          [this](const ChangeRuntimeMap &value) {
            if (value.map.empty()) throw std::invalid_argument("empty map change target");
            impl_->log("map change requested: " + value.map);
            if (impl_->mapChangeRequest) impl_->mapChangeRequest(value.map);
          },
          [this](const DamageRuntimePlayer &value) {
            impl_->log("player damage requested: " + std::to_string(value.amount));
          },
          [this](const TeleportRuntimePlayer &value) {
            impl_->player->teleport(value.position);
          },
          [this](const ApplyRuntimeParentMotion &value) {
            impl_->player->applyParentMotion(value.translation);
          },
          [this](const SetRuntimePlayerParented &value) {
            impl_->player->setParented(value.parented);
          },
          [this](const SetRuntimeHudVisible &value) {
            impl_->setHudVisible(value.visible);
          },
          [this](const SetRuntimeSubtitle &value) {
            impl_->setGameText(value.text, value.seconds);
          },
          [this](const SetRuntimeInventoryEnabled &value) {
            impl_->ui->setInventoryEnabled(value.enabled);
          },
          [this](const SetRuntimeFlashlightAllowed &value) {
            impl_->setFlashlightAllowed(value.allowed);
          },
          [this](const ToggleRuntimeFlashlight &) {
            impl_->toggleFlashlight();
          },
          [this](const SetRuntimeFov &value) {
            const double degrees = value.degrees.value_or(
                impl_->defaultFovDegrees);
            if (!std::isfinite(degrees) || degrees <= 0.0 || degrees >= 180.0) {
              throw std::invalid_argument(
                  "camera FOV must be finite and between 0 and 180 degrees");
            }
            impl_->camera->setFOVy(Ogre::Degree(
                static_cast<Ogre::Real>(degrees)));
          },
          [this](const SetRuntimeCompositor &value) {
            if (impl_->lighting == nullptr)
              throw std::logic_error("compositor renderer is not attached");
            impl_->lighting->setCompositorEnabled(value.name, value.enabled);
            impl_->log("compositor '" + value.name + "' " +
                       (value.enabled ? "enabled" : "disabled"));
          },
          [this](const SetRuntimeShaderParameter &value) {
            if (impl_->lighting == nullptr)
              throw std::logic_error("compositor renderer is not attached");
            impl_->lighting->setCompositorShaderParameter(
                value.program, value.parameter, value.value);
          },
          [this](const SetRuntimeEffectEnabled &value) {
            impl_->setEffectEnabled(value);
          },
          [this](const CreateRuntimeParticle &value) {
            RuntimeParticleSpec spec;
            spec.name = value.name;
            spec.templateName = value.templateName;
            spec.position = value.position;
            spec.scale = value.scale;
            spec.visible = true;
            impl_->createParticle(spec, *impl_->root, 0);
          },
          [this](const DestroyRuntimeParticle &value) {
            if (impl_->particles.count(value.name) == 0 &&
                impl_->sceneManager->hasParticleSystem(value.name)) {
              try {
                Ogre::ParticleSystem *system =
                    impl_->sceneManager->getParticleSystem(value.name);
                Ogre::SceneNode *node = system->getParentSceneNode();
                impl_->sceneManager->destroyParticleSystem(system);
                if (node != nullptr) impl_->sceneManager->destroySceneNode(node);
              } catch (const Ogre::Exception &error) {
                impl_->log("warning: deleteParticleSystem '" + value.name +
                           "' failed: " + error.getDescription());
              }
            } else {
              impl_->destroyParticle(value.name);
            }
          },
          [this](const SetComputerPresentation &value) {
            setComputerPresentation(value);
          },
          [this](const SendComputerInput &value) {
            sendComputerInput(value);
          },
          [this](const SetRuntimeDarkness &value) {
            const float factor = static_cast<float>(std::clamp(value.factor, 0.0, 4.0));
            impl_->sceneManager->setAmbientLight(impl_->baseAmbient * factor);
          },
          [this](const PlayRuntimeAnimation &value) {
            Impl::Presentation &entry = impl_->require(value.owner);
            if (entry.entity == nullptr ||
                !entry.entity->hasAnimationState(value.animation)) {
              if (entry.spec.kind == RuntimeEntityKind::Npc) {
                impl_->log("NPC '" + entry.spec.name + "' has no animation '" +
                           value.animation + "'");
                return;
              }
              throw std::runtime_error("missing animation '" + value.animation +
                                       "' on '" + entry.spec.name + "'");
            }
            Ogre::AnimationState *state =
                entry.entity->getAnimationState(value.animation);
            if (entry.spec.kind == RuntimeEntityKind::Npc &&
                entry.entity->getAllAnimationStates() != nullptr) {
              auto iterator = entry.entity->getAllAnimationStates()->getAnimationStateIterator();
              while (iterator.hasMoreElements()) {
                Ogre::AnimationState *other = iterator.getNext();
                if (other != state &&
                    other->getAnimationName().rfind(facialAnimationPrefix, 0) != 0)
                  other->setEnabled(false);
              }
            }
            state->setLoop(value.loop);
            state->setEnabled(true);
          },
          [this](const NpcRuntimeCommand &value) {
            if (impl_->npcs == nullptr)
              throw std::logic_error("NPC command before NpcSystem attach");
            impl_->npcs->dispatch(value);
          },
          [this](const DestroyNpcRuntimeCommand &value) {
            if (impl_->npcs == nullptr)
              throw std::logic_error("NPC destruction before NpcSystem attach");
            if (!impl_->npcs->destroy(value.name))
              impl_->log("warning: destroyNPC skipped missing NPC '" +
                         value.name + "'");
          },
          [this](const SetNpcAttachment &value) {
            impl_->setNpcAttachment(value);
          },
          [this](const PlayRuntimeFacial &value) {
            impl_->playFacial(value);
          },
          [this](const SpawnRuntimeRagdoll &value) {
            if (impl_->ragdolls.count(value.owner.id.value) != 0) return;
            const auto &owner = impl_->require(value.owner);
            impl_->ragdolls.emplace(value.owner.id.value,
                impl_->dynamicPhysics.createRagdoll(
                    {owner.spec.name + "/ragdoll", value.transform, 70.0, 10.0}));
          },
          [this](const SetNpcUpdateInterval &value) {
            if (impl_->npcs == nullptr)
              throw std::logic_error("NPC interval before NpcSystem attach");
            impl_->npcs->setUpdateInterval(value.seconds);
          },
          [this](const TickRuntimeNpcPhysics &value) {
            impl_->dynamicPhysics.update(value.seconds);
            impl_->syncNpcBodiesFromPhysics();
          },
          [this](const SetRuntimeNpcGravity &value) {
            auto &presentation = impl_->require(value.handle);
            if (presentation.spec.kind != RuntimeEntityKind::Npc ||
                !presentation.physicsEntity)
              return;
            presentation.spec.gravityEnabled = value.enabled;
            impl_->dynamicPhysics.setEntityGravityEnabled(
                *presentation.physicsEntity, value.enabled);
          },
          [this](const SetRuntimeBodyFrozen &value) {
            if (!impl_->setNamedBodyFrozen(value.name, value.frozen)) {
              impl_->log("warning: " +
                         std::string(value.frozen ? "freezeBod" :
                                                    "unfreezeBod") +
                         " target '" + value.name +
                         "' has no dynamic physics body");
              return;
            }
            impl_->log("body '" + value.name + "' " +
                       (value.frozen ? "frozen" : "unfrozen"));
          },
          [this](const DeferredLegacyCommand &value) {
            if (impl_->reportedDeferred.insert(value.name).second) {
              impl_->log("typed command deferred to a later porting step: " +
                         value.name + " (" + value.detail + ")");
            }
          },
          [this](const RuntimeLog &value) { impl_->log(value.message); }},
      command);
}

void OgreSequenceServices::setComputerPresentation(
    const SetComputerPresentation &state) {
  impl_->setComputerPresentation(state);
}

void OgreSequenceServices::sendComputerInput(
    const SendComputerInput &input) {
  impl_->sendComputerInput(input);
}

physics::Vec3 OgreSequenceServices::playerPosition() const {
  return impl_->player->state().position;
}
physics::Vec3 OgreSequenceServices::playerHalfExtents() const {
  return impl_->player->collisionHalfExtents();
}
bool OgreSequenceServices::playerStandingOn(EntityHandle handle) const {
  const PlayerState state = impl_->player->state();
  if (state.noclip || !state.grounded) return false;
  physics::RaycastQuery query;
  query.from = state.position;
  query.to = query.from;
  query.to.y -= impl_->player->collisionHalfExtents().y + 25.0;
  query.group = physics::CollisionGroup::Player;
  query.mask = physics::collisionMask(physics::CollisionGroup::Train);
  query.ignoreBody = impl_->player->bodyId();
  query.includeTriggers = false;
  const auto hit = impl_->physicsWorld->raycastClosest(query);
  if (!hit || hit->normal.y < 0.35) return false;
  const auto owner = handleForPhysicsEntity(hit->metadata.entityId);
  return owner && *owner == handle;
}
std::optional<bool>
OgreSequenceServices::lightVisible(std::string_view name) const {
  if (!impl_->sceneManager->hasLight(std::string(name))) return std::nullopt;
  return impl_->sceneManager->getLight(std::string(name))->getVisible();
}

std::optional<physics::Transform>
OgreSequenceServices::runtimeTransform(std::string_view name) const {
  const auto found = impl_->names.find(std::string(name));
  if (found != impl_->names.end()) {
    const auto &entry = impl_->presentations.at(found->second);
    if (entry.node) {
      entry.node->_update(true, true);
      return physics::Transform{fromOgre(entry.node->_getDerivedPosition()),
                                fromOgre(entry.node->_getDerivedOrientation())};
    }
    return entry.spec.transform;
  }
  if (impl_->sceneManager->hasSceneNode(std::string(name))) {
    auto *node = impl_->sceneManager->getSceneNode(std::string(name));
    node->_update(true, true);
    return physics::Transform{fromOgre(node->_getDerivedPosition()),
                              fromOgre(node->_getDerivedOrientation())};
  }
  return std::nullopt;
}

physics::Vec3 OgreSequenceServices::runtimeScale(std::string_view name) const {
  const auto found = impl_->names.find(std::string(name));
  if (found != impl_->names.end()) {
    const auto &entry = impl_->presentations.at(found->second);
    if (entry.node) {
      entry.node->_update(true, true);
      return fromOgre(entry.node->_getDerivedScale());
    }
    return entry.spec.scale;
  }
  if (impl_->sceneManager->hasSceneNode(std::string(name))) {
    auto *node = impl_->sceneManager->getSceneNode(std::string(name));
    node->_update(true, true);
    return fromOgre(node->_getDerivedScale());
  }
  return {1.0, 1.0, 1.0};
}

std::optional<physics::Transform>
OgreSequenceServices::settleRuntimeNpc(EntityHandle handle) {
  Impl::Presentation &entry = impl_->require(handle);
  if (entry.spec.kind != RuntimeEntityKind::Npc) return std::nullopt;

  physics::RaycastQuery query;
  query.from = entry.spec.transform.position;
  query.from.y += std::max(1.0, entry.collisionHalfExtents.y);
  query.to = query.from;
  query.to.y -= std::max(2000.0, entry.collisionHalfExtents.y * 8.0);
  query.group = physics::CollisionGroup::Npc;
  query.mask = physics::collisionMask(physics::CollisionGroup::World) |
               physics::collisionMask(physics::CollisionGroup::Door) |
               physics::collisionMask(physics::CollisionGroup::Train) |
               physics::collisionMask(physics::CollisionGroup::Dynamic);
  query.includeTriggers = false;
  const auto hit = impl_->physicsWorld->raycastClosest(query);
  if (!hit || hit->normal.y < 0.35) return std::nullopt;

  physics::Transform settled = entry.spec.transform;
  settled.position.y = hit->point.y + entry.collisionHalfExtents.y;
  impl_->setTransform(SetRuntimeTransform{handle, settled});
  return settled;
}

bool OgreSequenceServices::runtimeFacialActive(EntityHandle handle) const {
  const auto found = impl_->facials.find(handle.id.value);
  return found != impl_->facials.end() && found->second.active;
}

double OgreSequenceServices::runtimeFovDegrees() const {
  return static_cast<double>(impl_->camera->getFOVy().valueDegrees());
}

std::optional<EntityHandle>
OgreSequenceServices::handleForPhysicsEntity(std::uint64_t physicsEntity) const {
  const auto found = impl_->physicsHandles.find(physicsEntity);
  return found == impl_->physicsHandles.end()
             ? std::nullopt
             : std::optional<EntityHandle>{found->second};
}

OgreSequenceResourceCounts
OgreSequenceServices::resourceCounts() const noexcept {
  OgreSequenceResourceCounts result;
  result.presentations = impl_->presentations.size();
  for (const auto &[id, presentation] : impl_->presentations) {
    static_cast<void>(id);
    result.visualParts += presentation.parts.size();
  }
  result.particles = impl_->particles.size();
  result.physicsBindings = impl_->physicsHandles.size();
  result.audioHandles = impl_->sounds.size() + impl_->voices.size() +
                        (impl_->music.valid() ? 1U : 0U);
  result.facialAnimations = impl_->facials.size();
  result.attachments = impl_->attachments.size();
  result.ragdolls = impl_->ragdolls.size();
  result.rootNode = impl_->root != nullptr;
  return result;
}

NpcPresentationStats OgreSequenceServices::npcPresentationStats() const noexcept {
  return impl_->npcStats;
}

} // namespace run3::gameplay
