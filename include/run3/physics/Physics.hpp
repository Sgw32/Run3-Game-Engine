#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace run3::physics {

struct Vec3 {
  double x{};
  double y{};
  double z{};

  friend constexpr bool operator==(const Vec3 &left, const Vec3 &right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
  }
};

struct Quaternion {
  double w{1.0};
  double x{};
  double y{};
  double z{};

  friend constexpr bool operator==(const Quaternion &left,
                                   const Quaternion &right) {
    return left.w == right.w && left.x == right.x && left.y == right.y &&
           left.z == right.z;
  }
};

struct Transform {
  Vec3 position;
  Quaternion rotation;

  friend constexpr bool operator==(const Transform &left,
                                   const Transform &right) {
    return left.position == right.position && left.rotation == right.rotation;
  }
};

class UnitConversion final {
public:
  [[nodiscard]] static UnitConversion standard() noexcept;

  [[nodiscard]] double metresPerGameUnit() const noexcept;
  [[nodiscard]] double toMetres(double gameUnits) const noexcept;
  [[nodiscard]] double toGameUnits(double metres) const noexcept;
  [[nodiscard]] Vec3 toMetres(Vec3 gameUnits) const noexcept;
  [[nodiscard]] Vec3 toGameUnits(Vec3 metres) const noexcept;

private:
  explicit UnitConversion(double metresPerGameUnit);

  double metresPerGameUnit_{0.01};

  friend struct testing_PhysicsTestAccess;
};

using BodyId = std::uint64_t;
using ConstraintId = std::uint64_t;
using CollisionMask = std::uint32_t;

enum class CollisionGroup : CollisionMask {
  None = 0,
  World = 1U << 0U,
  Player = 1U << 1U,
  Dynamic = 1U << 2U,
  Default = Dynamic,
  Npc = 1U << 3U,
  Trigger = 1U << 4U,
  Projectile = 1U << 5U,
  Pickup = 1U << 6U,
  Button = 1U << 7U,
  Door = 1U << 8U,
  Train = 1U << 9U,
  Ragdoll = 1U << 10U,
  All = 0xffffffffU
};

[[nodiscard]] constexpr CollisionMask
collisionMask(CollisionGroup group) noexcept {
  return static_cast<CollisionMask>(group);
}

class Shape final {
public:
  enum class Type { Box, Capsule, TriangleMesh };

  [[nodiscard]] static Shape box(Vec3 halfExtentsGameUnits);
  [[nodiscard]] static Shape capsule(double radiusGameUnits,
                                     double cylinderHeightGameUnits);
  [[nodiscard]] static Shape
  triangleMesh(std::vector<Vec3> verticesGameUnits,
               std::vector<std::uint32_t> triangleIndices);

  [[nodiscard]] Type type() const noexcept;
  [[nodiscard]] Vec3 halfExtents() const noexcept;
  [[nodiscard]] double radius() const noexcept;
  [[nodiscard]] double cylinderHeight() const noexcept;
  [[nodiscard]] const std::vector<Vec3> &vertices() const noexcept;
  [[nodiscard]] const std::vector<std::uint32_t> &indices() const noexcept;

private:
  explicit Shape(Type type) noexcept;

  Type type_;
  Vec3 halfExtents_;
  double radius_{};
  double cylinderHeight_{};
  std::vector<Vec3> vertices_;
  std::vector<std::uint32_t> indices_;
};

enum class BodyMotion { Static, Dynamic, Kinematic };

enum class BodyType : std::uint32_t {
  Unknown,
  World,
  Player,
  PhysicalObject,
  Breakable,
  Pickup,
  Button,
  Trigger,
  Door,
  Train,
  Projectile,
  Npc,
  RagdollBone
};

struct BodyMetadata {
  std::uint64_t entityId{};
  BodyType type{BodyType::Unknown};
  std::uint32_t partId{};

  friend constexpr bool operator==(const BodyMetadata &left,
                                   const BodyMetadata &right) {
    return left.entityId == right.entityId && left.type == right.type &&
           left.partId == right.partId;
  }
};

struct BodyDesc {
  explicit BodyDesc(Shape bodyShape) : shape(std::move(bodyShape)) {}

  Shape shape;
  BodyMotion motion{BodyMotion::Static};
  double massKg{};
  Transform transform;
  Vec3 linearVelocity;
  Vec3 angularFactor{1.0, 1.0, 1.0};
  double friction{0.5};
  CollisionGroup group{CollisionGroup::Dynamic};
  CollisionMask mask{collisionMask(CollisionGroup::All)};
  bool trigger{};
  bool sleepingAllowed{true};
  BodyMetadata metadata;
};

struct PhysicsConfig {
  Vec3 gravity{0.0, -981.0, 0.0};
  std::uint32_t maxCatchUpSteps{8};
};

struct StepResult {
  std::uint32_t steps{};
  double simulatedSeconds{};
  double droppedSeconds{};
  double interpolationAlpha{};
};

enum class ContactPhase { Began, Persisted, Ended };

struct ContactEvent {
  ContactPhase phase{ContactPhase::Began};
  BodyId first{};
  BodyId second{};
  BodyMetadata firstMetadata;
  BodyMetadata secondMetadata;
  Vec3 point;
  Vec3 normal;
  double penetrationGameUnits{};
  double normalImpulseGameUnits{};
  bool trigger{};
};

