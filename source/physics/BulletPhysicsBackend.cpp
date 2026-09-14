#include "FixedStep.hpp"
#include "PhysicsBackend.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace run3::physics::detail {

namespace {

btVector3 toBullet(Vec3 value, const UnitConversion &units) {
  const Vec3 metres = units.toMetres(value);
  return {static_cast<btScalar>(metres.x), static_cast<btScalar>(metres.y),
          static_cast<btScalar>(metres.z)};
}

Vec3 fromBullet(const btVector3 &value, const UnitConversion &units) {
  return units.toGameUnits({static_cast<double>(value.x()),
                            static_cast<double>(value.y()),
                            static_cast<double>(value.z())});
}

btQuaternion toBullet(Quaternion value) {
  const double length = std::sqrt(value.w * value.w + value.x * value.x +
                                  value.y * value.y + value.z * value.z);
  if (!std::isfinite(length) || length <= 1e-12) {
    throw std::invalid_argument("body rotation must be a finite quaternion");
  }
  return {static_cast<btScalar>(value.x / length),
          static_cast<btScalar>(value.y / length),
          static_cast<btScalar>(value.z / length),
          static_cast<btScalar>(value.w / length)};
}

Quaternion fromBullet(const btQuaternion &value) {
  return {static_cast<double>(value.w()), static_cast<double>(value.x()),
          static_cast<double>(value.y()), static_cast<double>(value.z())};
}

btTransform toBullet(const Transform &value, const UnitConversion &units) {
  btTransform result;
  result.setIdentity();
  result.setOrigin(toBullet(value.position, units));
  result.setRotation(toBullet(value.rotation));
  return result;
}

Transform fromBullet(const btTransform &value, const UnitConversion &units) {
  return {fromBullet(value.getOrigin(), units),
          fromBullet(value.getRotation())};
}

Quaternion interpolate(Quaternion first, Quaternion second, double alpha) {
  double dot = first.w * second.w + first.x * second.x + first.y * second.y +
               first.z * second.z;
  if (dot < 0.0) {
    second = {-second.w, -second.x, -second.y, -second.z};
  }
  Quaternion result{first.w + (second.w - first.w) * alpha,
                    first.x + (second.x - first.x) * alpha,
                    first.y + (second.y - first.y) * alpha,
                    first.z + (second.z - first.z) * alpha};
  const double length = std::sqrt(result.w * result.w + result.x * result.x +
                                  result.y * result.y + result.z * result.z);
  if (length > 1e-12) {
    result.w /= length;
    result.x /= length;
    result.y /= length;
    result.z /= length;
  }
  return result;
}

Transform interpolate(const Transform &first, const Transform &second,
                      double alpha) {
  return {{first.position.x + (second.position.x - first.position.x) * alpha,
           first.position.y + (second.position.y - first.position.y) * alpha,
           first.position.z + (second.position.z - first.position.z) * alpha},
          interpolate(first.rotation, second.rotation, alpha)};
}

short filterBits(CollisionMask value) {
  return static_cast<short>(value & 0xffffU);
}

struct BodyRecord {
  BodyId id{};
  BodyMetadata metadata;
  CollisionGroup group{CollisionGroup::Dynamic};
  CollisionMask mask{collisionMask(CollisionGroup::All)};
  bool trigger{};
  bool enabled{true};
  Transform previous;
  Transform current;
  std::unique_ptr<btTriangleMesh> triangleMesh;
  std::unique_ptr<btCollisionShape> shape;
  std::unique_ptr<btRigidBody> body;
};

struct ConstraintRecord {
  ConstraintId id{};
  BodyId first{};
  BodyId second{};
  std::unique_ptr<btTypedConstraint> constraint;
};

struct BodyPair {
  BodyId first{};
  BodyId second{};

