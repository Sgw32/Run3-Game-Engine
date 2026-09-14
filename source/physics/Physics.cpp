#include <run3/physics/Physics.hpp>
#include <run3/physics/PhysicsTesting.hpp>

#include "PhysicsBackend.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace run3::physics {

namespace {

bool finite(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool finite(Quaternion value) {
  return std::isfinite(value.w) && std::isfinite(value.x) &&
         std::isfinite(value.y) && std::isfinite(value.z);
}

void requirePositive(Vec3 value, const char *what) {
  if (!finite(value) || value.x <= 0.0 || value.y <= 0.0 || value.z <= 0.0) {
    throw std::invalid_argument(std::string(what) +
                                " must have finite positive components");
  }
}

void validateConfig(const PhysicsConfig &config) {
  if (!finite(config.gravity)) {
    throw std::invalid_argument("physics gravity must be finite");
  }
  if (config.maxCatchUpSteps == 0) {
    throw std::invalid_argument("maxCatchUpSteps must be greater than zero");
  }
}

void validateBody(const BodyDesc &description) {
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
  if (!finite(description.transform.position) ||
      !finite(description.transform.rotation) ||
      !finite(description.linearVelocity) ||
      !finite(description.angularFactor) ||
      !std::isfinite(description.friction) || description.friction < 0.0) {
    throw std::invalid_argument("body transform and velocity must be finite");
  }
  const Quaternion rotation = description.transform.rotation;
  const double rotationLength =
      rotation.w * rotation.w + rotation.x * rotation.x +
      rotation.y * rotation.y + rotation.z * rotation.z;
  if (rotationLength <= 1e-24) {
    throw std::invalid_argument("body rotation quaternion must be non-zero");
  }
}

} // namespace

UnitConversion::UnitConversion(double metresPerGameUnit)
    : metresPerGameUnit_(metresPerGameUnit) {
  if (!std::isfinite(metresPerGameUnit_) || metresPerGameUnit_ <= 0.0) {
    throw std::invalid_argument(
        "metresPerGameUnit must be finite and greater than zero");
  }
}

UnitConversion UnitConversion::standard() noexcept {
  return UnitConversion{0.01};
}

double UnitConversion::metresPerGameUnit() const noexcept {
  return metresPerGameUnit_;
}

double UnitConversion::toMetres(double gameUnits) const noexcept {
  return gameUnits * metresPerGameUnit_;
}

double UnitConversion::toGameUnits(double metres) const noexcept {
  return metres / metresPerGameUnit_;
}

Vec3 UnitConversion::toMetres(Vec3 value) const noexcept {
  return {toMetres(value.x), toMetres(value.y), toMetres(value.z)};
}

Vec3 UnitConversion::toGameUnits(Vec3 value) const noexcept {
  return {toGameUnits(value.x), toGameUnits(value.y), toGameUnits(value.z)};
}

Shape::Shape(Type type) noexcept : type_(type) {}

Shape Shape::box(Vec3 halfExtentsGameUnits) {
  requirePositive(halfExtentsGameUnits, "box half extents");
  Shape result{Type::Box};
  result.halfExtents_ = halfExtentsGameUnits;
  return result;
}

Shape Shape::capsule(double radiusGameUnits, double cylinderHeightGameUnits) {
  if (!std::isfinite(radiusGameUnits) || radiusGameUnits <= 0.0 ||
      !std::isfinite(cylinderHeightGameUnits) || cylinderHeightGameUnits < 0.0) {
    throw std::invalid_argument(
        "capsule radius must be positive and cylinder height non-negative");
  }
  Shape result{Type::Capsule};
  result.radius_ = radiusGameUnits;
  result.cylinderHeight_ = cylinderHeightGameUnits;
  return result;
}

Shape Shape::triangleMesh(std::vector<Vec3> verticesGameUnits,
                          std::vector<std::uint32_t> triangleIndices) {
  if (verticesGameUnits.size() < 3 || triangleIndices.empty() ||
      triangleIndices.size() % 3 != 0) {
    throw std::invalid_argument(
        "triangle mesh requires vertices and complete triangle indices");
  }
  if (std::any_of(verticesGameUnits.begin(), verticesGameUnits.end(),
                  [](Vec3 vertex) { return !finite(vertex); })) {
    throw std::invalid_argument("triangle mesh vertices must be finite");
  }
  if (std::any_of(triangleIndices.begin(), triangleIndices.end(),
                  [&](std::uint32_t index) {
                    return index >= verticesGameUnits.size();
                  })) {
    throw std::invalid_argument("triangle mesh index is out of range");
  }
  Shape result{Type::TriangleMesh};
  result.vertices_ = std::move(verticesGameUnits);
  result.indices_ = std::move(triangleIndices);
  return result;
}

Shape::Type Shape::type() const noexcept { return type_; }
Vec3 Shape::halfExtents() const noexcept { return halfExtents_; }
double Shape::radius() const noexcept { return radius_; }
double Shape::cylinderHeight() const noexcept { return cylinderHeight_; }
const std::vector<Vec3> &Shape::vertices() const noexcept { return vertices_; }
const std::vector<std::uint32_t> &Shape::indices() const noexcept {
  return indices_;
}

BodyHandle::BodyHandle(std::weak_ptr<detail::PhysicsBackend> backend,
                       BodyId id) noexcept
    : backend_(std::move(backend)), id_(id) {}

BodyHandle::~BodyHandle() { reset(); }

BodyHandle::BodyHandle(BodyHandle &&other) noexcept
    : backend_(std::move(other.backend_)), id_(std::exchange(other.id_, 0)) {}

BodyHandle &BodyHandle::operator=(BodyHandle &&other) noexcept {
  if (this != &other) {
    reset();
    backend_ = std::move(other.backend_);
    id_ = std::exchange(other.id_, 0);
  }
  return *this;
}

BodyId BodyHandle::id() const noexcept { return id_; }

bool BodyHandle::valid() const noexcept {
  const auto backend = backend_.lock();
  return id_ != 0 && backend != nullptr && backend->hasBody(id_);
}

void BodyHandle::reset() noexcept {
  if (id_ != 0) {
    if (const auto backend = backend_.lock()) {
      backend->destroyBody(id_);
    }
  }
  id_ = 0;
  backend_.reset();
}

Constraint::Constraint(std::weak_ptr<detail::PhysicsBackend> backend,
                       ConstraintId id) noexcept
    : backend_(std::move(backend)), id_(id) {}

Constraint::~Constraint() { reset(); }

Constraint::Constraint(Constraint &&other) noexcept
    : backend_(std::move(other.backend_)), id_(std::exchange(other.id_, 0)) {}

Constraint &Constraint::operator=(Constraint &&other) noexcept {
  if (this != &other) {
    reset();
    backend_ = std::move(other.backend_);
    id_ = std::exchange(other.id_, 0);
  }
  return *this;
}

ConstraintId Constraint::id() const noexcept { return id_; }

bool Constraint::valid() const noexcept {
  const auto backend = backend_.lock();
  return id_ != 0 && backend != nullptr && backend->hasConstraint(id_);
}

void Constraint::reset() noexcept {
  if (id_ != 0) {
    if (const auto backend = backend_.lock()) {
      backend->destroyConstraint(id_);
    }
  }
  id_ = 0;
  backend_.reset();
}

PhysicsWorld::PhysicsWorld(std::shared_ptr<detail::PhysicsBackend> backend)
    : backend_(std::move(backend)) {}
PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld &&) noexcept = default;
PhysicsWorld &PhysicsWorld::operator=(PhysicsWorld &&) noexcept = default;

