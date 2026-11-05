#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/GameCommands.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>
#include <run3/gameplay/NpcSystem.hpp>
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
  gameplay::NpcSystem *npcs{};
  gameplay::SequenceRuntime *sequence{};
  std::optional<physics::Transform> runtimeTransform(std::string_view name) const override {
    const auto state = sequence ? sequence->state(name) : std::nullopt;
    return state ? std::optional<physics::Transform>{state->transform} : std::nullopt;
  }
  physics::Vec3 player{};
  std::vector<gameplay::GameCommand> commands;
  std::string failScript;
  bool standingOnTrain{};
  bool fixtureLightVisible{};
  bool playerParented{};
  std::size_t activePresentations{};
  double fovDegrees{75.0};
  std::optional<double> musicSeconds;
  std::optional<double> musicDuration;
  std::vector<double> musicSeeks;

  void submit(const gameplay::GameCommand &command) override {
    if (const auto *event = std::get_if<gameplay::NpcRuntimeCommand>(&command); event && npcs)
      npcs->dispatch(*event);
    if (const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
        script != nullptr && script->path.generic_string() == failScript) {
      throw std::runtime_error("fixture script failure");
    }
    if (std::holds_alternative<gameplay::SpawnRuntimeEntity>(command))
      ++activePresentations;
    if (std::holds_alternative<gameplay::DestroyRuntimeEntities>(command))
      activePresentations = 0;
    if (const auto *parent =
            std::get_if<gameplay::SetRuntimePlayerParented>(&command))
      playerParented = parent->parented;
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
  [[nodiscard]] std::optional<double> runtimeMusicSeconds() const override {
    return musicSeconds;
  }
  [[nodiscard]] std::optional<double> runtimeMusicDurationSeconds() const override {
    return musicDuration;
  }
  bool seekRuntimeMusicSeconds(double seconds) override {
    musicSeeks.push_back(seconds);
    musicSeconds = seconds;
    return true;
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

TEST_CASE("Step 8C music-synchronised cutscene follows the audio cursor",
          "[step8c][cutscene][audio]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  REQUIRE(fixture.runtime->startCutscene("music-sync"));
  CHECK(fixture.services.count<gameplay::PlayRuntimeSound>() == 1);

  fixture.services.musicSeconds = 0.5;
  fixture.runtime->fixedUpdate();
  REQUIRE(fixture.runtime->presentation().camera);
  CHECK(fixture.runtime->presentation().camera->position.x ==
        Catch::Approx(30.0));

  // A repeated cursor intentionally holds the camera: simulation load cannot
  // make the visual beat run ahead of the decoded track.
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->presentation().camera->position.x ==
        Catch::Approx(30.0));

  fixture.services.musicSeconds = 1.5;
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->presentation().camera->position.x ==
        Catch::Approx(90.0));
}

