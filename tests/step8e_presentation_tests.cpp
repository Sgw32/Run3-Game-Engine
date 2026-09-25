#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/GameCommands.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;
using namespace run3;

namespace {
struct Services final : gameplay::IGameServices {
  physics::Vec3 player{};
  physics::Transform platform{{10, 0, 0}, {}};
  std::string failingScript;
  std::vector<gameplay::GameCommand> commands;
  void submit(const gameplay::GameCommand &command) override {
    commands.push_back(command);
    const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
    if (script != nullptr && script->path == failingScript)
      throw std::runtime_error("fixture script failure");
  }
  physics::Vec3 playerPosition() const override { return player; }
  physics::Vec3 playerHalfExtents() const override { return {}; }
  bool playerStandingOn(gameplay::EntityHandle) const override { return false; }
  std::optional<bool> lightVisible(std::string_view) const override {
    return std::nullopt;
  }
  std::optional<physics::Transform>
  runtimeTransform(std::string_view name) const override {
    return name == "platform" ? std::optional<physics::Transform>{platform}
                              : std::nullopt;
  }
  template <class T> std::size_t count() const {
    return static_cast<std::size_t>(std::count_if(
        commands.begin(), commands.end(), [](const auto &command) {
          return std::holds_alternative<T>(command);
        }));
  }
};

struct ReadyFixture {
  AppPaths paths;
  content::MapDefinition definition;
  gameplay::EntityRegistry registry;
  Services services;
  std::unique_ptr<gameplay::SequenceRuntime> runtime;
  ReadyFixture()
      : paths(AppPaths::resolve(
            fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e.exe",
            fs::path(RUN3_TEST_SOURCE_DIR) / "tests/fixtures/step8e/content",
            fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e-user")),
        definition(content::loadMapDefinition(paths, "fixture", "low")) {
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry,
                                                        true));
    runtime = std::make_unique<gameplay::SequenceRuntime>(definition, registry,
                                                          services);
  }
};
}

TEST_CASE("Step 8E cutscene is deterministic, skippable and restores control",
          "[step8e][cutscene]") {
  ReadyFixture fixture;
  fixture.runtime->start();
  REQUIRE(fixture.runtime->startCutscene("intro"));
  CHECK(fixture.runtime->presentation().playerFrozen);
  CHECK_FALSE(fixture.runtime->presentation().hudVisible);
  for (int tick = 0; tick < 30; ++tick) fixture.runtime->fixedUpdate();
  REQUIRE(fixture.runtime->presentation().camera.has_value());
  CHECK(fixture.runtime->presentation().camera->position.x == 5.0);
  CHECK(fixture.runtime->handleInput(
      {InputEventType::KeyPressed, Key::Space}));
  for (int tick = 0; tick < 5; ++tick) fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->presentation().activeCutscene.empty());
  CHECK_FALSE(fixture.runtime->presentation().playerFrozen);
  CHECK(fixture.runtime->presentation().hudVisible);
  CHECK(std::count_if(fixture.services.commands.begin(),
                      fixture.services.commands.end(), [](const auto &command) {
    const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
    return script && script->path == "run3/lua/cutscene-hook.lua";
  }) == 1);
}

TEST_CASE("Step 8E cutscene pose is independent of rendering frequency",
          "[step8e][cutscene][replay]") {
  const auto poseAtRate = [](int renderingHz) {
    ReadyFixture fixture;
    fixture.runtime->start();
    REQUIRE(fixture.runtime->startCutscene("intro"));
    int accumulatedTicks{};
    for (int frame = 0; frame < renderingHz / 2; ++frame) {
      accumulatedTicks += 60;
      while (accumulatedTicks >= renderingHz) {
        fixture.runtime->fixedUpdate();
        accumulatedTicks -= renderingHz;
      }
    }
    REQUIRE(fixture.runtime->presentation().camera.has_value());
    return *fixture.runtime->presentation().camera;
  };

  const physics::Transform at30 = poseAtRate(30);
  const physics::Transform at60 = poseAtRate(60);
  const physics::Transform at144 = poseAtRate(144);
  CHECK(at30.position.x == at60.position.x);
  CHECK(at60.position.x == at144.position.x);
  CHECK(at30.rotation.w == at60.rotation.w);
  CHECK(at60.rotation.w == at144.rotation.w);
}

