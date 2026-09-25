#include <run3/gameplay/PlayerController.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace run3::gameplay {
namespace {

physics::CollisionMask playerWorldMask() {
  return physics::collisionMask(physics::CollisionGroup::World) |
         physics::collisionMask(physics::CollisionGroup::Dynamic) |
         physics::collisionMask(physics::CollisionGroup::Npc) |
         physics::collisionMask(physics::CollisionGroup::Trigger) |
         physics::collisionMask(physics::CollisionGroup::Button) |
         physics::collisionMask(physics::CollisionGroup::Door) |
         physics::collisionMask(physics::CollisionGroup::Train) |
         physics::collisionMask(physics::CollisionGroup::Pickup);
}

double length2(double x, double z) { return std::sqrt(x * x + z * z); }

physics::Vec3 add(physics::Vec3 a, physics::Vec3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

physics::Vec3 multiply(physics::Vec3 value, double scalar) {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

} // namespace

PlayerController::PlayerController(physics::PhysicsWorld &world,
                                   PlayerConfig config)
    : world_(&world), config_(config) {
  if (!world.valid() || config_.radius <= 0.0 ||
      config_.standingHeight <= config_.radius * 2.0 ||
      config_.crouchingHeight <= config_.radius * 2.0 ||
      config_.standingHeight <= config_.crouchingHeight) {
    throw std::invalid_argument("invalid player controller configuration");
  }
}

void PlayerController::spawn(physics::Vec3 centrePosition) {
  noclipPosition_ = centrePosition;
  parentedPosition_ = centrePosition;
  parentMotion_ = {};
  parented_ = false;
  crouched_ = false;
  rebuildBody(false);
  updateGrounded();
}

void PlayerController::setCommand(PlayerCommand command) noexcept {
  command.strafe = std::clamp(command.strafe, -1.0, 1.0);
  command.forward = std::clamp(command.forward, -1.0, 1.0);
  command.vertical = std::clamp(command.vertical, -1.0, 1.0);
  command_ = command;
}

void PlayerController::setYawRadians(double yaw) noexcept { yaw_ = yaw; }

void PlayerController::rebuildBody(bool crouched) {
  physics::Vec3 position = noclipPosition_;
  physics::Vec3 velocity{};
  if (body_.valid()) {
    position = world_->transform(body_).position;
    velocity = world_->linearVelocity(body_);
    const double oldHeight = crouched_ ? config_.crouchingHeight
                                      : config_.standingHeight;
    const double newHeight = crouched ? config_.crouchingHeight
                                      : config_.standingHeight;
    position.y += (newHeight - oldHeight) * 0.5;
    body_.reset();
  }
  const double totalHeight =
      crouched ? config_.crouchingHeight : config_.standingHeight;
  physics::BodyDesc description(physics::Shape::capsule(
      config_.radius, totalHeight - config_.radius * 2.0));
  description.motion = physics::BodyMotion::Dynamic;
  description.massKg = config_.massKg;
  description.transform.position = position;
  description.linearVelocity = velocity;
  description.angularFactor = {0.0, 0.0, 0.0};
  description.friction = 0.0;
  description.group = physics::CollisionGroup::Player;
  description.mask = playerWorldMask();
  description.sleepingAllowed = false;
  description.metadata.type = physics::BodyType::Player;
  body_ = world_->createBody(description);
  crouched_ = crouched;
  noclipPosition_ = position;
}

void PlayerController::updateGrounded() {
  if (!body_.valid() || noclip_) {
    grounded_ = false;
    return;
  }
  const physics::Vec3 centre = world_->transform(body_).position;
  const double halfHeight =
      (crouched_ ? config_.crouchingHeight : config_.standingHeight) * 0.5;
  physics::RaycastQuery query;
  query.from = centre;
  query.to = {centre.x, centre.y - halfHeight - config_.groundProbe, centre.z};
  query.group = physics::CollisionGroup::Player;
  query.mask = playerWorldMask();
  query.includeTriggers = false;
  query.ignoreBody = body_.id();
  const auto hit = world_->raycastClosest(query);
  grounded_ = hit && hit->normal.y > 0.35 &&
              centre.y - hit->point.y <= halfHeight + config_.groundProbe;
}

bool PlayerController::canStand() const {
  if (!body_.valid() || !crouched_) {
    return true;
  }
  const physics::Vec3 centre = world_->transform(body_).position;
  const double crouchHalf = config_.crouchingHeight * 0.5;
  const double standHalf = config_.standingHeight * 0.5;
  physics::RaycastQuery query;
  query.from = {centre.x, centre.y + crouchHalf - config_.radius, centre.z};
  query.to = {centre.x, centre.y + (standHalf - crouchHalf) + crouchHalf,
              centre.z};
  query.group = physics::CollisionGroup::Player;
  query.mask = playerWorldMask();
  query.includeTriggers = false;
  query.ignoreBody = body_.id();
  return !world_->raycastClosest(query);
}

void PlayerController::tryStep(physics::Vec3 direction) {
  if (!grounded_ || config_.stepHeight <= 0.0 || !body_.valid()) {
    return;
  }
  const double magnitude = length2(direction.x, direction.z);
  if (magnitude < 1e-9) {
    return;
  }
  direction.x /= magnitude;
  direction.z /= magnitude;
  const physics::Vec3 centre = world_->transform(body_).position;
  const double halfHeight =
      (crouched_ ? config_.crouchingHeight : config_.standingHeight) * 0.5;
  const double distance = config_.radius + 20.0;
  physics::RaycastQuery low;
  low.from = {centre.x, centre.y - halfHeight + config_.radius * 0.5, centre.z};
  low.to = add(low.from, multiply(direction, distance));
  low.group = physics::CollisionGroup::Player;
  low.mask = playerWorldMask();
  low.includeTriggers = false;
  low.ignoreBody = body_.id();
  physics::RaycastQuery high = low;
  high.from.y += config_.stepHeight;
  high.to.y += config_.stepHeight;
  if (world_->raycastClosest(low) && !world_->raycastClosest(high)) {
    physics::Transform transform = world_->transform(body_);
    transform.position.y += config_.stepHeight;
    world_->setTransform(body_, transform);
  }
}

void PlayerController::fixedUpdate(double seconds) {
  if (!body_.valid()) {
    throw std::logic_error("player must be spawned before update");
  }
  if (seconds <= 0.0) {
    return;
  }
  if (noclip_) {
    // Noclip is an explicit developer override.  Keep the scripted binding
    // alive, but do not let train deltas prevent free inspection.
    parentMotion_ = {};
    const physics::Vec3 direction = viewDirection();
    const physics::Vec3 right{std::cos(yaw_), 0.0, std::sin(yaw_)};
    physics::Vec3 motion = add(multiply(right, command_.strafe),
                               multiply(direction, command_.forward));
    motion.y += command_.vertical;
    const double magnitude =
        std::sqrt(motion.x * motion.x + motion.y * motion.y + motion.z * motion.z);
    if (magnitude > 1.0) {
      motion = multiply(motion, 1.0 / magnitude);
    }
    noclipPosition_ = add(noclipPosition_,
                          multiply(motion, config_.noclipSpeed * seconds));
    physics::Transform transform = world_->transform(body_);
    transform.position = noclipPosition_;
    world_->setTransform(body_, transform);
    jumpWasDown_ = command_.jump;
    return;
  }

  if (parented_) {
    physics::Transform transform = world_->transform(body_);
    transform.position = add(transform.position, parentMotion_);
    parentMotion_ = {};
    parentedPosition_ = transform.position;
    noclipPosition_ = transform.position;
    world_->setTransform(body_, transform);
    world_->setLinearVelocity(body_, {});
    grounded_ = false;
    jumpWasDown_ = command_.jump;
    return;
  }

  if (command_.crouch && !crouched_) {
    rebuildBody(true);
  } else if (!command_.crouch && crouched_ && canStand()) {
    rebuildBody(false);
  }
  updateGrounded();
  if (parentMotion_.x != 0.0 || parentMotion_.y != 0.0 ||
      parentMotion_.z != 0.0) {
    physics::Transform transform = world_->transform(body_);
    transform.position = add(transform.position, parentMotion_);
    world_->setTransform(body_, transform);
    parentMotion_ = {};
  }

  const double moveLength = length2(command_.strafe, command_.forward);
  double strafe = command_.strafe;
  double forward = command_.forward;
  if (moveLength > 1.0) {
    strafe /= moveLength;
    forward /= moveLength;
  }
  const physics::Vec3 forwardAxis{-std::sin(yaw_), 0.0, -std::cos(yaw_)};
  const physics::Vec3 rightAxis{std::cos(yaw_), 0.0, -std::sin(yaw_)};
  physics::Vec3 horizontal =
      add(multiply(forwardAxis, forward), multiply(rightAxis, strafe));
  tryStep(horizontal);

  physics::Vec3 velocity = world_->linearVelocity(body_);
  const double speed = command_.run ? config_.runSpeed : config_.walkSpeed;
  velocity.x = horizontal.x * speed;
  velocity.z = horizontal.z * speed;
  if (onLadder_) {
    velocity.y = (command_.vertical + command_.forward) * config_.ladderSpeed;
  } else if (command_.jump && !jumpWasDown_ && grounded_) {
    velocity.y = config_.jumpSpeed;
    grounded_ = false;
  }
  world_->setLinearVelocity(body_, velocity);
  jumpWasDown_ = command_.jump;
}

physics::StepResult PlayerController::simulateFixedStep() {
  fixedUpdate(physics::PhysicsWorld::fixedStepSeconds);
  const physics::StepResult result =
      world_->advance(physics::PhysicsWorld::fixedStepSeconds);
  // Bullet still integrates gravity for a dynamic capsule.  Re-assert the
  // legacy parent relation after the shared world step so the rendered camera
  // remains exactly at its authored seat instead of sagging every tick.
  if (parented_ && !noclip_ && body_.valid()) {
    physics::Transform transform = world_->transform(body_);
    transform.position = parentedPosition_;
    world_->setTransform(body_, transform);
    world_->setLinearVelocity(body_, {});
  }
  return result;
}

void PlayerController::setNoclip(bool enabled) {
  if (!body_.valid() || noclip_ == enabled) {
    return;
  }
  if (enabled) {
    noclipPosition_ = world_->transform(body_).position;
    world_->setLinearVelocity(body_, {});
    world_->setEnabled(body_, false);
  } else {
    world_->setEnabled(body_, true);
    physics::Transform transform = world_->transform(body_);
    transform.position = noclipPosition_;
    world_->setTransform(body_, transform);
    world_->setLinearVelocity(body_, {});
  }
  noclip_ = enabled;
  grounded_ = false;
}

void PlayerController::toggleNoclip() { setNoclip(!noclip_); }
void PlayerController::setOnLadder(bool enabled) noexcept { onLadder_ = enabled; }

void PlayerController::teleport(physics::Vec3 position) {
  if (!body_.valid()) {
    spawn(position);
    return;
  }
  noclipPosition_ = position;
  physics::Transform transform = world_->transform(body_);
  transform.position = position;
  world_->setTransform(body_, transform);
  world_->setLinearVelocity(body_, {});
  grounded_ = false;
}

void PlayerController::applyParentMotion(physics::Vec3 motion) noexcept {
  parentMotion_ = add(parentMotion_, motion);
}

void PlayerController::setParented(bool enabled) noexcept {
  if (parented_ == enabled) return;
  parented_ = enabled;
  parentMotion_ = {};
  if (!body_.valid()) return;
  parentedPosition_ = world_->transform(body_).position;
  noclipPosition_ = parentedPosition_;
  world_->setLinearVelocity(body_, {});
  grounded_ = false;
}

PlayerState PlayerController::state() const {
  if (!body_.valid()) {
    return {};
  }
  return {noclip_ ? noclipPosition_ : world_->transform(body_).position,
          noclip_ ? physics::Vec3{} : world_->linearVelocity(body_), grounded_,
          crouched_, noclip_, onLadder_, parented_};
}

physics::Vec3 PlayerController::collisionHalfExtents() const noexcept {
  return {config_.radius,
          (crouched_ ? config_.crouchingHeight : config_.standingHeight) * 0.5,
          config_.radius};
}

physics::Vec3 PlayerController::eyePosition() const {
  physics::Vec3 result = state().position;
  result.y += crouched_ ? config_.crouchingEyeOffset : config_.eyeOffset;
  return result;
}

physics::Vec3 PlayerController::viewDirection(double pitch) const {
  const double horizontal = std::cos(pitch);
  return {-std::sin(yaw_) * horizontal, std::sin(pitch),
          -std::cos(yaw_) * horizontal};
}

std::optional<physics::RaycastHit>
PlayerController::castForward(double distance, double pitch) const {
  physics::RaycastQuery query;
  query.from = eyePosition();
  query.to = add(query.from, multiply(viewDirection(pitch), distance));
  query.group = physics::CollisionGroup::Player;
  // Invisible trigger volumes participate in player overlap but must not
  // occlude use/weapon rays aimed at authored buttons and doors.
  query.mask = playerWorldMask() &
               ~physics::collisionMask(physics::CollisionGroup::Trigger);
  query.ignoreBody = body_.id();
  return world_->raycastClosest(query);
}

std::optional<physics::RaycastHit>
PlayerController::useRaycast(double pitch) const {
  return castForward(config_.interactionDistance, pitch);
}

std::optional<physics::RaycastHit>
PlayerController::weaponRaycast(double pitch) const {
  return castForward(config_.weaponDistance, pitch);
}

physics::BodyId PlayerController::bodyId() const noexcept { return body_.id(); }

void runReplay(PlayerController &player,
               const std::vector<ReplayFrame> &frames) {
  for (const ReplayFrame &frame : frames) {
    player.setCommand(frame.command);
    for (std::size_t step = 0; step < frame.fixedSteps; ++step) {
      static_cast<void>(player.simulateFixedStep());
    }
  }
}

} // namespace run3::gameplay
