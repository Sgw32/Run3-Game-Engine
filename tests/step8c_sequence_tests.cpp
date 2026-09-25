#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/GameCommands.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>
#include <run3/scripting/ScriptEngine.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace run3;

namespace {

const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};

struct FakeServices final : gameplay::IGameServices {
  physics::Vec3 player{};
  std::vector<gameplay::GameCommand> commands;
  std::string failScript;
  bool standingOnTrain{};
  bool fixtureLightVisible{};
  double fovDegrees{75.0};

  void submit(const gameplay::GameCommand &command) override {
    if (const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
        script != nullptr && script->path.generic_string() == failScript) {
      throw std::runtime_error("fixture script failure");
    }
    commands.push_back(command);
  }
  [[nodiscard]] physics::Vec3 playerPosition() const override { return player; }
  [[nodiscard]] physics::Vec3 playerHalfExtents() const override {
    return {0.0, 0.0, 0.0};
  }
  [[nodiscard]] bool playerStandingOn(gameplay::EntityHandle) const override {
    return standingOnTrain;
  }
  [[nodiscard]] std::optional<bool>
  lightVisible(std::string_view name) const override {
    return name == "fixture-light" ? std::optional<bool>{fixtureLightVisible}
                                   : std::nullopt;
  }
  [[nodiscard]] double runtimeFovDegrees() const override {
    return fovDegrees;
  }

  template <class T> std::size_t count() const {
    return static_cast<std::size_t>(std::count_if(
        commands.begin(), commands.end(), [](const gameplay::GameCommand &item) {
          return std::holds_alternative<T>(item);
        }));
  }
};

struct FixtureRuntime {
  AppPaths paths;
  content::MapDefinition definition;
  gameplay::EntityRegistry registry;
  FakeServices services;
  std::unique_ptr<gameplay::SequenceRuntime> runtime;

  FixtureRuntime()
      : paths(AppPaths::resolve(
            sourceRoot / "build/step8c-fixture.exe",
            sourceRoot / "tests/fixtures/step8c/content",
            sourceRoot / "build/step8c-user")),
        definition(content::loadMapDefinition(paths, "fixture", "low")) {
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry, true));
    runtime = std::make_unique<gameplay::SequenceRuntime>(definition, registry,
                                                          services);
  }
};

std::vector<gameplay::SequenceEntityState> runAtRenderRate(double renderHz) {
  FixtureRuntime fixture;
  fixture.services.player = {1000.0, 1000.0, 1000.0};
  fixture.runtime->start();
  double accumulator{};
  std::size_t ticks{};
  while (ticks < 180) {
    accumulator += 1.0 / renderHz;
    while (accumulator + 1e-12 >= gameplay::SequenceRuntime::fixedStepSeconds &&
           ticks < 180) {
      fixture.runtime->fixedUpdate();
      accumulator -= gameplay::SequenceRuntime::fixedStepSeconds;
      ++ticks;
    }
  }
  return fixture.runtime->states();
}

} // namespace

TEST_CASE("Step 8C lifecycle constructs in authored order and cancels queued work",
          "[step8c][lifecycle]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  CHECK(fixture.runtime->started());
  CHECK(fixture.services.count<gameplay::SpawnRuntimeEntity>() == 11);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);

  fixture.runtime->fixedUpdate(); // enter the one-shot trigger
  fixture.runtime->unload(true);  // cancel delayed actions before tick 3
  CHECK(fixture.runtime->unloaded());
  CHECK(fixture.services.count<gameplay::DestroyRuntimeEntities>() == 1);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 2);
  CHECK(fixture.services.count<gameplay::SetRuntimeTransform>() >= 1);
}

TEST_CASE("Step 8C timers, trigger modes, delayed events, and use are deterministic",
          "[step8c][scheduler][trigger][use]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.runtime->fixedUpdate();
  REQUIRE(fixture.runtime->state("once").has_value());
  CHECK(fixture.runtime->state("once")->activationCount == 1);
  fixture.runtime->fixedUpdate();
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("door")->active);
  CHECK(fixture.runtime->state("pulse")->activationCount == 1);

  fixture.services.player = {10.0, 0.0, 0.0};
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::SetRuntimeLightVisible>() == 1);
  fixture.services.player = {100.0, 0.0, 0.0};
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::SetRuntimeLightVisible>() == 2);
  CHECK(fixture.runtime->state("multiple")->activationCount == 2);

  const auto button = fixture.registry.findFirst("button");
  REQUIRE(button.has_value());
  CHECK(fixture.runtime->interact(*button));
  CHECK(fixture.runtime->state("button")->activationCount == 1);
}

