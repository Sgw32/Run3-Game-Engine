#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/NpcFacialAnimation.hpp>
#include <run3/gameplay/NpcSystem.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
#include <regex>
#include <string>
#include <variant>
#include <vector>

namespace fs = std::filesystem;
using namespace run3;

namespace {
const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};

struct Query final : physics::IPhysicsQuery {
  bool blockDirect{};
  bool blockEverything{};
  std::vector<physics::RaycastHit>
  raycastAll(const physics::RaycastQuery &query) const override {
    if (blockEverything ||
        (blockDirect && query.from.z == 0 && query.to.z == 0 &&
         query.from.x != query.to.x))
      return {{1, {1, physics::BodyType::World, 0}, {}, {}, 0.5, false}};
    return {};
  }
};

struct Services final : gameplay::IGameServices {
  physics::Vec3 player{1000, 0, 0};
  std::map<std::string, physics::Transform> transforms;
  std::map<std::string, physics::Vec3> scales;
  std::optional<physics::Transform> settledNpc;
  bool facialActive{};
  std::size_t settleCalls{};
  std::vector<gameplay::GameCommand> commands;
  void submit(const gameplay::GameCommand &command) override {
    commands.push_back(command);
  }
  physics::Vec3 playerPosition() const override { return player; }
  physics::Vec3 playerHalfExtents() const override { return {1, 1, 1}; }
  bool playerStandingOn(gameplay::EntityHandle) const override { return false; }
  std::optional<bool> lightVisible(std::string_view) const override { return {}; }
  std::optional<physics::Transform> runtimeTransform(std::string_view name) const override {
    const auto found = transforms.find(std::string(name));
    return found == transforms.end() ? std::nullopt
                                     : std::optional<physics::Transform>{found->second};
  }
  physics::Vec3 runtimeScale(std::string_view name) const override {
    const auto found = scales.find(std::string(name));
    return found == scales.end() ? physics::Vec3{1, 1, 1} : found->second;
  }
  std::optional<physics::Transform>
  settleRuntimeNpc(gameplay::EntityHandle) override {
    ++settleCalls;
    return settledNpc;
  }
  bool runtimeFacialActive(gameplay::EntityHandle) const override {
    return facialActive;
  }
  template <class T> std::size_t count() const {
    return static_cast<std::size_t>(std::count_if(commands.begin(), commands.end(),
        [](const auto &command) { return std::holds_alternative<T>(command); }));
  }
};

struct RuntimeFixture {
  AppPaths paths;
  content::MapDefinition definition;
  gameplay::EntityRegistry registry;
  Query query;
  Services services;
  std::unique_ptr<gameplay::NpcSystem> npcs;
  explicit RuntimeFixture(
      std::optional<physics::Transform> settledNpc = std::nullopt)
      : paths(AppPaths::resolve(sourceRoot / "build/step8d-fixture.exe",
                                sourceRoot / "tests/fixtures/step8d/content",
                                sourceRoot / "build/step8d-user")),
        definition(content::loadMapDefinition(paths, "fixture", "low")) {
    services.settledNpc = settledNpc;
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry));
    npcs = std::make_unique<gameplay::NpcSystem>(definition, registry, query, services);
    npcs->start();
  }
};

physics::Vec3 replayAt(double renderHz) {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  double accumulator{};
  int ticks{};
  while (ticks < 120) {
    accumulator += 1.0 / renderHz;
    while (accumulator + 1e-12 >= 1.0 / 60.0 && ticks < 120) {
      fixture.npcs->fixedUpdate();
      accumulator -= 1.0 / 60.0;
      ++ticks;
    }
  }
  return fixture.npcs->state("guide")->transform.position;
}
} // namespace

TEST_CASE("Step 8D gravity NPCs receive deterministic initial floor settling",
          "[step8d][npc][physics]") {
  RuntimeFixture fixture(physics::Transform{{10, 42, 30}, {}});
  CHECK(fixture.services.settleCalls == 2);
  CHECK(fixture.npcs->state("guide")->transform.position.y ==
        Catch::Approx(42.0));
}

TEST_CASE("Step 8D navigation preserves the Bullet-owned vertical position",
          "[step8d][npc][physics]") {
  RuntimeFixture fixture;
  fixture.services.transforms["guide"] = {{0, 42, 0}, {}};
  fixture.npcs->dispatch({"guide", 10, "100 0 0", {}, false});
  fixture.npcs->fixedUpdate();
  const auto state = fixture.npcs->state("guide");
  REQUIRE(state.has_value());
  CHECK(state->transform.position.y == Catch::Approx(42.0));
  CHECK(state->transform.position.x > 0.0);
}

