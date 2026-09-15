#include "FixedStep.hpp"
#include "PhysicsBackend.hpp"

#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace run3::physics::detail {

namespace {

struct NullBody {
  Transform transform;
  Vec3 velocity;
  BodyMotion motion{BodyMotion::Static};
  bool sleeping{};
  bool sleepingAllowed{true};
  bool enabled{true};
};

struct NullConstraint {
  BodyId first{};
  BodyId second{};
};

class NullPhysicsBackend final : public PhysicsBackend {
public:
  NullPhysicsBackend(const PhysicsConfig &config, UnitConversion units)
      : units_(std::move(units)), accumulator_(config.maxCatchUpSteps) {}

  const UnitConversion &units() const noexcept override { return units_; }

  BodyHandle createBody(const BodyDesc &description) override {
    const BodyId id = nextBody_++;
    bodies_.emplace(id, NullBody{description.transform,
                                 description.linearVelocity,
                                 description.motion, false,
                                 description.sleepingAllowed, true});
    return makeBodyHandle(id);
  }

  Constraint createPointConstraint(BodyId first, BodyId second, Vec3, Vec3,
                                   bool) override {
    requireBody(first);
    requireBody(second);
    const ConstraintId id = nextConstraint_++;
    constraints_.emplace(id, NullConstraint{first, second});
    return makeConstraintHandle(id);
  }

  Constraint createHingeConstraint(BodyId first, BodyId second, Vec3, Vec3,
                                   Vec3, Vec3, double, double,
                                   bool) override {
    requireBody(first);
    requireBody(second);
    const ConstraintId id = nextConstraint_++;
    constraints_.emplace(id, NullConstraint{first, second});
    return makeConstraintHandle(id);
  }

  void destroyBody(BodyId id) noexcept override {
    bodies_.erase(id);
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second.first == id || iterator->second.second == id) {
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
  }

  void destroyConstraint(ConstraintId id) noexcept override {
    constraints_.erase(id);
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
    return accumulator_.advance(frameSeconds, [](double) {});
  }
  Transform transform(BodyId id) const override { return requireBody(id).transform; }
  Transform interpolatedTransform(BodyId id) const override {
    return requireBody(id).transform;
  }
  void setTransform(BodyId id, const Transform &value) override {
    requireBody(id).transform = value;
  }
  Vec3 linearVelocity(BodyId id) const override {
    return requireBody(id).velocity;
  }
  void setLinearVelocity(BodyId id, Vec3 velocity) override {
    requireBody(id).velocity = velocity;
  }
  void applyCentralForce(BodyId id, Vec3) override {
    requireDynamicBody(id).sleeping = false;
  }
  void applyCentralImpulse(BodyId id, Vec3) override {
    requireDynamicBody(id).sleeping = false;
  }
  void clearForces(BodyId id) override { requireBody(id); }
  void setSleepingAllowed(BodyId id, bool allowed) override {
    auto &body = requireDynamicBody(id);
    body.sleepingAllowed = allowed;
    if (!allowed) {
      body.sleeping = false;
    }
  }
  void sleepBody(BodyId id) override {
    auto &body = requireDynamicBody(id);
    if (body.sleepingAllowed) {
      body.sleeping = true;
    }
  }
  void wakeBody(BodyId id) override { requireDynamicBody(id).sleeping = false; }
  bool isSleeping(BodyId id) const override { return requireBody(id).sleeping; }
  void setEnabled(BodyId id, bool enabled) override {
    requireBody(id).enabled = enabled;
  }
  bool isEnabled(BodyId id) const override { return requireBody(id).enabled; }
  std::vector<RaycastHit> raycastAll(const RaycastQuery &) const override {
    return {};
  }
  std::vector<ContactEvent> drainContactEvents() override { return {}; }

private:
  NullBody &requireBody(BodyId id) {
    const auto found = bodies_.find(id);
    if (found == bodies_.end()) {
      throw std::invalid_argument("unknown null-backend body");
    }
    return found->second;
  }
  const NullBody &requireBody(BodyId id) const {
    const auto found = bodies_.find(id);
    if (found == bodies_.end()) {
      throw std::invalid_argument("unknown null-backend body");
    }
    return found->second;
  }
  NullBody &requireDynamicBody(BodyId id) {
    NullBody &body = requireBody(id);
    if (body.motion != BodyMotion::Dynamic) {
      throw std::invalid_argument("operation requires a dynamic body");
    }
    return body;
  }

  UnitConversion units_;
  FixedStepAccumulator accumulator_;
  BodyId nextBody_{1};
  ConstraintId nextConstraint_{1};
  std::unordered_map<BodyId, NullBody> bodies_;
  std::unordered_map<ConstraintId, NullConstraint> constraints_;
};

} // namespace

std::shared_ptr<PhysicsBackend>
makeNullBackend(const PhysicsConfig &config, UnitConversion units) {
  return std::make_shared<NullPhysicsBackend>(config, std::move(units));
}

} // namespace run3::physics::detail
