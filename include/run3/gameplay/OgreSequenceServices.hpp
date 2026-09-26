#pragma once

#include <run3/app/AppPaths.hpp>
#include <run3/audio/Audio.hpp>
#include <run3/gameplay/GameCommands.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Ogre {
class Camera;
class SceneManager;
}

namespace run3::gameplay {

struct OgreSequenceResourceCounts {
  std::size_t presentations{};
  std::size_t visualParts{};
  std::size_t particles{};
  std::size_t physicsBindings{};
  std::size_t audioHandles{};
  std::size_t attachments{};
  std::size_t ragdolls{};
  bool rootNode{};

  [[nodiscard]] bool empty() const noexcept {
    return presentations == 0 && visualParts == 0 && particles == 0 &&
           physicsBindings == 0 && audioHandles == 0 && attachments == 0 &&
           ragdolls == 0 && !rootNode;
  }
};

class PlayerController;
class StaticMap;
class NpcSystem;
}
namespace run3::audio {
class MapAudioRuntime;
}
namespace run3::ui {
class IUiSystem;
}
namespace run3::gameplay {

class OgreSequenceServices final : public IGameServices,
                                   public IComputerPresentation {
public:
  using MapChangeRequest = std::function<void(std::string)>;

  OgreSequenceServices(const AppPaths &paths, Ogre::SceneManager &sceneManager,
                       Ogre::Camera &camera,
                       physics::PhysicsWorld &physicsWorld,
                       StaticMap &staticMap, PlayerController &player,
                       audio::IAudioEngine &audio, ui::IUiSystem &ui,
                       MapChangeRequest mapChangeRequest = {},
                       double meshLodBias = 1.0,
                       double defaultFovDegrees = 75.0);
  ~OgreSequenceServices() override;
  OgreSequenceServices(const OgreSequenceServices &) = delete;
  OgreSequenceServices &operator=(const OgreSequenceServices &) = delete;

  void attach(SequenceRuntime &runtime) noexcept;
  void attachNpcSystem(NpcSystem &system) noexcept;
  void attachMapAudio(audio::MapAudioRuntime &mapAudio) noexcept;
  void updateAudio(float seconds);
  void submit(const GameCommand &command) override;
  void setComputerPresentation(
      const SetComputerPresentation &state) override;
  void sendComputerInput(const SendComputerInput &input) override;
  [[nodiscard]] physics::Vec3 playerPosition() const override;
  [[nodiscard]] physics::Vec3 playerHalfExtents() const override;
  [[nodiscard]] bool playerStandingOn(EntityHandle handle) const override;
  [[nodiscard]] std::optional<bool>
  lightVisible(std::string_view name) const override;
  [[nodiscard]] std::optional<physics::Transform>
  runtimeTransform(std::string_view name) const override;
  [[nodiscard]] double runtimeFovDegrees() const override;
  [[nodiscard]] std::optional<EntityHandle>
  handleForPhysicsEntity(std::uint64_t physicsEntity) const;
  [[nodiscard]] OgreSequenceResourceCounts resourceCounts() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace run3::gameplay
