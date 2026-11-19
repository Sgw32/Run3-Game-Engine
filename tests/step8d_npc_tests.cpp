#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/NpcFacialAnimation.hpp>
#include <run3/gameplay/NpcBehavior.hpp>
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
  std::optional<physics::Vec3> npcHalfExtents;
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
  std::optional<physics::Vec3> runtimeNpcHalfExtents(gameplay::EntityHandle) const override {
    return npcHalfExtents;
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

TEST_CASE("Step 8D home NPCs and navigation use the original coordinate multipliers",
          "[step8d][npc][content][regression]") {
  const auto root = sourceRoot / "Games/The Long Way/TheLongWay";
  if (!fs::is_directory(root)) SKIP("optional author content not attached");
  for (const bool derived : {false, true}) {
    CAPTURE(derived);
    auto paths = AppPaths::resolve(sourceRoot / "build/npc-multiplier-test.exe",
        root, sourceRoot / "build/npc-multiplier-test-user");
    if (derived) paths.setContentOverlay(sourceRoot / "derived-content/tlwrm");
    const auto map = content::loadMapDefinition(paths, "tlwhome01", "high");
    gameplay::EntityRegistry registry;
    static_cast<void>(gameplay::populateEntityRegistry(map, registry));
    Query query;
    Services services;
    gameplay::NpcSystem npcs(map, registry, query, services);
    npcs.start();
    const double spawnY = -7576;
    const std::map<std::string, physics::Vec3> expected{
        {"first_npsc1", {4178.4, spawnY, 3091.2}},
        {"police01", {1600, spawnY, 720}},
        {"police02", {1760, spawnY, 720}},
        {"police03", {1920, spawnY, 768}},
        {"police04", {1600, spawnY, 240}},
        {"police05", {1680, spawnY, 240}},
        {"police06", {2080, spawnY, 240}}};
    REQUIRE(npcs.size() == expected.size());
    for (const auto &command : services.commands) {
      const auto *spawn = std::get_if<gameplay::SpawnRuntimeEntity>(&command);
      if (spawn == nullptr) continue;
      CAPTURE(spawn->spec.name);
      const auto &position = expected.at(spawn->spec.name);
      CHECK(spawn->spec.transform.position.x == Catch::Approx(position.x));
      CHECK(spawn->spec.transform.position.y == Catch::Approx(position.y));
      CHECK(spawn->spec.transform.position.z == Catch::Approx(position.z));
      CHECK(spawn->spec.autoPositionOffsetY == (derived ? 30 : 0));
      // Scene multiplier must not scale sequence mesh sizes a second time.
      CHECK(spawn->spec.scale.y == (spawn->spec.name == "first_npsc1" ? 2 : 80));
    }
    const std::vector<physics::Vec3> nodes{
        {2400, -7600, 880}, {1280, -7600, 880}, {1600, -7600, 720},
        {2465.6, -7600, 1917.6}, {1600, -7600, 1917.6},
        {2000, -7600, 1917.6}, {4178.4, -7576, 3091.2},
        {7872.8, -7576, 3091.2}};
    REQUIRE(npcs.navigationNodes().size() == nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      CAPTURE(i);
      const auto &actual = npcs.navigationNodes()[i].position;
      CHECK(actual.x == Catch::Approx(nodes[i].x));
      CHECK(actual.y == Catch::Approx(nodes[i].y));
      CHECK(actual.z == Catch::Approx(nodes[i].z));
    }
  }
}

TEST_CASE("Step 8D navigation preserves the Bullet-owned vertical position",
          "[step8d][npc][physics]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 20, "0.7071067811865476 0 -0.7071067811865476 0", {}, false});
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
  CHECK(spec.strangeLook);
  CHECK(spec.animationTimeMultiplier >= 0.9);
  CHECK(spec.animationTimeMultiplier < 1.1);
  CHECK(spec.behaviorSeed != 0);
  CHECK(spec.headBone == "GuideHead");
  CHECK(spec.headAxis.x == Catch::Approx(1.0));
  CHECK(spec.headAxis.y == Catch::Approx(0.0));
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
  CHECK(raiderSpec.animationTimeMultiplier >= 0.9);
  CHECK(raiderSpec.animationTimeMultiplier < 1.1);
  CHECK(raiderSpec.animationTimeMultiplier !=
        Catch::Approx(spec.animationTimeMultiplier));
  const auto handle = fixture.npcs->state("guide")->handle;
  fixture.npcs->dispatch({"guide", 19, "run3/lua/use.lua", {}, false});
  CHECK(fixture.npcs->use(handle));
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
  fixture.npcs->unload();
  CHECK(fixture.services.count<gameplay::DestroyRuntimeEntity>() == 2);
  CHECK(fixture.npcs->size() == 0);
}

TEST_CASE("NPC flashlight toggles without changing navigation and survives restore",
          "[step8d][npc][flashlight]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  const auto before = fixture.npcs->state("guide");
  fixture.npcs->dispatch({"guide", 28, "light", {}, false});
  REQUIRE(fixture.services.count<gameplay::SetNpcFlashlight>() == 1);
  CHECK(fixture.npcs->state("guide")->state == before->state);
  CHECK(fixture.npcs->state("guide")->animation == before->animation);
  const auto saved = fixture.npcs->serializeState();
  fixture.npcs->dispatch({"guide", 28, "light", {}, false});
  const auto lastLight = [&]() {
    for (auto i = fixture.services.commands.rbegin(); i != fixture.services.commands.rend(); ++i)
      if (const auto *light = std::get_if<gameplay::SetNpcFlashlight>(&*i)) return light->enabled;
    return false;
  };
  CHECK_FALSE(lastLight());
  fixture.npcs->restoreSerializedState(saved);
  // Restore also submits an off state for the other NPC.
  bool guideOn{};
  for (const auto &command : fixture.services.commands)
    if (const auto *light = std::get_if<gameplay::SetNpcFlashlight>(&command);
        light && light->owner == before->handle) guideOn = light->enabled;
  CHECK(guideOn);
  fixture.npcs->dispatch({"guide", 6, "", {}, false});
  CHECK_FALSE(lastLight());
}

TEST_CASE("Step 8D NPC animation variation and strange look are deterministic",
          "[step8d][npc][look]") {
  using namespace gameplay;
  const std::uint64_t seed =
      stableNpcBehaviorSeed("guide", "maps/fixture.xml", 7);
  CHECK(seed == stableNpcBehaviorSeed("guide", "maps/fixture.xml", 7));
  CHECK(seed != stableNpcBehaviorSeed("other", "maps/fixture.xml", 7));
  CHECK(npcAnimationTimeMultiplier(seed) >= 0.9);
  CHECK(npcAnimationTimeMultiplier(seed) < 1.1);

  NpcLookState state;
  CHECK(updateNpcLookState(state, true, 0.0, seed).target ==
        NpcLookTarget::Player);
  const double first = npcLookPhaseDuration(
      seed, NpcLookPhase::PlayerFirst, 0);
  CHECK(updateNpcLookState(state, true, first, seed).target ==
        NpcLookTarget::Forward);
  const double forward = npcLookPhaseDuration(
      seed, NpcLookPhase::Forward, 0);
  CHECK(updateNpcLookState(state, true, forward, seed).target ==
        NpcLookTarget::Player);
  const double second = npcLookPhaseDuration(
      seed, NpcLookPhase::PlayerSecond, 0);
  const auto elsewhere = updateNpcLookState(state, true, second, seed);
  CHECK(elsewhere.target == NpcLookTarget::Elsewhere);
  CHECK(std::abs(elsewhere.elsewhereYawDegrees) >= 20.0);
  CHECK(std::abs(elsewhere.elsewhereYawDegrees) <= 40.0);

  CHECK(updateNpcLookState(state, false, 1.0, seed).target ==
        NpcLookTarget::Forward);
  CHECK_FALSE(state.engaged);
  CHECK(updateNpcLookState(state, true, 0.0, seed).target ==
        NpcLookTarget::Player);
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
  // Allow the original 50-degree/second turn toward +X before walking.
  for (int tick = 0; tick < 120; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Reached);
  CHECK(fixture.npcs->state("guide")->transform.position.x == Catch::Approx(9));
  CHECK(std::abs(fixture.npcs->state("guide")->transform.rotation.y) > 0.5);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);
  CHECK(replayAt(30).x == replayAt(60).x);
  CHECK(replayAt(60).x == replayAt(144).x);
}

