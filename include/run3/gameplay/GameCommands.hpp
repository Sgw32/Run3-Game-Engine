#pragma once

#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/physics/Physics.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace run3::gameplay {

enum class RuntimeEntityKind {
  Button,
  Door,
  Rotator,
  Pendulum,
  Train,
  Trigger,
  Ladder,
  Pickup,
  DarkZone
};

struct RuntimeEntitySpec {
  EntityHandle handle;
  RuntimeEntityKind kind{RuntimeEntityKind::Trigger};
  std::string name;
  std::string mesh;
  std::string material;
  std::string parent;
  physics::Transform transform;
  physics::Vec3 scale{1.0, 1.0, 1.0};
  physics::Vec3 halfExtents{1.0, 1.0, 1.0};
  bool visible{true};
  bool collision{true};
};

struct SpawnRuntimeEntity { RuntimeEntitySpec spec; };
struct DestroyRuntimeEntities {};
struct SetRuntimeTransform {
  EntityHandle handle;
  physics::Transform transform;
};
struct SetRuntimeVisible {
  EntityHandle handle;
  bool visible{};
};
struct SetRuntimeNamedVisible {
  std::string name;
  std::optional<bool> visible;
};
struct SetRuntimeLightVisible {
  std::string name;
  std::optional<bool> visible;
};
struct PlayRuntimeSound {
  EntityHandle owner;
  std::filesystem::path path;
  physics::Vec3 position;
  bool loop{};
  float gain{1.0F};
};
struct StopRuntimeSound { EntityHandle owner; };
struct PlayRuntimeEffect {
  std::filesystem::path path;
  physics::Vec3 position;
  float durationSeconds{1.0F};
  bool spatial{};
};
struct SetRuntimeMusicGain { float gain{1.0F}; };
struct SetRuntimeAmbientEnabled {
  std::string name;
  bool enabled{};
};
struct RunRuntimeScript { std::filesystem::path path; };
struct ChangeRuntimeMap { std::string map; };
struct DamageRuntimePlayer { double amount{}; };
struct TeleportRuntimePlayer { physics::Vec3 position; };
struct ApplyRuntimeParentMotion { physics::Vec3 translation; };
struct SetRuntimeDarkness { double factor{1.0}; };
struct PlayRuntimeAnimation {
  EntityHandle owner;
  std::string animation;
  bool loop{};
};
struct DeferredLegacyCommand {
  std::string name;
  std::string detail;
};
struct RuntimeLog { std::string message; };

using GameCommand =
    std::variant<SpawnRuntimeEntity, DestroyRuntimeEntities,
                 SetRuntimeTransform, SetRuntimeVisible,
                 SetRuntimeNamedVisible, SetRuntimeLightVisible,
                 PlayRuntimeSound,
                 StopRuntimeSound, PlayRuntimeEffect, SetRuntimeMusicGain,
                 SetRuntimeAmbientEnabled, RunRuntimeScript, ChangeRuntimeMap,
                 DamageRuntimePlayer, TeleportRuntimePlayer,
                 ApplyRuntimeParentMotion, SetRuntimeDarkness,
                 PlayRuntimeAnimation, DeferredLegacyCommand, RuntimeLog>;

class IGameServices {
public:
  virtual ~IGameServices() = default;
  virtual void submit(const GameCommand &command) = 0;
  [[nodiscard]] virtual physics::Vec3 playerPosition() const = 0;
  [[nodiscard]] virtual physics::Vec3 playerHalfExtents() const = 0;
  [[nodiscard]] virtual bool playerStandingOn(EntityHandle handle) const = 0;
  [[nodiscard]] virtual std::optional<bool>
  lightVisible(std::string_view name) const = 0;
};

} // namespace run3::gameplay