TEST_CASE("Music skipping follows normalized playback without scaling the camera twice",
          "[step8c][cutscene][audio][skip]") {
  FixtureRuntime fixture;
  fixture.services.musicDuration = 4.0; // authored musicLength is 2 seconds
  fixture.services.musicSeconds = 1.0;
  fixture.runtime->start();
  REQUIRE(fixture.runtime->startCutscene("music-sync"));
  REQUIRE(fixture.runtime->skipCutscene());
  CHECK(fixture.runtime->cutsceneTimeScale() == 2.0);
  const auto tick = fixture.runtime->tick();
  fixture.runtime->updateCutscenePresentation();
  REQUIRE(fixture.runtime->presentation().camera);
  CHECK(fixture.runtime->presentation().camera->position.x == Catch::Approx(30));
  CHECK(fixture.runtime->tick() == tick); // no physics step was needed
  for (int step = 0; step < 20; ++step) fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->presentation().camera->position.x == Catch::Approx(30));
  fixture.services.musicSeconds = 3.0;
  fixture.runtime->updateCutscenePresentation();
  CHECK(fixture.runtime->presentation().camera->position.x == Catch::Approx(90));
  REQUIRE(fixture.runtime->handleInput({InputEventType::KeyReleased, Key::Space}));
  CHECK(fixture.runtime->cutsceneTimeScale() == 1.0);
  // A backward cursor correction must move the cutscene back too, not leave
  // it ahead of the music waiting for the old high-water mark.
  fixture.services.musicSeconds = 2.0;
  fixture.runtime->updateCutscenePresentation();
  CHECK(fixture.runtime->presentation().camera->position.x == Catch::Approx(60));
  CHECK(fixture.services.musicSeeks.empty());
  CHECK(std::count_if(fixture.services.commands.begin(), fixture.services.commands.end(),
      [](const auto &command) {
        const auto *sound = std::get_if<gameplay::PlayRuntimeSound>(&command);
        return sound != nullptr && sound->path == "run3/sounds/score.ogg";
      }) == 1);

  const auto saved = fixture.runtime->saveState();
  fixture.runtime->restoreState(saved);
  REQUIRE(fixture.services.musicSeeks.size() == 1);
  CHECK(fixture.services.musicSeeks.back() == Catch::Approx(2.0));
  fixture.services.musicSeconds = 1.013;
  fixture.runtime->updateCutscenePresentation();
  CHECK(fixture.runtime->presentation().camera->position.x == Catch::Approx(30.39));
  CHECK(fixture.runtime->presentation().cutsceneTimeSeconds == Catch::Approx(0.5065));
  CHECK(fixture.runtime->presentation().cutsceneDurationSeconds == Catch::Approx(2.0));
}

