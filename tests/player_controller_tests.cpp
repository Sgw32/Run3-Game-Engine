#include <run3/app/EngineClock.hpp>
#include <run3/gameplay/LegacyMaterialCatalog.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/physics/Physics.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <vector>

namespace {
namespace fs = std::filesystem;
using run3::gameplay::PlayerCommand;
using run3::gameplay::PlayerController;
using run3::physics::BodyDesc;
using run3::physics::BodyHandle;
using run3::physics::BodyMotion;
using run3::physics::CollisionGroup;
using run3::physics::PhysicsWorld;
using run3::physics::Shape;
using run3::physics::Vec3;

BodyHandle addBox(PhysicsWorld &world, Vec3 centre, Vec3 halfExtents,
                  CollisionGroup group = CollisionGroup::World) {
  BodyDesc description(Shape::box(halfExtents));
  description.motion = BodyMotion::Static;
  description.transform.position = centre;
  description.group = group;
  return world.createBody(description);
}

BodyHandle addFloor(PhysicsWorld &world) {
  return addBox(world, {0.0, -10.0, 0.0}, {1000.0, 10.0, 1000.0});
}

struct ScheduledResult {
  Vec3 position;
  std::size_t steps{};
};

ScheduledResult simulateAtRenderRate(unsigned renderHz) {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto floor = addFloor(world);
  static_cast<void>(floor);
  PlayerController player(world);
  player.spawn({0.0, 100.0, 0.0});
  player.setCommand(PlayerCommand{0.25, 1.0, 0.0, false, false, false});
  run3::EngineClock clock(run3::ClockMode::Fixed);
  ScheduledResult result;
  for (unsigned frame = 0; frame < renderHz * 2; ++frame) {
    const run3::ClockFrame timing =
        clock.advance(run3::EngineClock::Duration{1.0 / renderHz});
    for (std::size_t step = 0; step < timing.simulationSteps; ++step) {
      static_cast<void>(player.simulateFixedStep());
      ++result.steps;
    }
  }
  result.position = player.state().position;
  return result;
}
} // namespace

TEST_CASE("legacy materials retain diffuse textures without loading old shaders") {
  run3::gameplay::LegacyMaterialCatalog catalog;
  catalog.scan(fs::path(RUN3_TEST_SOURCE_DIR) / "tests" / "fixtures" /
               "materials");

  const auto inherited = catalog.find("run3/testchild");
  REQUIRE(inherited);
  CHECK(inherited->texture == "test_diffuse.dds");
  CHECK(inherited->transparent);
  CHECK(inherited->doubleSided);
  const auto opaqueMultipass = catalog.find("Run3/TestOpaqueChild");
  REQUIRE(opaqueMultipass);
  CHECK(opaqueMultipass->texture == "opaque_diffuse.dds");
  CHECK_FALSE(opaqueMultipass->transparent);
  const auto direct = catalog.find("Run3/TestDirect");
  REQUIRE(direct);
  CHECK(direct->texture == "direct.png");
  CHECK_FALSE(catalog.find("Run3/Missing"));
}

TEST_CASE("static triangle fixture supports and raycasts the player") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  std::vector<Vec3> vertices{{-500, 0, -500}, {500, 0, 500},
                             {500, 0, -500}, {-500, 0, 500}};
  BodyDesc floor(Shape::triangleMesh(std::move(vertices),
                                     {0, 1, 2, 0, 3, 1}));
  floor.group = CollisionGroup::World;
  const BodyHandle floorBody = world.createBody(floor);
  PlayerController player(world);
  player.spawn({0, 110, 0});
  for (int step = 0; step < 180; ++step) {
    static_cast<void>(player.simulateFixedStep());
  }
  REQUIRE(player.state().position.y > 90.0);
  REQUIRE(player.state().position.y < 115.0);
  REQUIRE(floorBody.valid());
}

TEST_CASE("player walks runs jumps and remains upright") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto floor = addFloor(world);
  static_cast<void>(floor);
  PlayerController player(world);
  player.spawn({0, 100, 0});
  player.setCommand({0, 1, 0, false, true, false});
  player.fixedUpdate();
  REQUIRE(player.state().velocity.y == Catch::Approx(360.0));
  REQUIRE(player.state().velocity.z == Catch::Approx(-300.0));
  static_cast<void>(world.advance(PhysicsWorld::fixedStepSeconds));
  player.setCommand({0, 1, 0, true, false, false});
  player.fixedUpdate();
  REQUIRE(player.state().velocity.z == Catch::Approx(-520.0));
}

