#include <run3/gameplay/DynamicPhysicsScene.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace {

using physics::BodyType;

bool pairIs(const physics::ContactEvent &event, BodyType first,
            BodyType second) {
  return (event.firstMetadata.type == first &&
          event.secondMetadata.type == second) ||
         (event.firstMetadata.type == second &&
          event.secondMetadata.type == first);
}

physics::BodyMetadata metadataOfType(const physics::ContactEvent &event,
                                     BodyType type) {
  return event.firstMetadata.type == type ? event.firstMetadata
                                         : event.secondMetadata;
}

physics::BodyMetadata otherThan(const physics::ContactEvent &event,
                                BodyType type) {
  return event.firstMetadata.type == type ? event.secondMetadata
                                         : event.firstMetadata;
}

double distance(physics::Vec3 first, physics::Vec3 second) {
  const double x = second.x - first.x;
  const double y = second.y - first.y;
  const double z = second.z - first.z;
  return std::sqrt(x * x + y * y + z * z);
}

physics::Vec3 approach(physics::Vec3 current, physics::Vec3 target,
                       double maximumDistance) {
  const double remaining = distance(current, target);
  if (remaining <= maximumDistance || remaining <= 1e-12) {
    return target;
  }
  const double scale = maximumDistance / remaining;
  return {current.x + (target.x - current.x) * scale,
          current.y + (target.y - current.y) * scale,
          current.z + (target.z - current.z) * scale};
}

} // namespace

struct DynamicPhysicsScene::Impl {
  struct Record {
    std::string name;
    BodyType type{BodyType::Unknown};
    double health{};
    std::vector<physics::BodyHandle> bodies;
    std::vector<physics::Constraint> constraints;
    std::optional<physics::Transform> closed;
    std::optional<physics::Transform> open;
    bool targetOpen{};
    double speed{};
    double remainingLifetime{-1.0};
  };

  explicit Impl(physics::PhysicsWorld &physicsWorld) : world(&physicsWorld) {}

  physics::BodyHandle makeBody(PhysicsEntityId entity, std::uint32_t part,
                               const DynamicEntityDesc &description) {
    physics::BodyDesc body(description.shape);
    body.motion = description.motion;
    body.massKg = description.motion == physics::BodyMotion::Dynamic
                      ? description.massKg
                      : 0.0;
    body.transform = description.transform;
    body.group = description.group;
    body.mask = description.mask;
    body.trigger = description.trigger;
    body.metadata = {entity, description.type, part};
    return world->createBody(body);
  }

  Record &require(PhysicsEntityId id) {
    const auto found = records.find(id);
    if (found == records.end()) {
      throw std::out_of_range("unknown dynamic physics entity");
    }
    return found->second;
  }
  const Record &require(PhysicsEntityId id) const {
    const auto found = records.find(id);
    if (found == records.end()) {
      throw std::out_of_range("unknown dynamic physics entity");
    }
    return found->second;
  }

  physics::PhysicsWorld *world{};
  PhysicsEntityId nextEntity{1};
  std::unordered_map<PhysicsEntityId, Record> records;
};

DynamicPhysicsScene::DynamicPhysicsScene(physics::PhysicsWorld &world)
    : implementation_(std::make_unique<Impl>(world)) {}
DynamicPhysicsScene::~DynamicPhysicsScene() {
  unload();
}

void DynamicPhysicsScene::destroyEntity(PhysicsEntityId entity) noexcept {
  implementation_->records.erase(entity);
}

void DynamicPhysicsScene::setEntityEnabled(PhysicsEntityId entity,
                                            bool enabled) {
  auto &record = implementation_->require(entity);
  for (auto &body : record.bodies)
    implementation_->world->setEnabled(body, enabled);
}

PhysicsEntityId
DynamicPhysicsScene::createEntity(DynamicEntityDesc description) {
  if (description.name.empty()) {
    throw std::invalid_argument("dynamic physics entity name cannot be empty");
  }
  const PhysicsEntityId id = implementation_->nextEntity++;
  Impl::Record record;
  record.name = description.name;
  record.type = description.type;
  record.health = description.health;
  record.bodies.push_back(implementation_->makeBody(id, 0, description));
  implementation_->records.emplace(id, std::move(record));
  return id;
}

PhysicsEntityId DynamicPhysicsScene::createDoor(
    DynamicEntityDesc description, physics::Transform openTransform,
    double speedGameUnitsPerSecond) {
  if (!std::isfinite(speedGameUnitsPerSecond) ||
      speedGameUnitsPerSecond <= 0.0) {
    throw std::invalid_argument("door speed must be finite and positive");
  }
  description.type = BodyType::Door;
  description.motion = physics::BodyMotion::Kinematic;
  description.massKg = 0.0;
  description.group = physics::CollisionGroup::Door;
  const physics::Transform closed = description.transform;
  const auto id = createEntity(std::move(description));
  auto &record = implementation_->require(id);
  record.closed = closed;
  record.open = openTransform;
  record.speed = speedGameUnitsPerSecond;
  return id;
}