struct RaycastQuery {
  Vec3 from;
  Vec3 to;
  CollisionGroup group{CollisionGroup::Default};
  CollisionMask mask{collisionMask(CollisionGroup::All)};
  bool includeTriggers{true};
  BodyId ignoreBody{};
};

struct RaycastHit {
  BodyId body{};
  BodyMetadata metadata;
  Vec3 point;
  Vec3 normal;
  double fraction{};
  bool trigger{};
};

namespace detail {
class PhysicsBackend;
}

class BodyHandle final {
public:
  BodyHandle() noexcept = default;
  ~BodyHandle();
  BodyHandle(BodyHandle &&other) noexcept;
  BodyHandle &operator=(BodyHandle &&other) noexcept;
  BodyHandle(const BodyHandle &) = delete;
  BodyHandle &operator=(const BodyHandle &) = delete;

  [[nodiscard]] BodyId id() const noexcept;
  [[nodiscard]] bool valid() const noexcept;
  void reset() noexcept;

private:
  BodyHandle(std::weak_ptr<detail::PhysicsBackend> backend, BodyId id) noexcept;

  std::weak_ptr<detail::PhysicsBackend> backend_;
  BodyId id_{};

  friend class detail::PhysicsBackend;
  friend class PhysicsWorld;
};

class Constraint final {
public:
  Constraint() noexcept = default;
  ~Constraint();
  Constraint(Constraint &&other) noexcept;
  Constraint &operator=(Constraint &&other) noexcept;
  Constraint(const Constraint &) = delete;
  Constraint &operator=(const Constraint &) = delete;

  [[nodiscard]] ConstraintId id() const noexcept;
  [[nodiscard]] bool valid() const noexcept;
  void reset() noexcept;

private:
  Constraint(std::weak_ptr<detail::PhysicsBackend> backend,
             ConstraintId id) noexcept;

  std::weak_ptr<detail::PhysicsBackend> backend_;
  ConstraintId id_{};

  friend class detail::PhysicsBackend;
  friend class PhysicsWorld;
};

class PhysicsWorld final {
public:
  static constexpr double fixedStepSeconds = 1.0 / 60.0;

  PhysicsWorld() noexcept = default;
  ~PhysicsWorld();
  PhysicsWorld(PhysicsWorld &&) noexcept;
  PhysicsWorld &operator=(PhysicsWorld &&) noexcept;
  PhysicsWorld(const PhysicsWorld &) = delete;
  PhysicsWorld &operator=(const PhysicsWorld &) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] const UnitConversion &units() const;
  [[nodiscard]] BodyHandle createBody(const BodyDesc &description);
  [[nodiscard]] Constraint
  createPointConstraint(const BodyHandle &first, const BodyHandle &second,
                        Vec3 firstPivotGameUnits,
                        Vec3 secondPivotGameUnits,
                        bool disableLinkedCollision = true);
  [[nodiscard]] Constraint
  createHingeConstraint(const BodyHandle &first, const BodyHandle &second,
                        Vec3 firstPivotGameUnits,
                        Vec3 secondPivotGameUnits, Vec3 firstAxis,
                        Vec3 secondAxis, double lowerLimitRadians,
                        double upperLimitRadians,
                        bool disableLinkedCollision = true);

  [[nodiscard]] std::size_t bodyCount() const noexcept;
  [[nodiscard]] std::size_t constraintCount() const noexcept;
  [[nodiscard]] StepResult advance(double frameSeconds);
  [[nodiscard]] Transform transform(const BodyHandle &body) const;
  [[nodiscard]] Transform
  interpolatedTransform(const BodyHandle &body) const;
  void setTransform(const BodyHandle &body, const Transform &transform);
  [[nodiscard]] Vec3 linearVelocity(const BodyHandle &body) const;
  void setLinearVelocity(const BodyHandle &body, Vec3 velocityGameUnits);
  void applyCentralForce(const BodyHandle &body, Vec3 forceGameUnits);
  void applyCentralImpulse(const BodyHandle &body, Vec3 impulseGameUnits);
  void clearForces(const BodyHandle &body);
  void setSleepingAllowed(const BodyHandle &body, bool allowed);
  void sleepBody(const BodyHandle &body);
  void wakeBody(const BodyHandle &body);
  [[nodiscard]] bool isSleeping(const BodyHandle &body) const;
  void setEnabled(const BodyHandle &body, bool enabled);
  [[nodiscard]] bool isEnabled(const BodyHandle &body) const;

  [[nodiscard]] std::vector<RaycastHit>
  raycastAll(const RaycastQuery &query) const;
  [[nodiscard]] std::optional<RaycastHit>
  raycastClosest(const RaycastQuery &query) const;
  [[nodiscard]] std::vector<ContactEvent> drainContactEvents();

private:
  explicit PhysicsWorld(std::shared_ptr<detail::PhysicsBackend> backend);
  [[nodiscard]] BodyId checkedBody(const BodyHandle &body) const;

  std::shared_ptr<detail::PhysicsBackend> backend_;

  friend PhysicsWorld createBulletPhysicsWorld(const PhysicsConfig &);
  friend PhysicsWorld createNullPhysicsWorld(const PhysicsConfig &);
  friend struct testing_PhysicsTestAccess;
};

[[nodiscard]] PhysicsWorld
createBulletPhysicsWorld(const PhysicsConfig &config = {});
[[nodiscard]] PhysicsWorld
createNullPhysicsWorld(const PhysicsConfig &config = {});

} // namespace run3::physics