TEST_CASE("Music-driven presentation fires crossed hooks once without physics catchup",
          "[step8c][cutscene][audio]") {
  FixtureRuntime fixture;
  fixture.runtime.reset();
  content::AuthoredElement hook{"cutscene", {{"name", "music-sync"}}};
  hook.children.push_back({"run", {{"sec", "1"}, {"script", "beat.lua"}}});
  fixture.definition.sequences.front().events.push_back(hook);
  fixture.runtime = std::make_unique<gameplay::SequenceRuntime>(
      fixture.definition, fixture.registry, fixture.services);
  fixture.runtime->start();
  fixture.services.musicDuration = 4.0;
  fixture.services.musicSeconds = 0.0;
  REQUIRE(fixture.runtime->startCutscene("music-sync"));
  const auto scriptsBefore = fixture.services.count<gameplay::RunRuntimeScript>();
  fixture.services.musicSeconds = 1.999; // just before the authored one-second beat
  fixture.runtime->updateCutscenePresentation();
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == scriptsBefore);
  fixture.services.musicSeconds = 3.5; // jump past the beat on a slow frame
  fixture.runtime->updateCutscenePresentation();
  const auto fired = [&] {
    return std::count_if(fixture.services.commands.begin(), fixture.services.commands.end(),
        [](const auto &command) {
          const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
          return script != nullptr && script->path == "beat.lua";
        });
  };
  CHECK(fired() == 1);
  fixture.runtime->updateCutscenePresentation();
  fixture.runtime->fixedUpdate();
  CHECK(fired() == 1);
  fixture.services.musicSeconds = 3.999;
  fixture.runtime->updateCutscenePresentation();
  CHECK_FALSE(fixture.runtime->presentation().activeCutscene.empty());
  fixture.services.musicSeconds = 4.0;
  fixture.runtime->updateCutscenePresentation();
  CHECK(fixture.runtime->presentation().activeCutscene.empty());
  CHECK(fixture.runtime->presentation().cutsceneTimeSeconds == 0.0);
  CHECK(fixture.runtime->presentation().cutsceneDurationSeconds == 0.0);
  CHECK(fixture.runtime->cutsceneTimeScale() == 1.0);
}

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
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "materialEntity", {"button", "Fixture/Material"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "toggleParticleSystem", {"fixture-particle"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "toggleEmitter", {"fixture-particle"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "enableEmitter", {"fixture-particle"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"ui", "disableEmitter", {"fixture-particle"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "createParticleSystem",
       {"Fixture/Particle", "spawned-particle", "1 2 3", "2 2 2"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"ui", "deleteParticleSystem", {"spawned-particle"}}));
  CHECK(fixture.services.count<gameplay::SetRuntimeNamedMaterial>() == 1);
  CHECK(fixture.services.count<gameplay::SetRuntimeEffectEnabled>() == 1);
  CHECK(fixture.services.count<gameplay::SetRuntimeEmitterEnabled>() == 3);
  std::vector<std::optional<bool>> emitterStates;
  for (const auto &command : fixture.services.commands) {
    if (const auto *emitter =
            std::get_if<gameplay::SetRuntimeEmitterEnabled>(&command))
      emitterStates.push_back(emitter->enabled);
  }
  REQUIRE(emitterStates.size() == 3);
  CHECK_FALSE(emitterStates[0].has_value());
  CHECK(emitterStates[1] == std::optional<bool>{true});
  CHECK(emitterStates[2] == std::optional<bool>{false});
  CHECK(fixture.services.count<gameplay::CreateRuntimeParticle>() == 1);
  CHECK(fixture.services.count<gameplay::DestroyRuntimeParticle>() == 1);

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

TEST_CASE("Step 8C Lua npcEvent routes facial activity and its definition",
          "[step8c][lua][npc][facial]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  scripting::ScriptEngine scripts(
      {fixture.paths.contentRoot(), fixture.paths.userRoot(), 10000},
      [&fixture](const scripting::ScriptCall &call) {
        return fixture.runtime->dispatchScriptCall(call);
      });
  REQUIRE_NOTHROW(scripts.executeText(
      "npcEvent('guide', '27', 'run3/sounds/speech/guide.xml')",
      "facial-event.lua"));
  const auto command = std::find_if(
      fixture.services.commands.rbegin(), fixture.services.commands.rend(),
      [](const gameplay::GameCommand &item) {
        return std::holds_alternative<gameplay::NpcRuntimeCommand>(item);
      });
  REQUIRE(command != fixture.services.commands.rend());
  const auto &npc = std::get<gameplay::NpcRuntimeCommand>(*command);
  CHECK(npc.name == "guide");
  CHECK(npc.legacyEvent == 27);
  CHECK(npc.argument == "run3/sounds/speech/guide.xml");
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

TEST_CASE("Step 8C scripted train parent pins and releases the player",
          "[step8c][train][parent]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "setCameraParent", {"train"}}));
  CHECK(fixture.services.playerParented);
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::ApplyRuntimeParentMotion>() == 1);
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "resetCameraParent", {}}));
  CHECK_FALSE(fixture.services.playerParented);

  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "setCameraParent", {"train"}}));
  fixture.runtime->unload(false);
  CHECK_FALSE(fixture.services.playerParented);
  CHECK(fixture.services.activePresentations == 0);
}

TEST_CASE("Step 8C train acceleration preserves legacy update ordering",
          "[step8c][train][lua]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "setSpeedTrain", {"train", "0"}}));
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "setAccTrain", {"train", "60"}}));
  fixture.runtime->fixedUpdate();
  REQUIRE(fixture.runtime->state("train").has_value());
  CHECK(fixture.runtime->state("train")->transform.position.x ==
        Catch::Approx(1.0 / 60.0));
  const std::string snapshot = fixture.runtime->serializeState();
  fixture.runtime->fixedUpdate();
  REQUIRE_NOTHROW(fixture.runtime->restoreSerializedState(snapshot));
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("train")->transform.position.x ==
        Catch::Approx(3.0 / 60.0));
}