TEST_CASE("Step 8D player contacts cannot push NPCs off their route",
          "[step8d][npc][physics]") {
  RuntimeFixture fixture;
  const auto intended = fixture.npcs->state("guide")->transform.position;
  fixture.services.commands.clear();
  fixture.services.transforms["guide"] = {
      {intended.x + 25.0, intended.y - 3.0, intended.z - 12.0}, {}};
  fixture.npcs->fixedUpdate();
  const auto state = fixture.npcs->state("guide");
  REQUIRE(state.has_value());
  CHECK(state->transform.position.x == Catch::Approx(intended.x));
  CHECK(state->transform.position.y == Catch::Approx(intended.y - 3.0));
  CHECK(state->transform.position.z == Catch::Approx(intended.z));
  CHECK(fixture.services.count<gameplay::SetRuntimeTransform>() == 1);
}

TEST_CASE("Step 8D facial playback pauses and resumes NPC navigation",
          "[step8d][npc][facial]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "100 0 0", {}, false});
  fixture.services.facialActive = true;
  fixture.npcs->fixedUpdate();
  const double pausedX = fixture.npcs->state("guide")->transform.position.x;
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x ==
        Catch::Approx(pausedX));
  fixture.services.facialActive = false;
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x > pausedX);
}

TEST_CASE("Step 8D constructs typed neutral and enemy NPCs", "[step8d][npc]") {
  RuntimeFixture fixture;
  REQUIRE(fixture.npcs->size() == 2);
  CHECK(fixture.npcs->state("guide")->npcClass == gameplay::NpcClass::Neutral);
  CHECK(fixture.npcs->state("raider")->npcClass == gameplay::NpcClass::Enemy);
  CHECK(fixture.services.count<gameplay::SpawnRuntimeEntity>() == 2);
  CHECK(fixture.services.count<gameplay::PlayRuntimeAnimation>() == 2);
  const auto spawned = std::find_if(
      fixture.services.commands.begin(), fixture.services.commands.end(),
      [](const gameplay::GameCommand &command) {
        const auto *spawn = std::get_if<gameplay::SpawnRuntimeEntity>(&command);
        return spawn != nullptr && spawn->spec.name == "guide";
      });
  REQUIRE(spawned != fixture.services.commands.end());
  const auto &spec = std::get<gameplay::SpawnRuntimeEntity>(*spawned).spec;
  CHECK(spec.autoPosition);
  CHECK(spec.autoPositionAnimation == "Walk");
  CHECK(spec.gravityEnabled);
  CHECK(spec.visualOffset.y == Catch::Approx(-8.0));
  CHECK(spec.collisionScale.x == Catch::Approx(0.3));
  CHECK(spec.collisionScale.y == Catch::Approx(0.7));
  CHECK(spec.autoPositionCorrection.x == Catch::Approx(1.1));
  CHECK(spec.autoPositionCorrection.y == Catch::Approx(0.9));
  CHECK(spec.autoPositionCorrection.z == Catch::Approx(1.2));
  CHECK(spec.visualRotationAxis.y == Catch::Approx(1.0));
  CHECK(spec.visualRotationDegrees == Catch::Approx(15.0));
  CHECK(spec.castShadows);
  const auto raiderSpawned = std::find_if(
      fixture.services.commands.begin(), fixture.services.commands.end(),
      [](const gameplay::GameCommand &command) {
        const auto *spawn = std::get_if<gameplay::SpawnRuntimeEntity>(&command);
        return spawn != nullptr && spawn->spec.name == "raider";
      });
  REQUIRE(raiderSpawned != fixture.services.commands.end());
  const auto &raiderSpec =
      std::get<gameplay::SpawnRuntimeEntity>(*raiderSpawned).spec;
  CHECK_FALSE(raiderSpec.autoPosition);
  CHECK_FALSE(raiderSpec.castShadows);
  const auto handle = fixture.npcs->state("guide")->handle;
  fixture.npcs->dispatch({"guide", 19, "run3/lua/use.lua", {}, false});
  CHECK(fixture.npcs->use(handle));
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
  fixture.npcs->unload();
  CHECK(fixture.services.count<gameplay::DestroyRuntimeEntity>() == 2);
  CHECK(fixture.npcs->size() == 0);
}

