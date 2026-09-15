#pragma once

#include <run3/physics/Physics.hpp>

#include <memory>

namespace run3::physics::detail {

class PhysicsBackend : public std::enable_shared_from_this<PhysicsBackend> {
public:
  virtual ~PhysicsBackend() = default;

  [[nodiscard]] virtual const UnitConversion &units() const noexcept = 0;
  [[nodiscard]] virtual BodyHandle createBody(const BodyDesc &description) = 0;
  [[nodiscard]] virtual Constraint
  createPointConstraint(BodyId first, BodyId second, Vec3 firstPivot,
                        Vec3 secondPivot, bool disableLinkedCollision) = 0;
  [[nodiscard]] virtual Constraint
  createHingeConstraint(BodyId first, BodyId second, Vec3 firstPivot,
                        Vec3 secondPivot, Vec3 firstAxis, Vec3 secondAxis,
                        double lowerLimitRadians, double upperLimitRadians,
                        bool disableLinkedCollision) = 0;
  virtual void destroyBody(BodyId id) noexcept = 0;
  virtual void destroyConstraint(ConstraintId id) noexcept = 0;
  [[nodiscard]] virtual bool hasBody(BodyId id) const noexcept = 0;
  [[nodiscard]] virtual bool hasConstraint(ConstraintId id) const noexcept = 0;
  [[nodiscard]] virtual std::size_t bodyCount() const noexcept = 0;
  [[nodiscard]] virtual std::size_t constraintCount() const noexcept = 0;
  [[nodiscard]] virtual StepResult advance(double frameSeconds) = 0;
  [[nodiscard]] virtual Transform transform(BodyId id) const = 0;
  [[nodiscard]] virtual Transform interpolatedTransform(BodyId id) const = 0;
  virtual void setTransform(BodyId id, const Transform &transform) = 0;
  [[nodiscard]] virtual Vec3 linearVelocity(BodyId id) const = 0;
  virtual void setLinearVelocity(BodyId id, Vec3 velocity) = 0;
  virtual void applyCentralForce(BodyId id, Vec3 force) = 0;
  virtual void applyCentralImpulse(BodyId id, Vec3 impulse) = 0;
  virtual void clearForces(BodyId id) = 0;
  virtual void setSleepingAllowed(BodyId id, bool allowed) = 0;
  virtual void sleepBody(BodyId id) = 0;
  virtual void wakeBody(BodyId id) = 0;
  [[nodiscard]] virtual bool isSleeping(BodyId id) const = 0;
  virtual void setEnabled(BodyId id, bool enabled) = 0;
  [[nodiscard]] virtual bool isEnabled(BodyId id) const = 0;
  [[nodiscard]] virtual std::vector<RaycastHit>
  raycastAll(const RaycastQuery &query) const = 0;
  [[nodiscard]] virtual std::vector<ContactEvent> drainContactEvents() = 0;

protected:
  [[nodiscard]] BodyHandle makeBodyHandle(BodyId id) {
    return BodyHandle{weak_from_this(), id};
  }
  [[nodiscard]] Constraint makeConstraintHandle(ConstraintId id) {
    return Constraint{weak_from_this(), id};
  }
};

[[nodiscard]] std::shared_ptr<PhysicsBackend>
makeBulletBackend(const PhysicsConfig &config, UnitConversion units);
[[nodiscard]] std::shared_ptr<PhysicsBackend>
makeNullBackend(const PhysicsConfig &config, UnitConversion units);

} // namespace run3::physics::detail
