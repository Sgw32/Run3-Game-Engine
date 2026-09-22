#pragma once

#include <run3/physics/Physics.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace run3::gameplay {

using PhysicsEntityId = std::uint64_t;

enum class GameplayPhysicsEventType {
  PickupCollected,
  ButtonPressed,
  TriggerEntered,
  TriggerExited,
  ProjectileImpact,
  Damage,
  Broken,
  NpcContact
};

struct GameplayPhysicsEvent {
  GameplayPhysicsEventType type{};
  PhysicsEntityId entity{};
  PhysicsEntityId other{};
  double amount{};
};

struct DynamicEntityDesc {
  std::string name;
  physics::BodyType type{physics::BodyType::PhysicalObject};
  physics::Shape shape;
  physics::Transform transform;
  physics::BodyMotion motion{physics::BodyMotion::Dynamic};
  double massKg{1.0};
  double health{100.0};
  physics::CollisionGroup group{physics::CollisionGroup::Dynamic};
  physics::CollisionMask mask{physics::collisionMask(
      physics::CollisionGroup::All)};
  bool trigger{};

  DynamicEntityDesc(std::string entityName, physics::BodyType bodyType,
                    physics::Shape bodyShape)
      : name(std::move(entityName)), type(bodyType),
        shape(std::move(bodyShape)) {}
};

struct RagdollDesc {
  std::string name;
  physics::Transform transform;
  double totalMassKg{70.0};
  double lifetimeSeconds{10.0};
};

class DynamicPhysicsScene final {
public:
  explicit DynamicPhysicsScene(physics::PhysicsWorld &world);
  ~DynamicPhysicsScene();
  DynamicPhysicsScene(const DynamicPhysicsScene &) = delete;
  DynamicPhysicsScene &operator=(const DynamicPhysicsScene &) = delete;

  [[nodiscard]] PhysicsEntityId createEntity(DynamicEntityDesc description);
  [[nodiscard]] PhysicsEntityId
  createDoor(DynamicEntityDesc description, physics::Transform openTransform,
             double speedGameUnitsPerSecond);
  [[nodiscard]] PhysicsEntityId createTrain(DynamicEntityDesc description);
  [[nodiscard]] PhysicsEntityId createRagdoll(const RagdollDesc &description);

  void setDoorOpen(PhysicsEntityId entity, bool open);
  void setTrainTransform(PhysicsEntityId entity,
                         const physics::Transform &transform);
  void setEntityTransform(PhysicsEntityId entity,
                          const physics::Transform &transform);
  void update(double seconds);
  [[nodiscard]] std::vector<GameplayPhysicsEvent> processContactEvents(
      double projectileDamage = 25.0);

  [[nodiscard]] physics::Transform transform(PhysicsEntityId entity,
                                             std::uint32_t part = 0) const;
  [[nodiscard]] double health(PhysicsEntityId entity) const;
  [[nodiscard]] bool enabled(PhysicsEntityId entity) const;
  [[nodiscard]] std::size_t entityCount() const noexcept;
  [[nodiscard]] std::size_t bodyCount() const noexcept;
  [[nodiscard]] std::size_t constraintCount() const noexcept;
  void destroyEntity(PhysicsEntityId entity) noexcept;
  void setEntityEnabled(PhysicsEntityId entity, bool enabled);
  void unload() noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