TEST_CASE("Step 8C warns on missing objects and preserves script execution",
          "[step8c][lua][commands]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "openDoor", {"door"}}));
  CHECK(fixture.runtime->state("door")->active);
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "openDoor", {"missing"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "startTrain", {"missing"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "enableTimer", {"missing"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "enableTrigger", {"missing"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "showEntity", {"missing"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "startEvent", {"missing"}}));
  CHECK_THROWS(fixture.runtime->dispatchScriptCall(
      {"sequence", "closeDoor", {}}));
  CHECK_THROWS(fixture.runtime->dispatchScriptCall(
      {"sequence", "setSpeedTrain", {"train", "not-a-number"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"player", "teleport", {"1", "2", "3"}}));
  CHECK(fixture.services.count<gameplay::TeleportRuntimePlayer>() == 1);

  scripting::ScriptEngine scripts(
      {fixture.paths.contentRoot(), fixture.paths.userRoot(), 10000},
      [&fixture](const scripting::ScriptCall &call) {
        return fixture.runtime->dispatchScriptCall(call);
      });
  CHECK_NOTHROW(scripts.executeFile("run3/lua/start.lua"));
  CHECK_NOTHROW(scripts.executeFile("run3/lua/bad-target.lua"));
  CHECK(std::any_of(fixture.services.commands.begin(),
                    fixture.services.commands.end(),
                    [](const gameplay::GameCommand &command) {
    const auto *message = std::get_if<gameplay::RuntimeLog>(&command);
    return message && message->message.find(
        "warning: Lua command openDoor skipped missing door 'missing'") !=
        std::string::npos;
  }));
  CHECK(std::any_of(fixture.services.commands.begin(),
                    fixture.services.commands.end(),
                    [](const gameplay::GameCommand &command) {
    const auto *message = std::get_if<gameplay::RuntimeLog>(&command);
    return message && message->message == "continued after missing door";
  }));
  CHECK_THROWS(scripts.executeFile("run3/lua/does-not-exist.lua"));
}

TEST_CASE("Step 8C persistent state round-trips entity state",
          "[step8c][save]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.runtime->setDoorOpen("door", true);
  for (int tick = 0; tick < 4; ++tick) fixture.runtime->fixedUpdate();
  const auto saved = fixture.runtime->saveState();
  fixture.runtime->setDoorOpen("door", false);
  fixture.runtime->fixedUpdate();
  fixture.runtime->restoreState(saved);
  CHECK(fixture.runtime->tick() == saved.tick);
  CHECK(fixture.runtime->state("door")->active);
  const auto savedDoor = std::find_if(
      saved.entities.begin(), saved.entities.end(),
      [](const gameplay::SequenceEntityState &state) {
        return state.name == "door";
      });
  REQUIRE(savedDoor != saved.entities.end());
  CHECK(fixture.runtime->state("door")->transform.position ==
        savedDoor->transform.position);
}

TEST_CASE("Step 8C platform carries only a player standing on that train",
          "[step8c][train][physics]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.services.standingOnTrain = false;
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::ApplyRuntimeParentMotion>() == 0);
  fixture.services.standingOnTrain = true;
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::ApplyRuntimeParentMotion>() == 1);
  fixture.services.standingOnTrain = false;
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::ApplyRuntimeParentMotion>() == 1);
}

TEST_CASE("Step 8C train stops at its last key point and door keeps legacy rate",
          "[step8c][train][door]") {
  FixtureRuntime fixture;
  fixture.services.player = {1000.0, 1000.0, 1000.0};
  fixture.runtime->start();
  REQUIRE(fixture.runtime->setDoorOpen("door", true));
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("door")->transform.position.x == 5.0);
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("door")->transform.position.x == 6.0);
  for (int tick = 0; tick < 8; ++tick) fixture.runtime->fixedUpdate();
  const auto train = fixture.runtime->state("train");
  REQUIRE(train.has_value());
  CHECK(train->transform.position.x == 6.0);
  CHECK_FALSE(train->active);
  for (int tick = 0; tick < 30; ++tick) fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("train")->transform.position.x == 6.0);
  CHECK(fixture.services.count<gameplay::StopRuntimeSound>() == 1);
}