TEST_CASE("Step 8C repeated map ownership teardown is idempotent",
          "[step8c][lifecycle][leak]") {
  for (int cycle = 0; cycle < 100; ++cycle) {
    FixtureRuntime fixture;
    fixture.runtime->start();
    REQUIRE(fixture.services.activePresentations == 11);
    REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
        {"world", "setCameraParent", {"train"}}));
    fixture.runtime->fixedUpdate();
    REQUIRE_NOTHROW(fixture.runtime->unload(false));
    CHECK(fixture.services.activePresentations == 0);
    CHECK_FALSE(fixture.services.playerParented);
    REQUIRE_NOTHROW(fixture.runtime->unload(false));
    CHECK(fixture.services.count<gameplay::DestroyRuntimeEntities>() == 1);
  }
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
  CHECK_FALSE(train->transform.rotation == physics::Quaternion{});
  CHECK_FALSE(train->active);
  for (int tick = 0; tick < 30; ++tick) fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("train")->transform.position.x == 6.0);
  CHECK(fixture.services.count<gameplay::StopRuntimeSound>() == 1);
}

TEST_CASE("Step 8C pendulum preserves independent authored amplitudes",
          "[step8c][pendulum]") {
  FixtureRuntime fixture;
  fixture.services.player = {1000.0, 1000.0, 1000.0};
  fixture.runtime->start();
  fixture.runtime->fixedUpdate();
  const auto pendulum = fixture.runtime->state("pend");
  REQUIRE(pendulum.has_value());
  const double wave = std::sin(2.0 / 60.0);
  CHECK(pendulum->transform.position.y == Catch::Approx(3.0 * wave));
  CHECK(pendulum->transform.rotation.x == Catch::Approx(
      std::sin(0.5 * 12.0 * wave * 3.14159265358979323846 / 180.0)));
  CHECK(std::abs(pendulum->transform.rotation.y) > 0.0);
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

TEST_CASE("Door acceleration starts zero-speed rotators and survives save restoration",
          "[step8c][door][rotator][lua]") {
  FixtureRuntime fixture;
  fixture.services.player = {1000, 1000, 1000};
  fixture.runtime->start();
  scripting::ScriptEngine scripts(
      {fixture.paths.contentRoot(), fixture.paths.userRoot(), 10000},
      [&fixture](const scripting::ScriptCall &call) {
        return fixture.runtime->dispatchScriptCall(call);
      });
  scripts.executeText(
      "setRotSpeed('rot','0'); setAccDoor('rot','5'); toggleDoor('rot')",
      "accelerate-wheel.lua");
  fixture.runtime->fixedUpdate();
  fixture.runtime->fixedUpdate();
  // Speeds 5/60 and 10/60 deg/s are applied before rotation, with the
  // original func_door factor of five: total angle = -25/1200 degrees.
  const auto expectedY = [](double degrees) {
    return -std::sin(degrees * 3.14159265358979323846 / 360.0);
  };
  CHECK(fixture.runtime->state("rot")->transform.rotation.y ==
        Catch::Approx(expectedY(25.0 / 1200.0)));
  const auto saved = fixture.runtime->saveState();
  fixture.runtime->fixedUpdate();
  const auto continued = fixture.runtime->state("rot")->transform.rotation;
  fixture.runtime->restoreState(saved);
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("rot")->transform.rotation == continued);
  CHECK(fixture.services.count<gameplay::DeferredLegacyCommand>() == 0);
}

