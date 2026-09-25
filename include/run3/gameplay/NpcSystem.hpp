#pragma once

#include <run3/air3/AirPathFind.hpp>
#include <run3/gameplay/GameCommands.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace run3::gameplay {

// Values are the authored Lua/Sequence ABI from AIR3 Defs.h; never renumber.
enum class NpcEvent : int {
  RunTo = 0, Alert = 1, Fear = 2, Crazy = 3, Stop = 4,
  Resume = 5, Kill = 6, Weapon = 7, Shoot = 8, Spawn = 9,
  GoTo = 10, SetAi = 11, Reach = 12, TakeOff = 13, Land = 14,
  SetAnimation = 15, Teleport = 16, SetParent = 17,
  SetGoalScript = 18, SetUseScript = 19, RotateOverride = 20,
  TeleportParent = 21, ToggleGravity = 22, TransitAnimation = 23,
  SetMoveActivity = 24, ResetParent = 25, SetGravity = 26,
  FacialActivity = 27, ToggleFlashlight = 28,
  AttachPhysicsObject = 29, DetachPhysicsObject = 30,
  AttachPhysicsObject2 = 31, DetachPhysicsObject2 = 32
};

enum class NpcClass { Neutral, Enemy };
enum class NpcState { Idle, Navigating, Blocked, Reached, Dead };

struct NpcSnapshot {
  EntityHandle handle;
  std::string name;
  NpcClass npcClass{NpcClass::Neutral};
  NpcState state{NpcState::Idle};
  physics::Transform transform;
  physics::Vec3 goal;
  double health{30.0};
  std::string animation;
  std::string parent;
  bool gravityEnabled{true};
};

// A map-owned deterministic state machine. Presentation, scripts, sound and
// collision are submitted through Run3 service commands, never Ogre/Newton.
class NpcSystem final {
public:
  NpcSystem(const content::MapDefinition &definition, EntityRegistry &registry,
            const physics::IPhysicsQuery &query, IGameServices &services);
  ~NpcSystem();
  NpcSystem(const NpcSystem &) = delete;
  NpcSystem &operator=(const NpcSystem &) = delete;

  void start();
  void fixedUpdate(double seconds = 1.0 / 60.0);
  void setUpdateInterval(double seconds);
  void dispatch(const NpcRuntimeCommand &command);
  bool damage(EntityHandle handle, double amount, bool headshot = false);
  bool damage(EntityHandle handle, double amount,
              physics::Vec3 hitPosition);
  bool use(EntityHandle handle);
  bool destroy(std::string_view name);
  void unload();
  [[nodiscard]] std::optional<NpcSnapshot> state(std::string_view name) const;
  [[nodiscard]] const std::vector<NpcSnapshot> &states() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::string serializeState() const;
  void restoreSerializedState(std::string_view state);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::optional<NpcEvent> npcEventFromLegacy(int code) noexcept;

} // namespace run3::gameplay