bool PhysicsWorld::valid() const noexcept { return backend_ != nullptr; }

const UnitConversion &PhysicsWorld::units() const {
  if (!backend_) {
    throw std::logic_error("physics world is not initialized");
  }
  return backend_->units();
}

BodyId PhysicsWorld::checkedBody(const BodyHandle &body) const {
  if (!backend_ || body.id_ == 0 || body.backend_.lock() != backend_ ||
      !backend_->hasBody(body.id_)) {
    throw std::invalid_argument("body handle does not belong to this world");
  }
  return body.id_;
}

BodyHandle PhysicsWorld::createBody(const BodyDesc &description) {
  if (!backend_) {
    throw std::logic_error("physics world is not initialized");
  }
  validateBody(description);
  return backend_->createBody(description);
}

Constraint PhysicsWorld::createPointConstraint(
    const BodyHandle &first, const BodyHandle &second, Vec3 firstPivot,
    Vec3 secondPivot, bool disableLinkedCollision) {
  return backend_->createPointConstraint(checkedBody(first), checkedBody(second),
                                         firstPivot, secondPivot,
                                         disableLinkedCollision);
}

std::size_t PhysicsWorld::bodyCount() const noexcept {
  return backend_ ? backend_->bodyCount() : 0;
}

std::size_t PhysicsWorld::constraintCount() const noexcept {
  return backend_ ? backend_->constraintCount() : 0;
}