PhysicsEntityId
DynamicPhysicsScene::createTrain(DynamicEntityDesc description) {
  description.type = BodyType::Train;
  description.motion = physics::BodyMotion::Kinematic;
  description.massKg = 0.0;
  description.group = physics::CollisionGroup::Train;
  return createEntity(std::move(description));
}

PhysicsEntityId
DynamicPhysicsScene::createRagdoll(const RagdollDesc &description) {
  if (description.name.empty() || description.totalMassKg <= 0.0 ||
      description.lifetimeSeconds < 0.0) {
    throw std::invalid_argument("invalid ragdoll description");
  }
  const PhysicsEntityId id = implementation_->nextEntity++;
  Impl::Record record;
  record.name = description.name;
  record.type = BodyType::RagdollBone;
  record.health = 0.0;
  record.remainingLifetime = description.lifetimeSeconds;

  DynamicEntityDesc torso{description.name + "/torso", BodyType::RagdollBone,
                          physics::Shape::capsule(18.0, 55.0)};
  torso.transform = description.transform;
  torso.massKg = description.totalMassKg * 0.55;
  torso.group = physics::CollisionGroup::Ragdoll;
  record.bodies.push_back(implementation_->makeBody(id, 0, torso));

  DynamicEntityDesc head{description.name + "/head", BodyType::RagdollBone,
                         physics::Shape::capsule(12.0, 8.0)};
  head.transform = description.transform;
  head.transform.position.y += 58.0;
  head.massKg = description.totalMassKg * 0.10;
  head.group = physics::CollisionGroup::Ragdoll;
  record.bodies.push_back(implementation_->makeBody(id, 1, head));

  DynamicEntityDesc leg{description.name + "/leg", BodyType::RagdollBone,
                        physics::Shape::capsule(10.0, 60.0)};
  leg.transform = description.transform;
  leg.transform.position.y -= 65.0;
  leg.massKg = description.totalMassKg * 0.35;
  leg.group = physics::CollisionGroup::Ragdoll;
  record.bodies.push_back(implementation_->makeBody(id, 2, leg));

  record.constraints.push_back(implementation_->world->createPointConstraint(
      record.bodies[0], record.bodies[1], {0.0, 28.0, 0.0},
      {0.0, -10.0, 0.0}));
  constexpr double hipLimit = 0.7853981633974483;
  record.constraints.push_back(implementation_->world->createHingeConstraint(
      record.bodies[0], record.bodies[2], {0.0, -30.0, 0.0},
      {0.0, 30.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, -hipLimit,
      hipLimit));
  implementation_->records.emplace(id, std::move(record));
  return id;
}

void DynamicPhysicsScene::setDoorOpen(PhysicsEntityId entity, bool open) {
  auto &record = implementation_->require(entity);
  if (!record.open) {
    throw std::invalid_argument("entity is not a door");
  }
  record.targetOpen = open;
}

void DynamicPhysicsScene::setTrainTransform(
    PhysicsEntityId entity, const physics::Transform &transform) {
  auto &record = implementation_->require(entity);
  if (record.type != BodyType::Train) {
    throw std::invalid_argument("entity is not a train");
  }
  implementation_->world->setTransform(record.bodies.front(), transform);
}

void DynamicPhysicsScene::setEntityTransform(
    PhysicsEntityId entity, const physics::Transform &transform) {
  auto &record = implementation_->require(entity);
  if (record.bodies.empty()) {
    throw std::invalid_argument("entity has no physics body");
  }
  implementation_->world->setTransform(record.bodies.front(), transform);
}

void DynamicPhysicsScene::update(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0) {
    throw std::invalid_argument("dynamic scene update must be non-negative");
  }
  std::vector<PhysicsEntityId> expired;
  for (auto &[id, record] : implementation_->records) {
    if (record.open) {
      auto current = implementation_->world->transform(record.bodies.front());
      const auto &target = record.targetOpen ? *record.open : *record.closed;
      current.position =
          approach(current.position, target.position, record.speed * seconds);
      if (current.position == target.position) {
        current.rotation = target.rotation;
      }
      implementation_->world->setTransform(record.bodies.front(), current);
    }
    if (record.remainingLifetime >= 0.0) {
      record.remainingLifetime -= seconds;
      if (record.remainingLifetime <= 0.0) {
        expired.push_back(id);
      }
    }
  }
  for (const auto id : expired) {
    implementation_->records.erase(id);
  }
}