TEST_CASE("Step 8D near script fires once and explicit use stays separate",
          "[step8d][npc][lua]") {
  RuntimeFixture fixture;
  const auto handle = fixture.npcs->state("guide")->handle;
  CHECK_FALSE(fixture.npcs->use(handle));
  fixture.services.player = {0, 0, 50};
  fixture.npcs->fixedUpdate();
  fixture.npcs->fixedUpdate();
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
}

TEST_CASE("Step 8D parent motion, teleport and animation commands are deterministic",
          "[step8d][npc][parent]") {
  RuntimeFixture fixture;
  fixture.services.transforms["train"] = {{10, 0, 0}, {}};
  fixture.npcs->dispatch({"guide", 17, "train", {}, false});
  fixture.services.transforms["train"] = {{20, 0, 0}, {}};
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x == Catch::Approx(10));
  fixture.services.scales["train"] = {2, 3, 4};
  fixture.npcs->dispatch({"guide", 21, "5 0 0", {}, false});
  CHECK(fixture.npcs->state("guide")->transform.position.x == Catch::Approx(30));
  fixture.npcs->dispatch({"guide", 25, {}, {}, false});
  fixture.npcs->dispatch({"guide", 15, "Sitting", {}, false});
  CHECK(fixture.npcs->state("guide")->animation == "Sitting");
  fixture.services.transforms["train"] = {{30, 0, 0}, {}};
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x == Catch::Approx(30));
}

TEST_CASE("Step 8D facial definitions blend legacy poses on the voice timeline",
          "[step8d][npc][facial]") {
  const auto definition = gameplay::loadFacialAnimationDefinition(
      sourceRoot / "tests/fixtures/step8d/facial-animation.xml");
  CHECK(definition.sound == fs::path("run3/sounds/speech/test.wav"));
  CHECK(definition.subtitle == "A test line.");
  CHECK(definition.patched);
  REQUIRE(definition.phonemes.size() == 3);
  CHECK(definition.durationSeconds() == Catch::Approx(1.0));
  CHECK(gameplay::facialPoseIndex("A") == 2);
  CHECK(gameplay::facialPoseIndex("L") == -1);

  const auto lead = gameplay::sampleFacialAnimation(definition, -0.1);
  REQUIRE(lead.active);
  REQUIRE(lead.influences.size() == 1);
  CHECK(lead.influences[0].poseIndex == 2);
  CHECK(lead.influences[0].weight == Catch::Approx(0.5F));

  const auto transition = gameplay::sampleFacialAnimation(definition, 0.25);
  REQUIRE(transition.influences.size() == 2);
  CHECK(transition.influences[0].poseIndex == 2);
  CHECK(transition.influences[0].weight == Catch::Approx(0.5F));
  CHECK(transition.influences[1].poseIndex == 3);
  CHECK(transition.influences[1].weight == Catch::Approx(0.5F));

  const auto tail = gameplay::sampleFacialAnimation(definition, 1.1);
  REQUIRE(tail.active);
  REQUIRE(tail.influences.size() == 1);
  CHECK(tail.influences[0].poseIndex == 4);
  CHECK(tail.influences[0].weight == Catch::Approx(0.5F));
  CHECK_FALSE(gameplay::sampleFacialAnimation(definition, 1.21).active);
}

TEST_CASE("Step 8D attached campaign facial definitions and voices resolve",
          "[step8d][npc][facial][content]") {
  const fs::path contentRoot = sourceRoot / "Games/The Long Way/TheLongWay";
  const fs::path sounds = contentRoot / "run3/sounds";
  if (!fs::exists(sounds)) SKIP("TLW content absent");
  std::size_t definitions{};
  for (const auto &entry : fs::recursive_directory_iterator(sounds)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".xml") continue;
    CAPTURE(entry.path().generic_string());
    const auto definition =
        gameplay::loadFacialAnimationDefinition(entry.path());
    CHECK_FALSE(definition.phonemes.empty());
    const fs::path exact = contentRoot / definition.sound;
    const fs::path legacySibling =
        entry.path().parent_path() / definition.sound.filename();
    CHECK((fs::exists(exact) || fs::exists(legacySibling)));
    ++definitions;
  }
  CHECK(definitions == 41);

  const std::regex facialEvent(
      R"npc(npcEvent2?\(\s*"[^"]+"\s*,\s*"27"\s*,\s*"([^"]+)")npc");
  std::size_t luaReferences{};
  const fs::path luaRoot = contentRoot / "run3/lua";
  for (const auto &entry : fs::recursive_directory_iterator(luaRoot)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
    std::ifstream stream(entry.path());
    std::string line;
    while (std::getline(stream, line)) {
      if (line.find("--") < line.find("npcEvent")) continue;
      std::smatch match;
      if (!std::regex_search(line, match, facialEvent)) continue;
      const fs::path definitionPath = contentRoot / match[1].str();
      CAPTURE(entry.path().generic_string(), line,
              definitionPath.generic_string());
      CHECK(fs::exists(definitionPath));
      if (fs::exists(definitionPath))
        CHECK_FALSE(gameplay::loadFacialAnimationDefinition(definitionPath)
                        .phonemes.empty());
      ++luaReferences;
    }
  }
  CHECK(luaReferences > 0);
}

