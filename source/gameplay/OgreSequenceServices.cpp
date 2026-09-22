#include <run3/gameplay/OgreSequenceServices.hpp>

#include <run3/audio/MapAudio.hpp>
#include <run3/audio/SoundRuntime.hpp>
#include <run3/content/XmlParser.hpp>
#include <run3/gameplay/DynamicPhysicsScene.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/NpcSystem.hpp>
#include <run3/gameplay/StaticMap.hpp>
#include <run3/scripting/ScriptEngine.hpp>

#include <OgreAnimationState.h>
#include <OgreAxisAlignedBox.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeletonInstance.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace {

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

physics::BodyType bodyType(RuntimeEntityKind kind) {
  switch (kind) {
  case RuntimeEntityKind::Button: return physics::BodyType::Button;
  case RuntimeEntityKind::Door:
  case RuntimeEntityKind::Rotator:
  case RuntimeEntityKind::Pendulum: return physics::BodyType::Door;
  case RuntimeEntityKind::Train: return physics::BodyType::Train;
  case RuntimeEntityKind::Npc: return physics::BodyType::Npc;
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
    RuntimeEntitySpec spec;
    Ogre::SceneNode *node{};
    Ogre::Entity *entity{};
    Ogre::Vector3 localCentre{Ogre::Vector3::ZERO};
    std::optional<PhysicsEntityId> physicsEntity;
  };

  Impl(const AppPaths &appPaths, Ogre::SceneManager &manager,
       physics::PhysicsWorld &world, StaticMap &loadedMap,
       PlayerController &playerController,
       audio::IAudioEngine &audioEngine, MapChangeRequest changeRequest)
      : paths(appPaths), sceneManager(&manager), physicsWorld(&world),
        staticMap(&loadedMap),
        player(&playerController),
        audio(&audioEngine), oneShots(audioEngine), dynamicPhysics(world),
        mapChangeRequest(std::move(changeRequest)),
        scripts({paths.contentRoot(), paths.userRoot(), 1'000'000},
                [this](const scripting::ScriptCall &call) {
                  if (runtime == nullptr) {
                    throw std::runtime_error(
                        "Script command arrived before SequenceRuntime attach");
                  }
                  return runtime->dispatchScriptCall(call);
                }) {
    root = sceneManager->getRootSceneNode()->createChildSceneNode(
        "Run3Step8CSequenceRoot");
  }

  ~Impl() { destroyAll(); }

  void log(const std::string &message) const {
    Ogre::LogManager::getSingleton().logMessage("Step 8C: " + message);
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
    body.motion = physics::BodyMotion::Kinematic;
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
      if (spec.kind == RuntimeEntityKind::Npc) {
        staticMap->applyCompatibleMaterials(*presentation.entity, spec.material);
      } else if (!spec.material.empty() &&
          Ogre::MaterialManager::getSingleton().resourceExists(spec.material)) {
        presentation.entity->setMaterialName(spec.material);
      }
      presentation.entity->setVisible(spec.visible);
      presentation.node = root->createChildSceneNode(
          "Run3Step8CNode/" + std::to_string(spec.handle.id.value));
      presentation.node->setPosition(toOgre(spec.transform.position));
      presentation.node->setOrientation(toOgre(spec.transform.rotation));
      if (spec.kind == RuntimeEntityKind::Npc) {
        presentation.node->yaw(Ogre::Degree(static_cast<Ogre::Real>(
            spec.visualYawDegrees)), Ogre::Node::TS_LOCAL);
        presentation.entity->setRenderingDistance(static_cast<Ogre::Real>(
            spec.renderDistance));
      }
      presentation.node->setScale(toOgre(spec.scale));
      presentation.node->attachObject(presentation.entity);
      const Ogre::Vector3 meshHalf = presentation.entity->getBoundingBox().getHalfSize();
      if (spec.kind != RuntimeEntityKind::Npc)
        half = {std::abs(meshHalf.x * spec.scale.x),
                std::abs(meshHalf.y * spec.scale.y),
                std::abs(meshHalf.z * spec.scale.z)};
      presentation.localCentre =
          presentation.entity->getBoundingBox().getCenter() * toOgre(spec.scale);
    }
    presentations.emplace(spec.handle.id.value, std::move(presentation));
    const auto previous = names.find(spec.name);
    if (previous == names.end() ||
        (presentations.at(previous->second).node == nullptr &&
         presentations.at(spec.handle.id.value).node != nullptr)) {
      names.insert_or_assign(spec.name, spec.handle.id.value);
    }
    resolveParents();
    createPhysics(presentations.at(spec.handle.id.value), half);
  }

  void setTransform(const SetRuntimeTransform &command) {
    Presentation &presentation = require(command.handle);
    presentation.spec.transform = command.transform;
    if (presentation.node != nullptr) {
      presentation.node->setPosition(toOgre(command.transform.position));
      presentation.node->setOrientation(toOgre(command.transform.rotation));
      if (presentation.spec.kind == RuntimeEntityKind::Npc)
        presentation.node->yaw(Ogre::Degree(static_cast<Ogre::Real>(
            presentation.spec.visualYawDegrees)), Ogre::Node::TS_LOCAL);
      presentation.node->_update(true, true);
    }
    syncPresentationBody(presentation);
    // Parent motion also moves authored children. Keep each child collision
    // body in the same Bullet world as the visual hierarchy.
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

  void syncPresentationBody(Presentation &presentation) {
    if (presentation.physicsEntity) {
      physics::Transform worldTransform = presentation.spec.transform;
      if (presentation.node != nullptr) {
        presentation.node->_update(true, true);
        worldTransform.position = fromOgre(
            presentation.node->_getDerivedPosition() +
            presentation.node->_getDerivedOrientation() *
                presentation.localCentre);
        worldTransform.rotation = fromOgre(presentation.node->_getDerivedOrientation());
        if (presentation.spec.kind == RuntimeEntityKind::Npc)
          worldTransform = presentation.spec.transform;
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

  void destroyAll() noexcept {
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
    voices.clear();
    ragdolls.clear();
    music.reset();
    oneShots.clear();
    dynamicPhysics.unload();
    physicsHandles.clear();
    names.clear();
    if (sceneManager != nullptr) {
      for (auto &[id, presentation] : presentations) {
        static_cast<void>(id);
        if (presentation.entity != nullptr) {
          try { sceneManager->destroyEntity(presentation.entity); }
          catch (...) {}
        }
      }
    }
    presentations.clear();
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
    Presentation &entry = require(handle);
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
    sounds.erase(handle.id.value);
    voices.erase(handle.id.value);
    const auto ragdoll = ragdolls.find(handle.id.value);
    if (ragdoll != ragdolls.end()) {
      dynamicPhysics.destroyEntity(ragdoll->second);
      ragdolls.erase(ragdoll);
    }
    names.erase(entry.spec.name);
    if (entry.entity) sceneManager->destroyEntity(entry.entity);
    if (entry.node) sceneManager->destroySceneNode(entry.node);
    presentations.erase(handle.id.value);
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

  void playFacial(const PlayRuntimeFacial &command) {
    const auto document = content::parseXmlFile(
        paths.contentPath(command.definition), content::XmlSchema::facialAnimation);
    const auto *file = document.root.firstChild("file");
    if (file == nullptr || file->attribute("name") == nullptr)
      throw std::runtime_error("facial animation '" +
          command.definition.string() + "' has no <file name>");
    audio::PlayOptions options;
    options.file = paths.contentPath(*file->attribute("name"));
    options.bus = audio::Bus::voice;
    options.spatial = true;
    options.position = {static_cast<float>(command.position.x),
                        static_cast<float>(command.position.y),
                        static_cast<float>(command.position.z)};
    options.minDistance = 50.0F;
    options.maxDistance = 1200.0F;
    audio::SoundHandle sound = audio->play(options);
    if (!sound.valid())
      throw std::runtime_error("NPC voice '" + options.file.string() +
                               "' failed: " + audio->lastError());
    voices.insert_or_assign(command.owner.id.value, std::move(sound));
  }

  AppPaths paths;
  Ogre::SceneManager *sceneManager{};
  physics::PhysicsWorld *physicsWorld{};
  StaticMap *staticMap{};
  PlayerController *player{};
  audio::IAudioEngine *audio{};
  audio::SoundRuntime oneShots;
  DynamicPhysicsScene dynamicPhysics;
  MapChangeRequest mapChangeRequest;
  scripting::ScriptEngine scripts;
  SequenceRuntime *runtime{};
  NpcSystem *npcs{};
  audio::MapAudioRuntime *mapAudio{};
  Ogre::SceneNode *root{};
  std::unordered_map<std::uint64_t, Presentation> presentations;
  std::unordered_map<std::string, std::uint64_t> names;
  std::unordered_map<PhysicsEntityId, EntityHandle> physicsHandles;
  std::unordered_map<std::uint64_t, audio::SoundHandle> sounds;
  std::unordered_map<std::uint64_t, audio::SoundHandle> voices;
  std::unordered_map<std::uint64_t, PhysicsEntityId> ragdolls;
  std::unordered_map<std::uint64_t, std::vector<Attachment>> attachments;
  audio::SoundHandle music;
  std::set<std::string> reportedDeferred;
  Ogre::ColourValue baseAmbient{0.25F, 0.25F, 0.25F};
};

OgreSequenceServices::OgreSequenceServices(
    const AppPaths &paths, Ogre::SceneManager &sceneManager,
    physics::PhysicsWorld &physicsWorld, StaticMap &staticMap,
    PlayerController &player,
    audio::IAudioEngine &audio, MapChangeRequest mapChangeRequest)
    : impl_(std::make_unique<Impl>(paths, sceneManager, physicsWorld, staticMap,
                                   player,
                                   audio, std::move(mapChangeRequest))) {}
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
void OgreSequenceServices::updateAudio(float seconds) {
  impl_->oneShots.update(seconds);
  for (auto &[id, presentation] : impl_->presentations) {
    static_cast<void>(id);
    if (presentation.spec.kind != RuntimeEntityKind::Npc ||
        presentation.entity == nullptr ||
        presentation.entity->getAllAnimationStates() == nullptr) continue;
    auto iterator = presentation.entity->getAllAnimationStates()->getAnimationStateIterator();
    while (iterator.hasMoreElements()) {
      Ogre::AnimationState *state = iterator.getNext();
      if (state->getEnabled()) state->addTime(seconds);
    }
  }
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
            impl_->log("authored entity '" + value.name +
                       "' is not yet presented (nested train part/deferred)");
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
            else impl_->sounds.erase(value.owner.id.value);
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
                if (other != state) other->setEnabled(false);
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
          [this](const TickRuntimeNpcPhysics &value) {
            impl_->dynamicPhysics.update(value.seconds);
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

std::optional<EntityHandle>
OgreSequenceServices::handleForPhysicsEntity(std::uint64_t physicsEntity) const {
  const auto found = impl_->physicsHandles.find(physicsEntity);
  return found == impl_->physicsHandles.end()
             ? std::nullopt
             : std::optional<EntityHandle>{found->second};
}

} // namespace run3::gameplay