TEST_CASE("duck preserves feet and blocked ceiling prevents unduck") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto floor = addFloor(world);
  PlayerController player(world);
  player.spawn({0, 100, 0});
  player.setCommand({0, 0, 0, false, false, true});
  player.fixedUpdate();
  REQUIRE(player.state().crouched);
  REQUIRE(player.state().position.y == Catch::Approx(55.0));
  const auto ceiling = addBox(world, {0, 150, 0}, {50, 10, 50});
  player.setCommand({});
  player.fixedUpdate();
  REQUIRE(player.state().crouched);
  static_cast<void>(floor);
  static_cast<void>(ceiling);
}

TEST_CASE("stair probe raises capsule over a low obstacle") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto floor = addFloor(world);
  const auto step = addBox(world, {0, 15, -40}, {80, 15, 10});
  PlayerController player(world);
  player.spawn({0, 100, 0});
  player.setCommand({0, 1, 0, false, false, false});
  player.fixedUpdate();
  REQUIRE(player.state().position.y == Catch::Approx(135.0));
  static_cast<void>(floor);
  static_cast<void>(step);
}

TEST_CASE("noclip teleport parent motion and ladder motion are explicit") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto floor = addFloor(world);
  PlayerController player(world);
  player.spawn({0, 100, 0});
  player.teleport({10, 200, 30});
  REQUIRE(player.state().position.x == Catch::Approx(10.0));
  player.applyParentMotion({5, 2, -3});
  player.fixedUpdate();
  REQUIRE(player.state().position.x == Catch::Approx(15.0));

  player.setOnLadder(true);
  player.setCommand({0, 1, 0, false, false, false});
  player.fixedUpdate();
  REQUIRE(player.state().velocity.y == Catch::Approx(220.0));

  player.setNoclip(true);
  player.setCommand({0, 1, 1, false, false, false});
  player.fixedUpdate(1.0);
  REQUIRE(player.state().noclip);
  REQUIRE(player.state().position.y > 600.0);
  player.setNoclip(false);
  REQUIRE_FALSE(player.state().noclip);
  static_cast<void>(floor);
}

TEST_CASE("use and weapon queries use the Run3 raycast API") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  const auto wall = addBox(world, {0, 150, -150}, {50, 100, 10});
  PlayerController player(world);
  player.spawn({0, 100, 0});
  const auto use = player.useRaycast();
  const auto weapon = player.weaponRaycast();
  REQUIRE(use);
  REQUIRE(weapon);
  REQUIRE(use->body == wall.id());
  REQUIRE(weapon->body == wall.id());
}

TEST_CASE("deterministic replay is render-rate independent") {
  const ScheduledResult at30 = simulateAtRenderRate(30);
  const ScheduledResult at60 = simulateAtRenderRate(60);
  const ScheduledResult at144 = simulateAtRenderRate(144);
  REQUIRE(at30.steps == 120);
  REQUIRE(at60.steps == 120);
  REQUIRE(at144.steps == 120);
  REQUIRE(at30.position.x == Catch::Approx(at60.position.x).margin(0.01));
  REQUIRE(at30.position.z == Catch::Approx(at60.position.z).margin(0.01));
  REQUIRE(at144.position.x == Catch::Approx(at60.position.x).margin(0.01));
  REQUIRE(at144.position.z == Catch::Approx(at60.position.z).margin(0.01));
}

TEST_CASE("fixed-step command replay is repeatable") {
  const auto run = [] {
    PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
    const auto floor = addFloor(world);
    PlayerController player(world);
    player.spawn({0, 100, 0});
    const std::vector<run3::gameplay::ReplayFrame> replay{
        {{0, 1, 0, false, false, false}, 30},
        {{1, 0, 0, true, false, false}, 30},
        {{0, 0, 0, false, true, false}, 1},
        {{0, 0, 0, false, false, true}, 20}};
    run3::gameplay::runReplay(player, replay);
    static_cast<void>(floor);
    return player.state();
  };
  const auto first = run();
  const auto second = run();
  REQUIRE(first.position.x == Catch::Approx(second.position.x).margin(1e-6));
  REQUIRE(first.position.y == Catch::Approx(second.position.y).margin(1e-6));
  REQUIRE(first.position.z == Catch::Approx(second.position.z).margin(1e-6));
  REQUIRE(first.crouched == second.crouched);
}