TEST_CASE("Stations02 pandus2 script accelerates all fourteen vehicle wheels",
          "[step8c][rotator][content][lua]") {
  const auto root = sourceRoot / "Games/The Long Way/TheLongWay";
  const auto script = root / "run3/lua/chapters/tlwstations02/pandus2.lua";
  if (!fs::is_regular_file(script)) SKIP("optional author content not attached");
  auto paths = AppPaths::resolve(sourceRoot / "build/wheel-regression.exe", root,
                                sourceRoot / "build/wheel-regression-user");
  paths.setContentOverlay(sourceRoot / "derived-content/tlwrm");
  const auto definition = content::loadMapDefinition(paths, "tlwstations02", "high");
  gameplay::EntityRegistry registry;
  static_cast<void>(gameplay::populateEntityRegistry(definition, registry, true));
  FakeServices services;
  services.player = {1000000, 1000000, 1000000};
  gameplay::SequenceRuntime runtime(definition, registry, services);
  runtime.start();
  std::map<std::string, physics::Transform> wheels;
  for (const auto &state : runtime.states()) {
    if (state.tag == "rot" && state.name.rfind("wheel", 0) == 0) {
      CHECK_FALSE(state.active);
      wheels.emplace(state.name, state.transform);
    }
  }
  REQUIRE(wheels.size() == 14);
  scripting::ScriptEngine scripts(
      {paths.contentRoot(), paths.userRoot(), 10000},
      [&runtime](const scripting::ScriptCall &call) {
        return runtime.dispatchScriptCall(call);
      });
  scripts.executeFile(script);
  for (int tick = 0; tick < 60; ++tick) runtime.fixedUpdate();
  for (const auto &[name, initial] : wheels) {
    INFO(name);
    const auto wheel = runtime.state(name);
    REQUIRE(wheel);
    CHECK(wheel->active);
    CHECK_FALSE(wheel->transform.rotation == initial.rotation);
    CHECK(wheel->transform.position == initial.position);
  }
  for (const auto &command : services.commands) {
    const auto *deferred = std::get_if<gameplay::DeferredLegacyCommand>(&command);
    if (deferred) CHECK(deferred->name != "setAccDoor");
  }
}

TEST_CASE("Train position-save mode holds, persists and releases the platform",
          "[step8c][train][save]") {
  FixtureRuntime fixture;
  fixture.runtime->start();
  fixture.runtime->fixedUpdate();
  const auto position = fixture.runtime->state("train")->transform.position;
  static_cast<void>(fixture.runtime->dispatchScriptCall({"sequence", "enableTrainSaveMode", {"train"}}));
  const auto saved = fixture.runtime->serializeState();
  for (int i = 0; i < 5; ++i) fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("train")->transform.position == position);
  static_cast<void>(fixture.runtime->dispatchScriptCall({"sequence", "disableTrainSaveMode", {"train"}}));
  fixture.runtime->fixedUpdate();
  CHECK_FALSE(fixture.runtime->state("train")->transform.position == position);
  fixture.runtime->restoreSerializedState(saved);
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->state("train")->transform.position == position);
  CHECK(fixture.services.count<gameplay::DeferredLegacyCommand>() == 0);
}