std::vector<GameplayPhysicsEvent>
DynamicPhysicsScene::processContactEvents(double projectileDamage) {
  std::vector<GameplayPhysicsEvent> result;
  for (const auto &contact : implementation_->world->drainContactEvents()) {
    const bool began = contact.phase == physics::ContactPhase::Began;
    if ((pairIs(contact, BodyType::Pickup, BodyType::Player) ||
         pairIs(contact, BodyType::Button, BodyType::Player)) && began) {
      const BodyType type = pairIs(contact, BodyType::Pickup, BodyType::Player)
                                ? BodyType::Pickup
                                : BodyType::Button;
      const auto subject = metadataOfType(contact, type);
      result.push_back({type == BodyType::Pickup
                            ? GameplayPhysicsEventType::PickupCollected
                            : GameplayPhysicsEventType::ButtonPressed,
                        subject.entityId,
                        otherThan(contact, type).entityId, 0.0});
      if (type == BodyType::Pickup) {
        auto found = implementation_->records.find(subject.entityId);
        if (found != implementation_->records.end()) {
          implementation_->world->setEnabled(found->second.bodies.front(),
                                              false);
        }
      }
    }
    if (pairIs(contact, BodyType::Trigger, BodyType::Player) &&
        contact.phase != physics::ContactPhase::Persisted) {
      const auto trigger = metadataOfType(contact, BodyType::Trigger);
      result.push_back({contact.phase == physics::ContactPhase::Began
                            ? GameplayPhysicsEventType::TriggerEntered
                            : GameplayPhysicsEventType::TriggerExited,
                        trigger.entityId,
                        otherThan(contact, BodyType::Trigger).entityId, 0.0});
    }
    if (contact.phase == physics::ContactPhase::Began &&
        (contact.firstMetadata.type == BodyType::Projectile ||
         contact.secondMetadata.type == BodyType::Projectile)) {
      const auto projectile = metadataOfType(contact, BodyType::Projectile);
      const auto target = otherThan(contact, BodyType::Projectile);
      result.push_back({GameplayPhysicsEventType::ProjectileImpact,
                        projectile.entityId, target.entityId,
                        projectileDamage});
      auto projectileRecord = implementation_->records.find(projectile.entityId);
      if (projectileRecord != implementation_->records.end()) {
        implementation_->world->setEnabled(
            projectileRecord->second.bodies.front(), false);
      }
      auto targetRecord = implementation_->records.find(target.entityId);
      if (targetRecord != implementation_->records.end() &&
          (target.type == BodyType::Breakable || target.type == BodyType::Npc)) {
        targetRecord->second.health -= projectileDamage;
        result.push_back({GameplayPhysicsEventType::Damage, target.entityId,
                          projectile.entityId, projectileDamage});
        if (targetRecord->second.health <= 0.0) {
          implementation_->world->setEnabled(
              targetRecord->second.bodies.front(), false);
          result.push_back({GameplayPhysicsEventType::Broken, target.entityId,
                            projectile.entityId, projectileDamage});
        }
      }
    }
    if (pairIs(contact, BodyType::Npc, BodyType::Player) && began) {
      const auto npc = metadataOfType(contact, BodyType::Npc);
      result.push_back({GameplayPhysicsEventType::NpcContact, npc.entityId,
                        otherThan(contact, BodyType::Npc).entityId, 0.0});
    }
  }
  return result;
}

physics::Transform DynamicPhysicsScene::transform(PhysicsEntityId entity,
                                                  std::uint32_t part) const {
  const auto &record = implementation_->require(entity);
  if (part >= record.bodies.size()) {
    throw std::out_of_range("dynamic physics entity part is out of range");
  }
  return implementation_->world->transform(record.bodies[part]);
}
double DynamicPhysicsScene::health(PhysicsEntityId entity) const {
  return implementation_->require(entity).health;
}
bool DynamicPhysicsScene::enabled(PhysicsEntityId entity) const {
  return implementation_->world->isEnabled(
      implementation_->require(entity).bodies.front());
}
std::size_t DynamicPhysicsScene::entityCount() const noexcept {
  return implementation_->records.size();
}
std::size_t DynamicPhysicsScene::bodyCount() const noexcept {
  std::size_t count{};
  for (const auto &entry : implementation_->records) {
    count += entry.second.bodies.size();
  }
  return count;
}
std::size_t DynamicPhysicsScene::constraintCount() const noexcept {
  std::size_t count{};
  for (const auto &entry : implementation_->records) {
    count += entry.second.constraints.size();
  }
  return count;
}
void DynamicPhysicsScene::unload() noexcept {
  // Records own constraints after bodies, so clear explicitly in safe order.
  for (auto &entry : implementation_->records) {
    entry.second.constraints.clear();
  }
  implementation_->records.clear();
  static_cast<void>(implementation_->world->drainContactEvents());
}

} // namespace run3::gameplay