TEST_CASE("Step 8D facial event reaches the typed NPC presentation command",
          "[step8d][npc][facial]") {
  RuntimeFixture fixture;
  fixture.services.commands.clear();
  fixture.npcs->dispatch(
      {"guide", 27, "run3/sounds/speech/guide.xml", {}, false});
  REQUIRE(fixture.services.count<gameplay::PlayRuntimeFacial>() == 1);
  const auto *facial = std::get_if<gameplay::PlayRuntimeFacial>(
      &fixture.services.commands.back());
  REQUIRE(facial != nullptr);
  CHECK(facial->owner == fixture.npcs->state("guide")->handle);
  CHECK(facial->definition == fs::path("run3/sounds/speech/guide.xml"));
  CHECK(facial->position.x == Catch::Approx(0.0));
}

TEST_CASE("Step 8D enemy perception, damage and ragdoll are bounded",
          "[step8d][npc][enemy]") {
  RuntimeFixture fixture;
  fixture.services.player = {0, 0, 20};
  for (int tick = 0; tick < 60; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.services.count<gameplay::DamageRuntimePlayer>() == 1);
  const auto guide = fixture.npcs->state("guide")->handle;
  CHECK(fixture.npcs->damage(guide, 5, physics::Vec3{0, 30, 0}));
  CHECK(fixture.npcs->state("guide")->health == Catch::Approx(25));
  CHECK(fixture.services.count<gameplay::RuntimeLog>() >= 1);
  const auto raider = fixture.npcs->state("raider")->handle;
  CHECK(fixture.npcs->damage(raider, 20));
  CHECK(fixture.services.count<gameplay::SpawnRuntimeRagdoll>() == 1);
}

TEST_CASE("Step 8D navigation and reach callback follow fixed ticks", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  REQUIRE(fixture.npcs->state("guide")->state == gameplay::NpcState::Navigating);
  for (int tick = 0; tick < 20; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Reached);
  CHECK(fixture.npcs->state("guide")->transform.position.x == Catch::Approx(9));
  CHECK(std::abs(fixture.npcs->state("guide")->transform.rotation.y) > 0.5);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
  CHECK(replayAt(30).x == replayAt(60).x);
  CHECK(replayAt(60).x == replayAt(144).x);
}

TEST_CASE("Step 8E NPC scheduler interval retains fixed-tick determinism",
          "[step8d][step8e][npc][scheduler]") {
  RuntimeFixture fixture;
  fixture.npcs->setUpdateInterval(0.05);
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  fixture.npcs->fixedUpdate();
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x == 0.0);
  fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position.x > 0.0);
  CHECK_THROWS(fixture.npcs->setUpdateInterval(-0.1));
  CHECK_THROWS(fixture.npcs->setUpdateInterval(2.0));
}

TEST_CASE("Step 8D blocked path and invalid commands are visible", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  fixture.query.blockEverything = true;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Blocked);
  CHECK(fixture.services.count<gameplay::RuntimeLog>() == 1);
  CHECK_THROWS(fixture.npcs->dispatch({"guide", 999, {}, {}, false}));
  CHECK_THROWS(fixture.npcs->dispatch({"guide", 10, "bad", {}, false}));
  CHECK_NOTHROW(fixture.npcs->dispatch({"missing", 4, {}, {}, false}));
  CHECK(fixture.services.count<gameplay::RuntimeLog>() == 2);
}