StepResult PhysicsWorld::advance(double frameSeconds) {
  if (!backend_) {
    throw std::logic_error("physics world is not initialized");
  }
  return backend_->advance(frameSeconds);
}

Transform PhysicsWorld::transform(const BodyHandle &body) const {
  return backend_->transform(checkedBody(body));
}

Transform PhysicsWorld::interpolatedTransform(const BodyHandle &body) const {
  return backend_->interpolatedTransform(checkedBody(body));
}

void PhysicsWorld::setTransform(const BodyHandle &body, const Transform &value) {
  backend_->setTransform(checkedBody(body), value);
}

Vec3 PhysicsWorld::linearVelocity(const BodyHandle &body) const {
  return backend_->linearVelocity(checkedBody(body));
}

void PhysicsWorld::setLinearVelocity(const BodyHandle &body, Vec3 velocity) {
  backend_->setLinearVelocity(checkedBody(body), velocity);
}

void PhysicsWorld::applyCentralForce(const BodyHandle &body, Vec3 force) {
  backend_->applyCentralForce(checkedBody(body), force);
}

void PhysicsWorld::applyCentralImpulse(const BodyHandle &body, Vec3 impulse) {
  backend_->applyCentralImpulse(checkedBody(body), impulse);
}

void PhysicsWorld::clearForces(const BodyHandle &body) {
  backend_->clearForces(checkedBody(body));
}

void PhysicsWorld::setSleepingAllowed(const BodyHandle &body, bool allowed) {
  backend_->setSleepingAllowed(checkedBody(body), allowed);
}

void PhysicsWorld::sleepBody(const BodyHandle &body) {
  backend_->sleepBody(checkedBody(body));
}

void PhysicsWorld::wakeBody(const BodyHandle &body) {
  backend_->wakeBody(checkedBody(body));
}

bool PhysicsWorld::isSleeping(const BodyHandle &body) const {
  return backend_->isSleeping(checkedBody(body));
}

void PhysicsWorld::setEnabled(const BodyHandle &body, bool enabled) {
  backend_->setEnabled(checkedBody(body), enabled);
}

bool PhysicsWorld::isEnabled(const BodyHandle &body) const {
  return backend_->isEnabled(checkedBody(body));
}

std::vector<RaycastHit>
PhysicsWorld::raycastAll(const RaycastQuery &query) const {
  if (!backend_) {
    throw std::logic_error("physics world is not initialized");
  }
  return backend_->raycastAll(query);
}

std::optional<RaycastHit>
PhysicsWorld::raycastClosest(const RaycastQuery &query) const {
  auto hits = raycastAll(query);
  if (hits.empty()) {
    return std::nullopt;
  }
  return hits.front();
}

std::vector<ContactEvent> PhysicsWorld::drainContactEvents() {
  if (!backend_) {
    throw std::logic_error("physics world is not initialized");
  }
  return backend_->drainContactEvents();
}

PhysicsWorld createBulletPhysicsWorld(const PhysicsConfig &config) {
  validateConfig(config);
  return PhysicsWorld{
      detail::makeBulletBackend(config, UnitConversion::standard())};
}

PhysicsWorld createNullPhysicsWorld(const PhysicsConfig &config) {
  validateConfig(config);
  return PhysicsWorld{detail::makeNullBackend(config, UnitConversion::standard())};
}

UnitConversion testing_PhysicsTestAccess::unitConversion(
    double metresPerGameUnit) {
  return UnitConversion{metresPerGameUnit};
}

PhysicsWorld testing_PhysicsTestAccess::createBulletWorld(
    const PhysicsConfig &config, UnitConversion units) {
  validateConfig(config);
  return PhysicsWorld{detail::makeBulletBackend(config, std::move(units))};
}

PhysicsWorld testing_PhysicsTestAccess::createNullWorld(
    const PhysicsConfig &config, UnitConversion units) {
  validateConfig(config);
  return PhysicsWorld{detail::makeNullBackend(config, std::move(units))};
}

} // namespace run3::physics