  friend bool operator==(const BodyPair &left, const BodyPair &right) {
    return left.first == right.first && left.second == right.second;
  }
};

struct BodyPairHash {
  std::size_t operator()(const BodyPair &pair) const noexcept {
    const auto first = std::hash<BodyId>{}(pair.first);
    const auto second = std::hash<BodyId>{}(pair.second);
    return first ^ (second + 0x9e3779b9U + (first << 6U) + (first >> 2U));
  }
};

class BulletPhysicsBackend final : public PhysicsBackend {
public:
  BulletPhysicsBackend(const PhysicsConfig &config, UnitConversion units)
      : units_(std::move(units)), accumulator_(config.maxCatchUpSteps),
        collisionConfiguration_(std::make_unique<btDefaultCollisionConfiguration>()),
        dispatcher_(std::make_unique<btCollisionDispatcher>(
            collisionConfiguration_.get())),
        broadphase_(std::make_unique<btDbvtBroadphase>()),
        solver_(std::make_unique<btSequentialImpulseConstraintSolver>()),
        world_(std::make_unique<btDiscreteDynamicsWorld>(
            dispatcher_.get(), broadphase_.get(), solver_.get(),
            collisionConfiguration_.get())) {
    world_->setGravity(toBullet(config.gravity, units_));
  }

  ~BulletPhysicsBackend() override {
    while (!constraints_.empty()) {
      destroyConstraint(constraints_.begin()->first);
    }
    while (!bodies_.empty()) {
      destroyBody(bodies_.begin()->first);
    }
  }

  const UnitConversion &units() const noexcept override { return units_; }