TEST_CASE("Step 8D damage, death, removal and unload invalidate handles", "[step8d][npc]") {
  RuntimeFixture fixture;
  const auto guide = fixture.npcs->state("guide")->handle;
  REQUIRE(fixture.npcs->damage(guide, 30, true));
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Dead);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
  CHECK_FALSE(fixture.npcs->damage(guide, 1));
  CHECK(fixture.npcs->destroy("guide"));
  CHECK_FALSE(fixture.registry.valid(guide));
  CHECK_FALSE(fixture.npcs->destroy("guide"));
  fixture.npcs->unload();
  CHECK(fixture.services.count<gameplay::DestroyRuntimeEntity>() == 2);
}

TEST_CASE("Step 8D selected campaign NPC declaration counts", "[step8d][npc][content]") {
  const fs::path contentRoot = sourceRoot / "Games/The Long Way/TheLongWay";
  if (!fs::exists(contentRoot / "run3/maps/low/tlwcao/scene.cfg")) SKIP("TLW content absent");
  AppPaths paths = AppPaths::resolve(sourceRoot / "build/step8d-content.exe",
                                     contentRoot, sourceRoot / "build/step8d-user");
  for (const auto &[map, expected] : {std::pair{"tlwcao", 19},
                                       std::pair{"tlwhome02", 28}}) {
    auto definition = content::loadMapDefinition(paths, map, "low");
    gameplay::EntityRegistry registry;
    static_cast<void>(gameplay::populateEntityRegistry(definition, registry));
    Query query;
    Services services;
    gameplay::NpcSystem npcs(definition, registry, query, services);
    CHECK(npcs.size() == static_cast<std::size_t>(expected));
    for (const auto &npc : npcs.states()) {
      CHECK(registry.valid(npc.handle));
      CHECK_FALSE(npc.name.empty());
    }
  }
}

TEST_CASE("Step 8D selected NPC Lua targets exist", "[step8d][npc][content]") {
  const fs::path contentRoot = sourceRoot / "Games/The Long Way/TheLongWay";
  if (!fs::exists(contentRoot / "run3/maps/low/tlwcao/scene.cfg")) SKIP("TLW content absent");
  AppPaths paths = AppPaths::resolve(sourceRoot / "build/step8d-content.exe",
                                     contentRoot, sourceRoot / "build/step8d-user");
  std::size_t referenced{};
  for (const std::string map : {"tlwcao", "tlwhome02"}) {
    const auto definition = content::loadMapDefinition(paths, map, "low");
    for (const auto *element : content::sequenceDeclarations(definition)) {
      if (element->tag != "npc") continue;
      for (const std::string key : {"cNearScript", "scriptOnDeath", "scriptOnReach"}) {
        const auto *script = element->attribute(key);
        if (!script || script->empty()) continue;
        ++referenced;
        INFO(map << " NPC " << *element->attribute("name") << " " << key
                 << "=" << *script);
        CHECK(fs::exists(contentRoot / *script));
      }
    }
    const fs::path lua = contentRoot / "run3/lua/chapters" / map;
    if (!fs::exists(lua)) continue;
    const std::regex target(R"npc(npcEvent\([^,]+,\s*"(18|19)"\s*,\s*"([^"]+)")npc");
    for (const auto &entry : fs::recursive_directory_iterator(lua)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
      std::ifstream stream(entry.path());
      std::string line;
      while (std::getline(stream, line)) {
        if (line.find("--") < line.find("npcEvent")) continue;
        std::smatch match;
        if (!std::regex_search(line, match, target)) continue;
        ++referenced;
        INFO(entry.path().string() << ": " << line);
        CHECK(fs::exists(contentRoot / match[2].str()));
      }
    }
  }
  CHECK(referenced > 0);
}

TEST_CASE("Step 8E NPC gravity event and stable state round trip",
          "[step8e][npc][save]") {
  RuntimeFixture fixture;
  CHECK_NOTHROW(fixture.npcs->dispatch({"guide", 26, "0", {}, false}));
  REQUIRE(fixture.npcs->state("guide").has_value());
  CHECK_FALSE(fixture.npcs->state("guide")->gravityEnabled);
  CHECK(fixture.services.count<gameplay::SetRuntimeNpcGravity>() == 1);
  const std::string saved = fixture.npcs->serializeState();
  CHECK(saved.rfind("RUN3_NPC_STATE 1", 0) == 0);
  CHECK_NOTHROW(fixture.npcs->dispatch({"guide", 26, "1", {}, false}));
  CHECK(fixture.npcs->state("guide")->gravityEnabled);
  CHECK_NOTHROW(fixture.npcs->restoreSerializedState(saved));
  CHECK_FALSE(fixture.npcs->state("guide")->gravityEnabled);
}
