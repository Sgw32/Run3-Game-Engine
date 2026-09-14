#pragma once

#include <run3/physics/Physics.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace run3::gameplay {

struct PlayerConfig {
  double radius{20.0};
  double standingHeight{200.0};
  double crouchingHeight{110.0};
  double eyeOffset{50.0};
  double crouchingEyeOffset{20.0};
  double massKg{40.0};
  double walkSpeed{300.0};
  double runSpeed{520.0};
  double noclipSpeed{650.0};
  double jumpSpeed{360.0};
  double ladderSpeed{220.0};
  double stepHeight{35.0};
  double groundProbe{8.0};
  double interactionDistance{250.0};
  double weaponDistance{100000.0};
};

struct PlayerCommand {
  double strafe{};
  double forward{};
  double vertical{};
  bool run{};
  bool jump{};
  bool crouch{};
};

struct PlayerState {
  physics::Vec3 position;
  physics::Vec3 velocity;
  bool grounded{};
  bool crouched{};
  bool noclip{};
  bool onLadder{};
};

class PlayerController final {
public:
  PlayerController(physics::PhysicsWorld &world, PlayerConfig config = {});

  void spawn(physics::Vec3 centrePosition);
  void setCommand(PlayerCommand command) noexcept;
  void setYawRadians(double yaw) noexcept;
  void fixedUpdate(double seconds = physics::PhysicsWorld::fixedStepSeconds);
  [[nodiscard]] physics::StepResult simulateFixedStep();

  void setNoclip(bool enabled);
  void toggleNoclip();
  void setOnLadder(bool enabled) noexcept;
  void teleport(physics::Vec3 centrePosition);
  void applyParentMotion(physics::Vec3 translationPerStep) noexcept;

  [[nodiscard]] PlayerState state() const;
  [[nodiscard]] physics::Vec3 eyePosition() const;
  [[nodiscard]] physics::Vec3 viewDirection(double pitchRadians = 0.0) const;
  [[nodiscard]] std::optional<physics::RaycastHit>
  useRaycast(double pitchRadians = 0.0) const;
  [[nodiscard]] std::optional<physics::RaycastHit>
  weaponRaycast(double pitchRadians = 0.0) const;
  [[nodiscard]] physics::BodyId bodyId() const noexcept;

private:
  void rebuildBody(bool crouched);
  void updateGrounded();
  bool canStand() const;
  void tryStep(physics::Vec3 horizontalDirection);
  std::optional<physics::RaycastHit> castForward(double distance,
                                                 double pitch) const;

  physics::PhysicsWorld *world_{};
  PlayerConfig config_;
  physics::BodyHandle body_;
  PlayerCommand command_;
  physics::Vec3 noclipPosition_;
  physics::Vec3 parentMotion_;
  double yaw_{};
  bool grounded_{};
  bool crouched_{};
  bool noclip_{};
  bool onLadder_{};
  bool jumpWasDown_{};
};

struct ReplayFrame {
  PlayerCommand command;
  std::size_t fixedSteps{1};
};

void runReplay(PlayerController &player,
               const std::vector<ReplayFrame> &frames);

} // namespace run3::gameplay