TEST_CASE("Step 8C rotating doors and func rotators preserve legacy activation and rate",
          "[step8c][door][rotator]") {
  FixtureRuntime fixture;
  fixture.services.player = {1000.0, 1000.0, 1000.0};
  fixture.runtime->start();

  const auto rotator = fixture.registry.findFirst("rot");
  REQUIRE(rotator.has_value());
  REQUIRE(fixture.runtime->state("rot").has_value());
  CHECK_FALSE(fixture.runtime->state("rot")->active);
  CHECK(fixture.runtime->state("rot")->transform.rotation ==
        physics::Quaternion{});
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("rot")->transform.rotation ==
        physics::Quaternion{});
  REQUIRE(fixture.runtime->interact(*rotator));
  fixture.runtime->fixedUpdate();
  const auto spinning = fixture.runtime->state("rot");
  REQUIRE(spinning.has_value());
  CHECK(spinning->active);
  // 90 authored degrees/s * legacy factor 5 / 60 Hz = -7.5 degrees.
  CHECK(spinning->transform.rotation.y ==
        Catch::Approx(-std::sin(3.75 * 3.14159265358979323846 / 180.0)));

  REQUIRE(fixture.runtime->setDoorOpen("rotDoor", true));
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("rotDoor")->transform.rotation.x ==
        Catch::Approx(std::sin(0.5 * 3.14159265358979323846 / 180.0)));
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("rotDoor")->transform.rotation.x ==
        Catch::Approx(std::sin(1.0 * 3.14159265358979323846 / 180.0)));
  REQUIRE(fixture.runtime->setDoorOpen("rotDoor", false));
  fixture.runtime->fixedUpdate();
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("rotDoor")->transform.rotation ==
        physics::Quaternion{});
}

TEST_CASE("Step 8C legacy FOV bindings return a number and drive the camera service",
          "[step8c][lua][fov]") {
  FixtureRuntime fixture;
  fixture.services.fovDegrees = 86.0;
  const scripting::ScriptValue queried = fixture.runtime->dispatchScriptCall(
      {"world", "getFov", {}});
  REQUIRE(std::holds_alternative<double>(queried));
  CHECK(std::get<double>(queried) == 86.0);

  scripting::ScriptEngine scripts(
      {fixture.paths.contentRoot(), fixture.paths.userRoot(), 10000},
      [&fixture](const scripting::ScriptCall &call) {
        return fixture.runtime->dispatchScriptCall(call);
      });
  CHECK_NOTHROW(scripts.executeText(
      "goalf=getFov(); epict=170; epict=epict+(goalf-epict)/20; "
      "setFov(epict); resetFov()",
      "fov-compat.lua"));
  std::vector<gameplay::SetRuntimeFov> commands;
  for (const auto &command : fixture.services.commands) {
    if (const auto *fov = std::get_if<gameplay::SetRuntimeFov>(&command)) {
      commands.push_back(*fov);
    }
  }
  REQUIRE(commands.size() == 2);
  REQUIRE(commands[0].degrees.has_value());
  CHECK(*commands[0].degrees == Catch::Approx(165.8));
  CHECK_FALSE(commands[1].degrees.has_value());
}

TEST_CASE("Step 8C closed-door completion requires a real close transition",
          "[step8c][door][lua]") {
  FixtureRuntime fixture;
  fixture.services.player = {1000.0, 1000.0, 1000.0};
  fixture.runtime->start();
  const auto closedCalls = [&fixture] {
    return std::count_if(fixture.services.commands.begin(),
                         fixture.services.commands.end(),
                         [](const gameplay::GameCommand &command) {
      const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
      return script && script->path == "run3/lua/closed.lua";
    });
  };
  fixture.runtime->fixedUpdate();
  CHECK(closedCalls() == 0);
  fixture.runtime->setDoorOpen("door", true);
  for (int tick = 0; tick < 8; ++tick) fixture.runtime->fixedUpdate();
  fixture.runtime->setDoorOpen("door", false);
  for (int tick = 0; tick < 8; ++tick) fixture.runtime->fixedUpdate();
  CHECK(closedCalls() == 1);
}

TEST_CASE("Step 8C snapshot restores delayed actions and timer phase",
          "[step8c][save][scheduler]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.runtime->fixedUpdate();
  const auto saved = fixture.runtime->saveState();
  const std::string serialized = fixture.runtime->serializeState();
  REQUIRE_FALSE(saved.pending.empty());
  fixture.runtime->fixedUpdate();
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("door")->active);
  fixture.runtime->restoreState(saved);
  CHECK_FALSE(fixture.runtime->state("door")->active);
  fixture.runtime->fixedUpdate();
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("door")->active);
  CHECK(fixture.runtime->state("pulse")->activationCount == 1);
  fixture.runtime->fixedUpdate();
  REQUIRE_NOTHROW(fixture.runtime->restoreSerializedState(serialized));
  CHECK(fixture.runtime->tick() == saved.tick);
  CHECK_FALSE(fixture.runtime->state("door")->active);
}