TEST_CASE("Stations03 chapter Lua commands execute without deferred operations",
          "[step8c][stations03][content][lua]") {
  const auto root = sourceRoot / "Games/The Long Way/TheLongWay";
  const auto chapter = root / "run3/lua/chapters/tlwstations03";
  if (!fs::is_directory(chapter)) SKIP("optional author content not attached");
  auto paths = AppPaths::resolve(sourceRoot / "build/stations03-regression.exe", root,
                                sourceRoot / "build/stations03-regression-user");
  paths.setContentOverlay(sourceRoot / "derived-content/tlwrm");
  const auto definition = content::loadMapDefinition(paths, "tlwstations03", "high");
  gameplay::EntityRegistry registry;
  static_cast<void>(gameplay::populateEntityRegistry(definition, registry, true));
  struct Query final : physics::IPhysicsQuery {
    std::vector<physics::RaycastHit> raycastAll(const physics::RaycastQuery &) const override { return {}; }
  } query;
  FakeServices services;
  gameplay::SequenceRuntime runtime(definition, registry, services);
  gameplay::NpcSystem npcs(definition, registry, query, services);
  services.npcs = &npcs;
  services.sequence = &runtime;
  npcs.start();
  runtime.start();
  scripting::ScriptEngine scripts({paths.contentRoot(), paths.userRoot(), 100000},
      [&runtime](const scripting::ScriptCall &call) { return runtime.dispatchScriptCall(call); });
  std::vector<fs::path> files;
  for (const auto &entry : fs::directory_iterator(chapter))
    if (entry.path().extension() == ".lua") files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  REQUIRE(files.size() > 10);
  for (const auto &file : files) {
    INFO(file.string());
    scripts.executeText("invstate=1; set__register('ra','100')", "chapter-test-setup");
    CHECK_NOTHROW(scripts.executeFile(file));
  }
  for (const auto setup : {"invstate=2", "invstate=5; set__register('ra','100')",
                           "invstate=5; set__register('ra','0')", "invstate=7"}) {
    scripts.executeText(setup, "document-check-setup");
    CHECK_NOTHROW(scripts.executeFile(chapter / "card_scan_timer.lua"));
  }
  CHECK(services.count<gameplay::SetNpcFlashlight>() >= 1);
  scripts.executeText("invstate=5; set__register('ra','100')", "money-setup");
  scripts.executeFile(chapter / "card_scan_timer.lua");
  scripts.executeText("assert(get__register('ra')==70); assert(toString(12.9)=='12')", "money-result");
  for (const auto &command : services.commands)
    if (const auto *deferred = std::get_if<gameplay::DeferredLegacyCommand>(&command))
      FAIL_CHECK("unimplemented chapter command: " << deferred->name);
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

TEST_CASE("Step 8C legacy flashlight bindings remain typed",
          "[step8c][lua][flashlight]") {
  FixtureRuntime fixture;
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"player", "player__allowFlash", {"false"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"player", "player__toggleFlash", {}}));
  REQUIRE(fixture.services.commands.size() == 2);
  CHECK_FALSE(std::get<gameplay::SetRuntimeFlashlightAllowed>(
                  fixture.services.commands[0]).allowed);
  CHECK(std::holds_alternative<gameplay::ToggleRuntimeFlashlight>(
      fixture.services.commands[1]));
}

TEST_CASE("Step 8C gameText2 retains legacy typewriter timing",
          "[step8c][lua][ui]") {
  FixtureRuntime fixture;
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"ui", "gameText2", {"Incoming message", "0.25", "4"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"ui", "gameText2", {"Default timing"}}));
  REQUIRE(fixture.services.commands.size() == 2);
  const auto &explicitText = std::get<gameplay::SetRuntimeTypewriterText>(
      fixture.services.commands[0]);
  CHECK(explicitText.text == "Incoming message");
  CHECK(explicitText.fadeSeconds == Catch::Approx(0.25));
  CHECK(explicitText.typeSeconds == Catch::Approx(4.0));
  const auto &defaultText = std::get<gameplay::SetRuntimeTypewriterText>(
      fixture.services.commands[1]);
  CHECK(defaultText.text == "Default timing");
  CHECK(defaultText.fadeSeconds == Catch::Approx(0.5));
  CHECK(defaultText.typeSeconds == Catch::Approx(3.0));
}

TEST_CASE("Step 8C legacy Lua compositor bindings retain names and parameters",
          "[step8c][lua][compositor]") {
  FixtureRuntime fixture;
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "setCompositorEnabled", {"true", "RunLSD"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "fragmentGPUProgramParams",
       {"Ogre/Compositor/BloomBlend2", "BlurWeight", "0.3"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "dssao", {}}));

  REQUIRE(fixture.services.commands.size() == 3);
  const auto &effect =
      std::get<gameplay::SetRuntimeCompositor>(fixture.services.commands[0]);
  CHECK(effect.name == "RunLSD");
  CHECK(effect.enabled);
  const auto &parameter = std::get<gameplay::SetRuntimeShaderParameter>(
      fixture.services.commands[1]);
  CHECK(parameter.program == "Ogre/Compositor/BloomBlend2");
  CHECK(parameter.parameter == "BlurWeight");
  CHECK(parameter.value == "0.3");
  const auto &ssao =
      std::get<gameplay::SetRuntimeCompositor>(fixture.services.commands[2]);
  CHECK(ssao.name == "ssao");
  CHECK_FALSE(ssao.enabled);
}

