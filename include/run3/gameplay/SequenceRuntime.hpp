#pragma once

#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/GameCommands.hpp>
#include <run3/scripting/ScriptEngine.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace run3::gameplay {

class SequenceRuntimeError final : public std::runtime_error {
public:
  SequenceRuntimeError(content::SourceLocation source, std::string cause);
  [[nodiscard]] const content::SourceLocation &source() const noexcept {
    return source_;
  }

private:
  content::SourceLocation source_;
};

struct SequenceEntityState {
  EntityHandle handle;
  std::string name;
  std::string tag;
  physics::Transform transform;
  bool enabled{true};
  bool visible{true};
  bool active{};
  bool inside{};
  std::uint64_t activationCount{};
};

struct PersistentSequenceState {
  struct Internals {
    std::uint64_t nextTick{};
    std::size_t keyPoint{};
    double phase{};
    bool oneShotFired{};
    bool completionFired{};
    bool reverse{};
  };
  struct PendingAction {
    std::uint64_t dueTick{};
    std::uint64_t order{};
    content::AuthoredElement action;
  };

  std::string mapName;
  std::uint64_t tick{};
  std::vector<SequenceEntityState> entities;
  std::vector<Internals> internals;
  std::vector<PendingAction> pending;
  std::uint64_t nextQueueOrder{};
};

class SequenceRuntime final {
public:
  static constexpr double fixedStepSeconds = 1.0 / 60.0;

  SequenceRuntime(const content::MapDefinition &definition,
                  EntityRegistry &registry, IGameServices &services);
  ~SequenceRuntime();
  SequenceRuntime(const SequenceRuntime &) = delete;
  SequenceRuntime &operator=(const SequenceRuntime &) = delete;

  void start();
  void fixedUpdate();
  void unload(bool runOnExit = true);

  bool interact(EntityHandle handle);
  bool interactByEntityId(EntityId id);
  bool setTriggerEnabled(std::string_view name, bool enabled);
  bool setTimerEnabled(std::string_view name, bool enabled);
  bool setDoorOpen(std::string_view name, bool open);
  bool setTrainRunning(std::string_view name, bool running);

  [[nodiscard]] scripting::ScriptValue
  dispatchScriptCall(const scripting::ScriptCall &call);
  [[nodiscard]] PersistentSequenceState saveState() const;
  void restoreState(const PersistentSequenceState &state);

  [[nodiscard]] std::uint64_t tick() const noexcept;
  [[nodiscard]] bool started() const noexcept;
  [[nodiscard]] bool unloaded() const noexcept;
  [[nodiscard]] const std::vector<SequenceEntityState> &states() const noexcept;
  [[nodiscard]] std::optional<SequenceEntityState>
  state(std::string_view name) const;
  [[nodiscard]] bool playerOnLadder() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace run3::gameplay