TEST_CASE("NPC arrival stays stopped and an opposite goal turns before walking",
          "[step8d][npc][navigation][turn]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "0 0 -10", {}, false});
  for (int tick = 0; tick < 60; ++tick) fixture.npcs->fixedUpdate();
  REQUIRE(fixture.npcs->state("guide")->state == gameplay::NpcState::Reached);
  const auto arrived = fixture.npcs->state("guide")->transform.position;
  for (int tick = 0; tick < 120; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position == arrived);
  CHECK(fixture.services.count<gameplay::RunRuntimeScript>() == 1);

  // This is the inspector's arrival pattern: the reach Lua orders a goal
  // behind him, then a timer issues Stop. He must turn, not instantly retreat.
  fixture.npcs->dispatch({"guide", 10, "0 0 100", {}, false});
  for (int tick = 0; tick < 120; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position == arrived);
  CHECK(fixture.npcs->state("guide")->animation == "Stealth");
  const auto turning = fixture.npcs->state("guide")->transform.rotation;
  const double angle = 100.0 * 3.14159265358979323846 / 180.0;
  CHECK(std::abs(turning.y) == Catch::Approx(std::sin(angle / 2)).margin(1e-8));
  fixture.npcs->dispatch({"guide", 4, "Idle1", {}, false});
  for (int tick = 0; tick < 240; ++tick) fixture.npcs->fixedUpdate();
  CHECK(fixture.npcs->state("guide")->transform.position == arrived);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Idle);
}