TEST_CASE("Step 8C freezeBod bindings use typed physics commands",
          "[step8c][lua][physics]") {
  FixtureRuntime fixture;
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"physics", "freezeBod", {"parts01#0"}}));
  CHECK_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"physics", "unfreezeBod", {"parts01#0"}}));
  REQUIRE(fixture.services.commands.size() == 2);
  const auto &freeze = std::get<gameplay::SetRuntimeBodyFrozen>(
      fixture.services.commands[0]);
  const auto &unfreeze = std::get<gameplay::SetRuntimeBodyFrozen>(
      fixture.services.commands[1]);
  CHECK(freeze.name == "parts01#0");
  CHECK(freeze.frozen);
  CHECK(unfreeze.name == "parts01#0");
  CHECK_FALSE(unfreeze.frozen);
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
                                "tlwstations02", "tlwstations03",
                                "tlwdelusion05"}) {
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
      const auto computer = runtime.state("turn1");
      REQUIRE(computer.has_value());
      CHECK(std::abs(computer->transform.rotation.y) > 0.9);
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
    if (map == "tlwdelusion05") {
      REQUIRE_NOTHROW(runtime.dispatchScriptCall(
          {"sequence", "startTrain", {"coffin"}}));
      const auto before = runtime.state("coffin")->transform.rotation;
      runtime.fixedUpdate();
      const auto after = runtime.state("coffin")->transform.rotation;
      CHECK_FALSE(after == before);
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
    if (map == "tlwstations01" || map == "tlwstations02" ||
        map == "tlwstations03") {
      const auto parentSpawn = std::find_if(
          services.commands.begin(), services.commands.end(),
          [](const gameplay::GameCommand &command) {
            const auto *spawn =
                std::get_if<gameplay::SpawnRuntimeEntity>(&command);
            return spawn != nullptr && spawn->spec.name == "er9m_1_1";
          });
      REQUIRE(parentSpawn != services.commands.end());
      const auto &trainSpec =
          std::get<gameplay::SpawnRuntimeEntity>(*parentSpawn).spec;
      if (map == "tlwstations02" || map == "tlwstations03") {
        CHECK_FALSE(trainSpec.parts.empty());
        CHECK(std::any_of(trainSpec.parts.begin(), trainSpec.parts.end(),
                          [](const gameplay::RuntimeVisualPartSpec &part) {
          return part.collision;
        }));
        CHECK(std::any_of(trainSpec.parts.begin(), trainSpec.parts.end(),
                          [](const gameplay::RuntimeVisualPartSpec &part) {
          return !part.collision;
        }));
      }
      scripting::ScriptEngine startup(
          {paths.contentRoot(), paths.userRoot(), 1'000'000},
          [&runtime](const scripting::ScriptCall &call) {
            return runtime.dispatchScriptCall(call);
          });
      CHECK_NOTHROW(startup.executeFile(
          "run3/lua/chapters/" + map + "/startup.lua"));
      CHECK(services.playerParented);
      const std::size_t motionBefore =
          services.count<gameplay::ApplyRuntimeParentMotion>();
      runtime.fixedUpdate();
      CHECK(services.count<gameplay::ApplyRuntimeParentMotion>() ==
            motionBefore + 1);
    }
    if (map != "tlwstations01" && map != "tlwstations02" &&
        map != "tlwstations03")
      runtime.fixedUpdate();
    runtime.unload(false);
    CHECK(services.activePresentations == 0);
    CHECK_FALSE(services.playerParented);
  }
}