TEST_CASE("Step 8C onexit failure is contextual and still releases ownership",
          "[step8c][lifecycle][lua]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.services.failScript = "run3/lua/exit.lua";
  CHECK_THROWS_WITH(
      fixture.runtime->unload(true),
      Catch::Matchers::ContainsSubstring("fixture script failure"));
  CHECK(fixture.runtime->unloaded());
  CHECK(fixture.services.count<gameplay::DestroyRuntimeEntities>() == 1);
}

TEST_CASE("Step 8C fixed gameplay result is independent of render FPS",
          "[step8c][replay]") {
  const auto at30 = runAtRenderRate(30.0);
  const auto at60 = runAtRenderRate(60.0);
  const auto at144 = runAtRenderRate(144.0);
  REQUIRE(at30.size() == at60.size());
  REQUIRE(at60.size() == at144.size());
  for (std::size_t index = 0; index < at30.size(); ++index) {
    CAPTURE(index, at30[index].name);
    CHECK(at30[index].transform == at60[index].transform);
    CHECK(at60[index].transform == at144[index].transform);
    CHECK(at30[index].activationCount == at144[index].activationCount);
  }
}

TEST_CASE("Step 8C inventories selected TLW maps and constructs live slices",
          "[step8c][content]") {
  const fs::path contentRoot = sourceRoot / "Games/The Long Way/TheLongWay";
  if (!fs::is_directory(contentRoot)) {
    SKIP("Optional author-provided The Long Way content is not attached");
  }
  const AppPaths paths = AppPaths::resolve(
      sourceRoot / "build/step8c-content.exe", contentRoot,
      sourceRoot / "build/step8c-content-user");
  for (const std::string map : {"tlwcao", "tlwhome02", "tlwstations01",
                                "tlwstations03"}) {
    CAPTURE(map);
    const auto definition = content::loadMapDefinition(paths, map, "low");
    gameplay::EntityRegistry registry;
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry, false));
    FakeServices services;
    gameplay::SequenceRuntime runtime(definition, registry, services);
    CHECK_FALSE(runtime.states().empty());
    if (map == "tlwcao") CHECK(runtime.states().size() == 114);
    if (map == "tlwhome02") CHECK(runtime.states().size() == 148);
    runtime.start();
    if (map == "tlwcao") {
      const auto fake = runtime.state("fake1");
      REQUIRE(fake.has_value());
      CHECK(fake->tag == "rot");
      CHECK_FALSE(fake->active);
      const physics::Quaternion initialRotation = fake->transform.rotation;
      REQUIRE(runtime.interact(fake->handle));
      runtime.fixedUpdate();
      CHECK_FALSE(runtime.state("fake1")->transform.rotation ==
                  initialRotation);

      scripting::ScriptEngine scripts(
          {paths.contentRoot(), paths.userRoot(), 10000},
          [&runtime](const scripting::ScriptCall &call) {
            return runtime.dispatchScriptCall(call);
          });
      CHECK_NOTHROW(scripts.executeFile(
          "run3/lua/chapters/tlwcao/close_turnik.lua"));
      CHECK_NOTHROW(scripts.executeText(
          "goalf=getFov(); epict=170; sb=0", "tlwoutro-fov-setup.lua"));
      CHECK_NOTHROW(scripts.executeFile(
          "run3/lua/chapters/tlwoutro/epictimer2.lua"));
      CHECK(std::any_of(services.commands.begin(), services.commands.end(),
                        [](const gameplay::GameCommand &command) {
        const auto *message = std::get_if<gameplay::RuntimeLog>(&command);
        return message && message->message.find("missing door 'right3'") !=
                              std::string::npos;
      }));
    }
    if (map == "tlwstations01") {
      CHECK_NOTHROW(runtime.dispatchScriptCall(
          {"sequence", "startTrain", {"mspz1"}}));
      CHECK_NOTHROW(runtime.dispatchScriptCall(
          {"sequence", "toggleEntity", {"mspz1"}}));
      const auto &states = runtime.states();
      const auto train = std::find_if(states.begin(), states.end(),
                                      [](const gameplay::SequenceEntityState &state) {
        return state.name == "mspz1" && state.tag == "train";
      });
      REQUIRE(train != states.end());
      CHECK(train->active);
      CHECK(train->enabled);
      CHECK_FALSE(train->visible);
      CHECK(services.count<gameplay::RuntimeLog>() == 0);
      scripting::ScriptEngine scripts(
          {paths.contentRoot(), paths.userRoot(), 10000},
          [&runtime](const scripting::ScriptCall &call) {
            return runtime.dispatchScriptCall(call);
          });
      CHECK_NOTHROW(scripts.executeFile(
          "run3/lua/chapters/tlwstations01/air.lua"));
      CHECK(services.count<gameplay::RuntimeLog>() == 0);
    }
    runtime.fixedUpdate();
    runtime.unload(false);
  }
}