  BodyHandle createBody(const BodyDesc &description) override {
    if (!std::isfinite(description.massKg) || description.massKg < 0.0) {
      throw std::invalid_argument("body mass must be finite and non-negative");
    }
    if (description.motion == BodyMotion::Dynamic && description.massKg <= 0.0) {
      throw std::invalid_argument("dynamic body mass must be greater than zero");
    }
    if (description.motion != BodyMotion::Dynamic && description.massKg != 0.0) {
      throw std::invalid_argument("static and kinematic bodies must have zero mass");
    }
    if (description.shape.type() == Shape::Type::TriangleMesh &&
        description.motion == BodyMotion::Dynamic) {
      throw std::invalid_argument("triangle mesh bodies cannot be dynamic");
    }

    auto record = std::make_unique<BodyRecord>();
    record->id = nextBody_++;
    record->metadata = description.metadata;
    record->group = description.group;
    record->mask = description.mask;
    record->trigger = description.trigger;
    record->previous = description.transform;
    record->current = description.transform;
    record->shape = makeShape(description.shape, record->triangleMesh);

    const btScalar mass = description.motion == BodyMotion::Dynamic
                              ? static_cast<btScalar>(description.massKg)
                              : btScalar(0);
    btVector3 inertia(0, 0, 0);
    if (mass > btScalar(0)) {
      record->shape->calculateLocalInertia(mass, inertia);
    }
    btRigidBody::btRigidBodyConstructionInfo construction(
        mass, nullptr, record->shape.get(), inertia);
    record->body = std::make_unique<btRigidBody>(construction);
    record->body->setWorldTransform(toBullet(description.transform, units_));
    record->body->setInterpolationWorldTransform(
        record->body->getWorldTransform());
    record->body->setLinearVelocity(toBullet(description.linearVelocity, units_));
    record->body->setUserPointer(record.get());

    int flags = record->body->getCollisionFlags();
    if (description.motion == BodyMotion::Kinematic) {
      flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
      record->body->setActivationState(DISABLE_DEACTIVATION);
    } else if (!description.sleepingAllowed &&
               description.motion == BodyMotion::Dynamic) {
      record->body->setActivationState(DISABLE_DEACTIVATION);
    }
    if (description.trigger) {
      flags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    record->body->setCollisionFlags(flags);

    const BodyId id = record->id;
    world_->addRigidBody(record->body.get(), filterBits(collisionMask(record->group)),
                         filterBits(record->mask));
    bodies_.emplace(id, std::move(record));
    return makeBodyHandle(id);
  }

  Constraint createPointConstraint(BodyId first, BodyId second, Vec3 firstPivot,
                                   Vec3 secondPivot,
                                   bool disableLinkedCollision) override {
    BodyRecord &firstBody = requireBody(first);
    BodyRecord &secondBody = requireBody(second);
    auto record = std::make_unique<ConstraintRecord>();
    record->id = nextConstraint_++;
    record->first = first;
    record->second = second;
    record->constraint = std::make_unique<btPoint2PointConstraint>(
        *firstBody.body, *secondBody.body, toBullet(firstPivot, units_),
        toBullet(secondPivot, units_));
    world_->addConstraint(record->constraint.get(), disableLinkedCollision);
    const ConstraintId id = record->id;
    constraints_.emplace(id, std::move(record));
    return makeConstraintHandle(id);
  }

  void destroyBody(BodyId id) noexcept override {
    const auto found = bodies_.find(id);
    if (found == bodies_.end()) {
      return;
    }
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second->first == id || iterator->second->second == id) {
        world_->removeConstraint(iterator->second->constraint.get());
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    for (auto iterator = activeContacts_.begin();
         iterator != activeContacts_.end();) {
      if (iterator->first.first == id || iterator->first.second == id) {
        ContactEvent ended = iterator->second;
        ended.phase = ContactPhase::Ended;
        try {
          contactQueue_.push_back(ended);
        } catch (...) {
          // Destruction and handle reset are noexcept. A missing terminal event
          // under allocation failure is safer than terminating during cleanup.
        }
        iterator = activeContacts_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    world_->removeRigidBody(found->second->body.get());
    bodies_.erase(found);
  }

  void destroyConstraint(ConstraintId id) noexcept override {
    const auto found = constraints_.find(id);
    if (found != constraints_.end()) {
      world_->removeConstraint(found->second->constraint.get());
      constraints_.erase(found);
    }
  }

  bool hasBody(BodyId id) const noexcept override {
    return bodies_.find(id) != bodies_.end();
  }
  bool hasConstraint(ConstraintId id) const noexcept override {
    return constraints_.find(id) != constraints_.end();
  }
  std::size_t bodyCount() const noexcept override { return bodies_.size(); }
  std::size_t constraintCount() const noexcept override {
    return constraints_.size();
  }

  StepResult advance(double frameSeconds) override {
    return accumulator_.advance(frameSeconds,
                                [this](double step) { fixedStep(step); });
  }

  Transform transform(BodyId id) const override { return requireBody(id).current; }

  Transform interpolatedTransform(BodyId id) const override {
    const BodyRecord &record = requireBody(id);
    return interpolate(record.previous, record.current,
                       accumulator_.interpolationAlpha());
  }

  void setTransform(BodyId id, const Transform &value) override {
    BodyRecord &record = requireBody(id);
    const btTransform bulletTransform = toBullet(value, units_);
    record.body->setWorldTransform(bulletTransform);
    record.body->setInterpolationWorldTransform(bulletTransform);
    record.previous = value;
    record.current = value;
    record.body->activate(true);
    world_->updateSingleAabb(record.body.get());
  }

  Vec3 linearVelocity(BodyId id) const override {
    return fromBullet(requireBody(id).body->getLinearVelocity(), units_);
  }

  void setLinearVelocity(BodyId id, Vec3 velocity) override {
    BodyRecord &record = requireBody(id);
    record.body->setLinearVelocity(toBullet(velocity, units_));
    record.body->activate(true);
  }

  void applyCentralForce(BodyId id, Vec3 force) override {
    BodyRecord &record = requireDynamicBody(id);
    record.body->applyCentralForce(toBullet(force, units_));
    record.body->activate(true);
  }

  void applyCentralImpulse(BodyId id, Vec3 impulse) override {
    BodyRecord &record = requireDynamicBody(id);
    record.body->applyCentralImpulse(toBullet(impulse, units_));
    record.body->activate(true);
  }

  void clearForces(BodyId id) override { requireBody(id).body->clearForces(); }

  void setSleepingAllowed(BodyId id, bool allowed) override {
    BodyRecord &record = requireDynamicBody(id);
    if (allowed) {
      record.body->forceActivationState(ACTIVE_TAG);
      record.body->setDeactivationTime(0);
    } else {
      record.body->forceActivationState(DISABLE_DEACTIVATION);
    }
  }

  void sleepBody(BodyId id) override {
    BodyRecord &record = requireDynamicBody(id);
    record.body->setLinearVelocity({0, 0, 0});
    record.body->setAngularVelocity({0, 0, 0});
    record.body->clearForces();
    record.body->forceActivationState(ISLAND_SLEEPING);
  }

  void wakeBody(BodyId id) override { requireDynamicBody(id).body->activate(true); }

  bool isSleeping(BodyId id) const override {
    return requireBody(id).body->getActivationState() == ISLAND_SLEEPING;
  }

  void setEnabled(BodyId id, bool enabled) override {
    BodyRecord &record = requireBody(id);
    record.enabled = enabled;
    if (enabled) {
      record.body->forceActivationState(ACTIVE_TAG);
      world_->updateSingleAabb(record.body.get());
    } else {
      record.body->forceActivationState(DISABLE_SIMULATION);
    }
  }

  bool isEnabled(BodyId id) const override { return requireBody(id).enabled; }

  std::vector<RaycastHit>
  raycastAll(const RaycastQuery &query) const override {
    const btVector3 from = toBullet(query.from, units_);
    const btVector3 to = toBullet(query.to, units_);
    btCollisionWorld::AllHitsRayResultCallback callback(from, to);
    callback.m_collisionFilterGroup = filterBits(collisionMask(query.group));
    callback.m_collisionFilterMask = filterBits(query.mask);
    world_->rayTest(from, to, callback);

    std::vector<RaycastHit> result;
    if (!callback.hasHit()) {
      return result;
    }
    result.reserve(static_cast<std::size_t>(callback.m_collisionObjects.size()));
    for (int index = 0; index < callback.m_collisionObjects.size(); ++index) {
      const auto *record = static_cast<const BodyRecord *>(
          callback.m_collisionObjects[index]->getUserPointer());
      if (record == nullptr || !record->enabled ||
          (!query.includeTriggers && record->trigger)) {
        continue;
      }
      result.push_back(
          {record->id,
           record->metadata,
           fromBullet(callback.m_hitPointWorld[index], units_),
           {static_cast<double>(callback.m_hitNormalWorld[index].x()),
            static_cast<double>(callback.m_hitNormalWorld[index].y()),
            static_cast<double>(callback.m_hitNormalWorld[index].z())},
           static_cast<double>(callback.m_hitFractions[index]),
           record->trigger});
    }
    std::sort(result.begin(), result.end(),
              [](const RaycastHit &left, const RaycastHit &right) {
                if (left.fraction != right.fraction) {
                  return left.fraction < right.fraction;
                }
                return left.body < right.body;
              });
    return result;
  }

  std::vector<ContactEvent> drainContactEvents() override {
    std::vector<ContactEvent> result;
    result.swap(contactQueue_);
    return result;
  }

private:
  std::unique_ptr<btCollisionShape>
  makeShape(const Shape &shape, std::unique_ptr<btTriangleMesh> &triangleMesh) {
    switch (shape.type()) {
    case Shape::Type::Box:
      return std::make_unique<btBoxShape>(toBullet(shape.halfExtents(), units_));
    case Shape::Type::Capsule:
      return std::make_unique<btCapsuleShape>(
          static_cast<btScalar>(units_.toMetres(shape.radius())),
          static_cast<btScalar>(units_.toMetres(shape.cylinderHeight())));
    case Shape::Type::TriangleMesh: {
      triangleMesh = std::make_unique<btTriangleMesh>(true, false);
      const auto &vertices = shape.vertices();
      const auto &indices = shape.indices();
      for (std::size_t index = 0; index < indices.size(); index += 3) {
        triangleMesh->addTriangle(toBullet(vertices[indices[index]], units_),
                                  toBullet(vertices[indices[index + 1]], units_),
                                  toBullet(vertices[indices[index + 2]], units_),
                                  true);
      }
      return std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);
    }
    }
    throw std::logic_error("unknown physics shape type");
  }

  BodyRecord &requireBody(BodyId id) {
    const auto found = bodies_.find(id);
    if (found == bodies_.end()) {
      throw std::invalid_argument("unknown Bullet body");
    }
    return *found->second;
  }
  const BodyRecord &requireBody(BodyId id) const {
    const auto found = bodies_.find(id);
    if (found == bodies_.end()) {
      throw std::invalid_argument("unknown Bullet body");
    }
    return *found->second;
  }
  BodyRecord &requireDynamicBody(BodyId id) {
    BodyRecord &record = requireBody(id);
    if (record.body->isStaticOrKinematicObject()) {
      throw std::invalid_argument("operation requires a dynamic body");
    }
    return record;
  }

  void fixedStep(double seconds) {
    for (auto &[id, record] : bodies_) {
      static_cast<void>(id);
      record->previous = record->current;
    }
    world_->stepSimulation(static_cast<btScalar>(seconds), 0,
                           static_cast<btScalar>(seconds));
    for (auto &[id, record] : bodies_) {
      static_cast<void>(id);
      record->current = fromBullet(record->body->getWorldTransform(), units_);
    }
    collectContacts();
  }

  void collectContacts() {
    std::unordered_map<BodyPair, ContactEvent, BodyPairHash> current;
    const int manifoldCount = dispatcher_->getNumManifolds();
    for (int manifoldIndex = 0; manifoldIndex < manifoldCount;
         ++manifoldIndex) {
      btPersistentManifold *manifold =
          dispatcher_->getManifoldByIndexInternal(manifoldIndex);
      auto *firstRecord = static_cast<BodyRecord *>(
          manifold->getBody0()->getUserPointer());
      auto *secondRecord = static_cast<BodyRecord *>(
          manifold->getBody1()->getUserPointer());
      if (firstRecord == nullptr || secondRecord == nullptr ||
          !firstRecord->enabled || !secondRecord->enabled) {
        continue;
      }
      const btManifoldPoint *deepest = nullptr;
      for (int pointIndex = 0; pointIndex < manifold->getNumContacts();
           ++pointIndex) {
        const btManifoldPoint &point = manifold->getContactPoint(pointIndex);
        if (point.getDistance() <= btScalar(0) &&
            (deepest == nullptr || point.getDistance() < deepest->getDistance())) {
          deepest = &point;
        }
      }
      if (deepest == nullptr) {
        continue;
      }

      bool swapped = firstRecord->id > secondRecord->id;
      BodyRecord *first = swapped ? secondRecord : firstRecord;
      BodyRecord *second = swapped ? firstRecord : secondRecord;
      btVector3 normal = deepest->m_normalWorldOnB;
      if (swapped) {
        normal = -normal;
      }
      const BodyPair pair{first->id, second->id};
      ContactEvent event{
          activeContacts_.find(pair) == activeContacts_.end()
              ? ContactPhase::Began
              : ContactPhase::Persisted,
          first->id,
          second->id,
          first->metadata,
          second->metadata,
          fromBullet(deepest->getPositionWorldOnB(), units_),
          {static_cast<double>(normal.x()), static_cast<double>(normal.y()),
           static_cast<double>(normal.z())},
          units_.toGameUnits(-static_cast<double>(deepest->getDistance())),
          units_.toGameUnits(
              static_cast<double>(deepest->getAppliedImpulse())),
          first->trigger || second->trigger};
      current[pair] = event;
    }

    std::vector<std::pair<BodyPair, ContactEvent>> orderedCurrent(
        current.begin(), current.end());
    std::sort(orderedCurrent.begin(), orderedCurrent.end(),
              [](const auto &left, const auto &right) {
                return left.first.first < right.first.first ||
                       (left.first.first == right.first.first &&
                        left.first.second < right.first.second);
              });
    for (const auto &[pair, event] : orderedCurrent) {
      static_cast<void>(pair);
      contactQueue_.push_back(event);
    }
    std::vector<std::pair<BodyPair, ContactEvent>> endedEvents;
    for (const auto &[pair, prior] : activeContacts_) {
      if (current.find(pair) == current.end()) {
        ContactEvent ended = prior;
        ended.phase = ContactPhase::Ended;
        endedEvents.emplace_back(pair, ended);
      }
    }
    std::sort(endedEvents.begin(), endedEvents.end(),
              [](const auto &left, const auto &right) {
                return left.first.first < right.first.first ||
                       (left.first.first == right.first.first &&
                        left.first.second < right.first.second);
              });
    for (const auto &[pair, event] : endedEvents) {
      static_cast<void>(pair);
      contactQueue_.push_back(event);
    }
    activeContacts_ = std::move(current);
  }

  UnitConversion units_;
  FixedStepAccumulator accumulator_;
  std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration_;
  std::unique_ptr<btCollisionDispatcher> dispatcher_;
  std::unique_ptr<btBroadphaseInterface> broadphase_;
  std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
  std::unique_ptr<btDiscreteDynamicsWorld> world_;
  BodyId nextBody_{1};
  ConstraintId nextConstraint_{1};
  std::unordered_map<BodyId, std::unique_ptr<BodyRecord>> bodies_;
  std::unordered_map<ConstraintId, std::unique_ptr<ConstraintRecord>>
      constraints_;
  std::unordered_map<BodyPair, ContactEvent, BodyPairHash> activeContacts_;
  std::vector<ContactEvent> contactQueue_;
};

} // namespace

std::shared_ptr<PhysicsBackend>
makeBulletBackend(const PhysicsConfig &config, UnitConversion units) {
  return std::make_shared<BulletPhysicsBackend>(config, std::move(units));
}

} // namespace run3::physics::detail
