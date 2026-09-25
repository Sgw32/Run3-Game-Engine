#pragma once

#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/physics/Physics.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
  DarkZone,
  Npc,
  Computer
};

struct RuntimeVisualPartSpec {
  std::string name;
  std::string mesh;
  std::string material;
  physics::Transform transform;
  physics::Vec3 scale{1.0, 1.0, 1.0};
  bool collision{};
};

struct RuntimeParticleSpec {
  std::string name;
  std::string templateName;
  physics::Vec3 position;
  physics::Vec3 scale{1.0, 1.0, 1.0};
  bool visible{};
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
  // Legacy NPC physPosit is a local visual offset (scaled by the parent),
  // while physSize scales the collision box independently.
  physics::Vec3 visualOffset{};
  physics::Vec3 collisionScale{1.0, 1.0, 1.0};
  physics::Vec3 visualRotationAxis{0.0, 1.0, 0.0};
  double visualRotationDegrees{};
  double visualYawDegrees{};
  double renderDistance{10000.0};
  std::string handBone{"Hand"};
  bool visible{true};
  bool collision{true};
  std::vector<RuntimeVisualPartSpec> parts;
  std::vector<RuntimeParticleSpec> particles;
};

struct SpawnRuntimeEntity { RuntimeEntitySpec spec; };
struct DestroyRuntimeEntity { EntityHandle handle; };
struct DestroyRuntimeEntities {};
struct SetRuntimeTransform {
  EntityHandle handle;
  physics::Transform transform;
};
struct SetRuntimeVisible {
  EntityHandle handle;
  bool visible{};
};
struct SetRuntimeCollision {
  EntityHandle handle;
  bool enabled{};
};
struct SetRuntimeNamedVisible {
  std::string name;
  std::optional<bool> visible;
};
struct SetRuntimeNamedMaterial {
  std::string name;
  std::string material;
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
// Script-authored train/camera parenting is a gameplay constraint, not just a
// camera hint.  While attached the player is pinned to the authored relative
// position and ordinary movement/gravity cannot make the capsule drift.
struct SetRuntimePlayerParented { bool parented{}; };
struct SetRuntimeHudVisible { bool visible{true}; };
struct SetRuntimeSubtitle { std::string text; double seconds{}; };
struct SetRuntimeInventoryEnabled { bool enabled{true}; };
struct SetRuntimeFlashlightAllowed { bool allowed{true}; };
struct SetRuntimeFov {
  // An empty value restores the configured gameplay FOV.
  std::optional<double> degrees;
};
struct SetRuntimeCompositor {
  std::string name;
  bool enabled{};
};
struct SetRuntimeShaderParameter {
  std::string program;
  std::string parameter;
  std::string value;
};
struct SetRuntimeEffectEnabled {
  std::string name;
  std::optional<bool> enabled;
};
struct CreateRuntimeParticle {
  std::string templateName;
  std::string name;
  physics::Vec3 position;
  physics::Vec3 scale{1.0, 1.0, 1.0};
};
struct DestroyRuntimeParticle { std::string name; };
struct SetComputerPresentation {
  EntityHandle owner;
  std::string material;
  bool focused{};
  bool allowVirtualDisplay{true};
};
struct SendComputerInput {
  EntityHandle owner;
  std::string text;
  int key{};
  bool pressed{};
};

// Step 9 may replace this presentation-only adapter. Gameplay communicates
// solely in backend-neutral handles, material keys, text and Run3 keys.
class IComputerPresentation {
public:
  virtual ~IComputerPresentation() = default;
  virtual void setComputerPresentation(
      const SetComputerPresentation &state) = 0;
  virtual void sendComputerInput(const SendComputerInput &input) = 0;
};
struct SetRuntimeDarkness { double factor{1.0}; };
struct PlayRuntimeAnimation {
  EntityHandle owner;
  std::string animation;
  bool loop{};
};
struct NpcRuntimeCommand {
  std::string name;
  int legacyEvent{};
  std::string argument;
  std::string secondArgument;
  bool broadcast{};
};
struct DestroyNpcRuntimeCommand { std::string name; };
struct SetNpcAttachment {
  EntityHandle owner;
  std::string object;
  std::string bone;
  bool attach{true};
};
struct PlayRuntimeFacial {
  EntityHandle owner;
  std::filesystem::path definition;
  physics::Vec3 position;
};
struct SpawnRuntimeRagdoll {
  EntityHandle owner;
  physics::Transform transform;
};
struct SetNpcUpdateInterval { double seconds{}; };
struct TickRuntimeNpcPhysics { double seconds{}; };
struct DeferredLegacyCommand {
  std::string name;
  std::string detail;
};
struct RuntimeLog { std::string message; };

using GameCommand =
    std::variant<SpawnRuntimeEntity, DestroyRuntimeEntity, DestroyRuntimeEntities,
                 SetRuntimeTransform, SetRuntimeVisible, SetRuntimeCollision,
                 SetRuntimeNamedVisible, SetRuntimeNamedMaterial,
                 SetRuntimeLightVisible,
                 PlayRuntimeSound,
                 StopRuntimeSound, PlayRuntimeEffect, SetRuntimeMusicGain,
                 SetRuntimeAmbientEnabled, RunRuntimeScript, ChangeRuntimeMap,
                 DamageRuntimePlayer, TeleportRuntimePlayer,
                 ApplyRuntimeParentMotion, SetRuntimePlayerParented,
                 SetRuntimeHudVisible,
                  SetRuntimeSubtitle, SetRuntimeInventoryEnabled,
                  SetRuntimeFlashlightAllowed, SetRuntimeFov,
                  SetRuntimeCompositor,
                 SetRuntimeShaderParameter, SetRuntimeEffectEnabled,
                 CreateRuntimeParticle, DestroyRuntimeParticle,
                 SetComputerPresentation,
                 SendComputerInput, SetRuntimeDarkness,
                 PlayRuntimeAnimation, NpcRuntimeCommand,
                 DestroyNpcRuntimeCommand,
                 SetNpcAttachment,
                 PlayRuntimeFacial, SpawnRuntimeRagdoll,
                 SetNpcUpdateInterval,
                 TickRuntimeNpcPhysics,
                 DeferredLegacyCommand, RuntimeLog>;

class IGameServices {
public:
  virtual ~IGameServices() = default;
  virtual void submit(const GameCommand &command) = 0;
  [[nodiscard]] virtual physics::Vec3 playerPosition() const = 0;
  [[nodiscard]] virtual physics::Vec3 playerHalfExtents() const = 0;
  [[nodiscard]] virtual bool playerStandingOn(EntityHandle handle) const = 0;
  [[nodiscard]] virtual std::optional<bool>
  lightVisible(std::string_view name) const = 0;
  [[nodiscard]] virtual std::optional<physics::Transform>
  runtimeTransform(std::string_view) const { return std::nullopt; }
  [[nodiscard]] virtual double runtimeFovDegrees() const { return 75.0; }
};

} // namespace run3::gameplay