TEST_CASE("Step 8E computer captures backend-neutral input and exits safely",
          "[step8e][computer]") {
  ReadyFixture fixture;
  fixture.services.player = {5, 0, 0};
  fixture.runtime->start();
  fixture.runtime->fixedUpdate();
  const auto terminal = fixture.registry.findFirst("terminal");
  REQUIRE(terminal.has_value());
  REQUIRE(fixture.runtime->interact(*terminal));
  CHECK(fixture.runtime->presentation().computerFocused);
  CHECK(fixture.runtime->handleInput(
      {InputEventType::TextEntered, Key::Unknown, MouseButton::None, "a"}));
  CHECK(fixture.services.count<gameplay::SendComputerInput>() == 1);
  CHECK(fixture.runtime->handleInput(
      {InputEventType::KeyPressed, Key::Escape}));
  CHECK_FALSE(fixture.runtime->presentation().computerFocused);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 3);
}

TEST_CASE("Step 8E content-proven NPC scheduler control is typed",
          "[step8e][npc]") {
  ReadyFixture fixture;
  fixture.runtime->start();
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "setNPCManagerStep", {"0.05"}}));
  CHECK(fixture.services.count<gameplay::SetNpcUpdateInterval>() == 1);
  const auto command = std::find_if(
      fixture.services.commands.begin(), fixture.services.commands.end(),
      [](const auto &value) {
        return std::holds_alternative<gameplay::SetNpcUpdateInterval>(value);
      });
  REQUIRE(command != fixture.services.commands.end());
  CHECK(std::get<gameplay::SetNpcUpdateInterval>(*command).seconds == 0.05);
}

TEST_CASE("Step 8E presentation state restores on failures and map unload",
          "[step8e][lifecycle]") {
  SECTION("cutscene hook failure") {
    ReadyFixture fixture;
    fixture.services.failingScript = "run3/lua/cutscene-hook.lua";
    fixture.runtime->start();
    REQUIRE(fixture.runtime->startCutscene("intro"));
    for (int tick = 0; tick < 59; ++tick) fixture.runtime->fixedUpdate();
    CHECK_THROWS(fixture.runtime->fixedUpdate());
    CHECK(fixture.runtime->presentation().activeCutscene.empty());
    CHECK_FALSE(fixture.runtime->presentation().playerFrozen);
    CHECK(fixture.runtime->presentation().hudVisible);
  }

  SECTION("computer init failure") {
    ReadyFixture fixture;
    fixture.services.failingScript = "run3/lua/computer-init.lua";
    fixture.runtime->start();
    const auto terminal = fixture.registry.findFirst("terminal");
    REQUIRE(terminal.has_value());
    CHECK_THROWS(fixture.runtime->interact(*terminal));
    CHECK_FALSE(fixture.runtime->presentation().computerFocused);
    CHECK_FALSE(fixture.runtime->presentation().playerFrozen);
    CHECK(fixture.runtime->presentation().hudVisible);
  }

  SECTION("active computer map unload") {
    ReadyFixture fixture;
    fixture.runtime->start();
    const auto terminal = fixture.registry.findFirst("terminal");
    REQUIRE(terminal.has_value());
    REQUIRE(fixture.runtime->interact(*terminal));
    REQUIRE_NOTHROW(fixture.runtime->unload(false));
    CHECK_FALSE(fixture.runtime->presentation().computerFocused);
    CHECK_FALSE(fixture.runtime->presentation().playerFrozen);
    CHECK(fixture.runtime->presentation().hudVisible);
    CHECK(fixture.services.count<gameplay::DestroyRuntimeEntities>() == 1);
  }
}

TEST_CASE("Step 8E stable state and authored train binding round trip",
          "[step8e][save][parent]") {
  ReadyFixture fixture;
  fixture.runtime->start();
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"world", "setCameraParent", {"platform"}}));
  fixture.services.platform.position.x = 12;
  fixture.runtime->fixedUpdate();
  CHECK(fixture.services.count<gameplay::ApplyRuntimeParentMotion>() == 1);
  REQUIRE(fixture.runtime->startCutscene("intro"));
  fixture.runtime->fixedUpdate();
  const std::string state = fixture.runtime->serializeState();
  CHECK(state.rfind("RUN3_SEQUENCE_STATE 2", 0) == 0);
  fixture.runtime->skipCutscene();
  for (int tick = 0; tick < 10; ++tick) fixture.runtime->fixedUpdate();
  REQUIRE_NOTHROW(fixture.runtime->restoreSerializedState(state));
  CHECK(fixture.runtime->presentation().activeCutscene == "intro");
  CHECK(fixture.runtime->presentation().camera.has_value());
}

TEST_CASE("Step 8E chapter transition cancels presentation before request",
          "[step8e][transition]") {
  ReadyFixture fixture;
  fixture.runtime->start();
  REQUIRE(fixture.runtime->startCutscene("intro"));
  REQUIRE_NOTHROW(fixture.runtime->dispatchScriptCall(
      {"sequence", "startEvent", {"transition"}}));
  fixture.runtime->fixedUpdate();
  CHECK(fixture.runtime->presentation().activeCutscene.empty());
  CHECK(fixture.runtime->presentation().hudVisible);
  CHECK(fixture.services.count<gameplay::ChangeRuntimeMap>() == 1);
}