TEST_CASE("Step 8E NPC scheduler interval retains fixed-tick determinism",
          "[step8d][step8e][npc][scheduler]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 20, "0.7071067811865476 0 -0.7071067811865476 0", {}, false});
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

TEST_CASE("NPCs use fitted body clearance for search and active route checks", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  auto world = physics::createBulletPhysicsWorld();
  physics::BodyDesc wall{physics::Shape::box({50, 100, 10})};
  wall.group = physics::CollisionGroup::World;
  wall.transform.position = {100, 0, 30};
  const auto left = world.createBody(wall);
  wall.transform.position.z = -30;
  const auto right = world.createBody(wall);
  physics::WorldPhysicsQuery query(world);
  for (const double width : {15.0, 30.0}) {
    CAPTURE(width);
    gameplay::EntityRegistry registry;
    static_cast<void>(gameplay::populateEntityRegistry(fixture.definition, registry));
    Services services;
    services.npcHalfExtents = physics::Vec3{width, 75.9, width};
    gameplay::NpcSystem npcs(fixture.definition, registry, query, services);
    npcs.start();
    npcs.dispatch({"guide", 10, "200 0 0", {}, false});
    for (int tick = 0; tick < 100; ++tick) npcs.fixedUpdate(0.1);
    CHECK(npcs.state("guide")->state == (width == 15.0
        ? gameplay::NpcState::Reached : gameplay::NpcState::Blocked));
  }
}

TEST_CASE("Step 8D blocked scripted goals idle and retry each second", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  CHECK(fixture.npcs->state("guide")->animation == "Walk");
  fixture.query.blockEverything = true;
  fixture.npcs->fixedUpdate(0.1);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Blocked);
  CHECK(fixture.npcs->state("guide")->animation == "Idle1");
  const auto stopped = fixture.npcs->state("guide")->transform.position;
  fixture.query.blockEverything = false;
  fixture.npcs->fixedUpdate(0.9);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Blocked);
  CHECK(fixture.npcs->state("guide")->transform.position == stopped);
  fixture.npcs->fixedUpdate(0.1);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Navigating);
  for (int tick = 0; tick < 100; ++tick) fixture.npcs->fixedUpdate(0.1);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Reached);
  CHECK(fixture.npcs->state("guide")->animation == "Idle1");
  const auto reached = std::count_if(fixture.services.commands.begin(),
      fixture.services.commands.end(), [](const gameplay::GameCommand &command) {
        const auto *script = std::get_if<gameplay::RunRuntimeScript>(&command);
        return script && script->path == "run3/lua/reach.lua";
      });
  CHECK(reached == 1);
}

TEST_CASE("Step 8D failed initial paths recover and explicit stop cancels retries", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  fixture.query.blockEverything = true;
  fixture.npcs->dispatch({"guide", 10, "10 0 0", {}, false});
  fixture.npcs->fixedUpdate(1.0);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Blocked);
  fixture.query.blockEverything = false;
  fixture.npcs->fixedUpdate(1.0);
  CHECK(fixture.npcs->state("guide")->state != gameplay::NpcState::Blocked);
  fixture.query.blockEverything = true;
  fixture.npcs->dispatch({"guide", 10, "100 0 0", {}, false});
  fixture.npcs->dispatch({"guide", 4, {}, {}, false});
  fixture.query.blockEverything = false;
  fixture.npcs->fixedUpdate(1.0);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Idle);
}

TEST_CASE("Step 8D physics obstruction stops walking even with a clear ray", "[step8d][npc][navigation]") {
  RuntimeFixture fixture;
  fixture.services.transforms["guide"] = {};
  fixture.npcs->dispatch({"guide", 10, "100 0 0", {}, false});
  for (int tick = 0; tick < 40; ++tick) fixture.npcs->fixedUpdate(0.1);
  CHECK(fixture.npcs->state("guide")->state == gameplay::NpcState::Blocked);
  CHECK(fixture.npcs->state("guide")->animation == "Idle1");
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
  CHECK(saved.rfind("RUN3_NPC_STATE 2", 0) == 0);
  CHECK_NOTHROW(fixture.npcs->dispatch({"guide", 26, "1", {}, false}));
  CHECK(fixture.npcs->state("guide")->gravityEnabled);
  CHECK_NOTHROW(fixture.npcs->restoreSerializedState(saved));
  CHECK_FALSE(fixture.npcs->state("guide")->gravityEnabled);
}