TEST_CASE("Step 8E selected content constructs every computer and cutscene",
          "[step8e][content]") {
  const fs::path root = fs::path(RUN3_TEST_SOURCE_DIR) /
                        "Games/The Long Way/TheLongWay";
  if (!fs::is_directory(root)) SKIP("optional author content not attached");
  const AppPaths paths = AppPaths::resolve(
      fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e-content.exe", root,
      fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e-content-user");
  for (const auto &[map, computers, cutscenes] :
       std::vector<std::tuple<std::string, int, int>>{{"tlwcao", 4, 1},
                                                      {"tlwhome02", 5, 3}}) {
    const auto definition = content::loadMapDefinition(paths, map, "low");
    gameplay::EntityRegistry registry;
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry,
                                                        false));
    Services services;
    gameplay::SequenceRuntime runtime(definition, registry, services);
    CHECK(std::count_if(runtime.states().begin(), runtime.states().end(),
                        [](const auto &state) { return state.tag == "computer"; })
          == computers);
    int actualCutscenes{};
    const auto declarations = content::sequenceDeclarations(definition);
    for (const auto *declaration : declarations) {
      actualCutscenes += declaration->tag == "cutscene";
      if (declaration->tag == "computer") {
        for (const std::string_view key : {"script", "cNearScript",
                                           "cShutScript"}) {
          const std::string *script = declaration->attribute(key);
          if (script != nullptr && !script->empty()) {
            INFO(map << " computer script " << *script);
            CHECK(fs::is_regular_file(root / *script));
          }
        }
      }
    }
    CHECK(actualCutscenes == cutscenes);
    runtime.start();
    for (const auto *declaration : declarations) {
      const std::string *name = declaration->attribute("name");
      if (name == nullptr) continue;
      if (declaration->tag == "computer") {
        const auto handle = registry.findFirst(*name);
        REQUIRE(handle.has_value());
        INFO(map << " computer " << *name);
        CHECK(runtime.interact(*handle));
        CHECK(runtime.presentation().computerFocused);
        CHECK(runtime.exitComputer());
      } else if (declaration->tag == "cutscene") {
        INFO(map << " cutscene " << *name);
        CHECK(runtime.startCutscene(*name));
        runtime.fixedUpdate();
        CHECK(runtime.presentation().activeCutscene == *name);
        CHECK(runtime.skipCutscene());
        for (int tick = 0;
             tick < 600 && !runtime.presentation().activeCutscene.empty();
             ++tick) {
          runtime.fixedUpdate();
        }
        CHECK(runtime.presentation().activeCutscene.empty());
      }
    }
    for (const auto *event : content::sequenceEvents(definition)) {
      if (event->tag != "cutscene") continue;
      CHECK(std::any_of(declarations.begin(), declarations.end(),
                        [event](const auto *declaration) {
        return declaration->tag == "cutscene" &&
               declaration->attribute("name") != nullptr &&
               event->attribute("name") != nullptr &&
               *declaration->attribute("name") == *event->attribute("name");
      }));
      for (const auto &run : event->children) {
        if (run.tag != "run") continue;
        const std::string *script = run.attribute("script");
        REQUIRE(script != nullptr);
        INFO(map << " cutscene hook " << *script);
        CHECK(fs::is_regular_file(root / *script));
      }
    }
  }
}

TEST_CASE("Step 8E real stations chapter transition keeps authored target",
          "[step8e][content][transition]") {
  const fs::path root = fs::path(RUN3_TEST_SOURCE_DIR) /
                        "Games/The Long Way/TheLongWay";
  if (!fs::is_directory(root)) SKIP("optional author content not attached");
  const AppPaths paths = AppPaths::resolve(
      fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e-transition.exe", root,
      fs::path(RUN3_TEST_SOURCE_DIR) / "build/step8e-transition-user");
  const auto definition = content::loadMapDefinition(paths, "tlwstations01",
                                                      "low");
  gameplay::EntityRegistry registry;
  static_cast<void>(gameplay::populateEntityRegistry(definition, registry,
                                                      false));
  Services services;
  services.player = {-7775.07, -14958.5, -40063.7};
  gameplay::SequenceRuntime runtime(definition, registry, services);
  runtime.start();
  runtime.fixedUpdate();
  const auto transition = std::find_if(
      services.commands.begin(), services.commands.end(),
      [](const auto &command) {
        const auto *change = std::get_if<gameplay::ChangeRuntimeMap>(&command);
        return change != nullptr && change->map == "tlwstations02";
      });
  CHECK(transition != services.commands.end());
  CHECK(runtime.presentation().activeCutscene.empty());
  CHECK_FALSE(runtime.presentation().computerFocused);
}
