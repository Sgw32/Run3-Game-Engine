#include <run3/physics/Physics.hpp>
#include <run3/physics/PhysicsTesting.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

using Catch::Matchers::WithinAbs;
using run3::physics::BodyDesc;
using run3::physics::BodyMotion;
using run3::physics::BodyType;
using run3::physics::CollisionGroup;
using run3::physics::ContactPhase;
using run3::physics::PhysicsConfig;
using run3::physics::PhysicsWorld;
using run3::physics::RaycastQuery;
using run3::physics::Shape;
using run3::physics::Transform;
using run3::physics::Vec3;

constexpr double fixedStep = 1.0 / 60.0;

BodyDesc body(Shape shape, BodyMotion motion, Vec3 position,
              double massKg = 0.0) {
  BodyDesc result{std::move(shape)};
  result.motion = motion;
  result.transform.position = position;
  result.massKg = massKg;
  return result;
}

void advance(PhysicsWorld &world, double seconds) {
  const int steps = static_cast<int>(seconds / fixedStep + 0.5);
  for (int index = 0; index < steps; ++index) {
    static_cast<void>(world.advance(fixedStep));
  }
}

} // namespace

TEST_CASE("legacy game units cross one centralized metre boundary") {
  const auto standard = run3::physics::UnitConversion::standard();
  CHECK_THAT(standard.toMetres(100.0), WithinAbs(1.0, 1e-12));
  CHECK_THAT(standard.toGameUnits(1.0), WithinAbs(100.0, 1e-12));
  CHECK(standard.toMetres(Vec3{100.0, -50.0, 25.0}) ==
        Vec3{1.0, -0.5, 0.25});

  const auto testScale =
      run3::physics::testing::PhysicsTestAccess::unitConversion(0.5);
  CHECK_THAT(testScale.toMetres(4.0), WithinAbs(2.0, 1e-12));
  CHECK_THAT(testScale.toGameUnits(2.0), WithinAbs(4.0, 1e-12));
  PhysicsWorld testWorld =
      run3::physics::testing::PhysicsTestAccess::createNullWorld(
          PhysicsConfig{}, testScale);
  CHECK_THAT(testWorld.units().metresPerGameUnit(), WithinAbs(0.5, 1e-12));
}

TEST_CASE("Bullet and null backends enforce the same invalid input contract") {
  PhysicsConfig invalidConfig;
  invalidConfig.maxCatchUpSteps = 0;
  CHECK_THROWS_AS(run3::physics::createBulletPhysicsWorld(invalidConfig),
                  std::invalid_argument);
  CHECK_THROWS_AS(run3::physics::createNullPhysicsWorld(invalidConfig),
                  std::invalid_argument);

  auto rejectInvalidBody = [](PhysicsWorld world) {
    BodyDesc invalid = body(Shape::box({1.0, 1.0, 1.0}), BodyMotion::Static,
                            {}, 1.0);
    CHECK_THROWS_AS(world.createBody(invalid), std::invalid_argument);
  };
  rejectInvalidBody(run3::physics::createBulletPhysicsWorld());
  rejectInvalidBody(run3::physics::createNullPhysicsWorld());
}

TEST_CASE("fixed stepping is render-rate independent and bounds catch-up") {
  auto simulate = [](int renderHz) {
    PhysicsWorld world = run3::physics::createNullPhysicsWorld();
    std::uint64_t steps{};
    for (int frame = 0; frame < renderHz; ++frame) {
      steps += world.advance(1.0 / renderHz).steps;
    }
    return steps;
  };
  CHECK(simulate(30) == 60);
  CHECK(simulate(60) == 60);
  CHECK(simulate(144) == 60);

  PhysicsConfig config;
  config.maxCatchUpSteps = 4;
  PhysicsWorld bounded = run3::physics::createNullPhysicsWorld(config);
  const auto result = bounded.advance(1.0);
  CHECK(result.steps == 4);
  CHECK(result.droppedSeconds > 0.9);
  CHECK(result.interpolationAlpha >= 0.0);
  CHECK(result.interpolationAlpha < 1.0);
}

TEST_CASE("Bullet gravity moves a dynamic body in game units") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  auto falling = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic,
           {0.0, 500.0, 0.0}, 1.0));
  advance(world, 0.5);
  CHECK(world.transform(falling).position.y < 400.0);
  CHECK(world.linearVelocity(falling).y < -400.0);
}

TEST_CASE("a falling box settles on a static box floor") {
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld();
  auto floor = world.createBody(
      body(Shape::box({500.0, 50.0, 500.0}), BodyMotion::Static,
           {0.0, -50.0, 0.0}));
  auto falling = world.createBody(
      body(Shape::box({50.0, 50.0, 50.0}), BodyMotion::Dynamic,
           {0.0, 300.0, 0.0}, 10.0));
  advance(world, 4.0);
  CHECK(floor.valid());
  CHECK_THAT(world.transform(falling).position.y, WithinAbs(50.0, 2.0));
  CHECK_THAT(world.linearVelocity(falling).y, WithinAbs(0.0, 2.0));
}

TEST_CASE("box capsule and indexed mesh shapes are readable by Bullet") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);

  auto boxHandle = world.createBody(
      body(Shape::box({20.0, 20.0, 20.0}), BodyMotion::Static,
           {100.0, 0.0, 0.0}));
  auto capsuleHandle = world.createBody(
      body(Shape::capsule(20.0, 60.0), BodyMotion::Static,
           {250.0, 0.0, 0.0}));
  const std::vector<Vec3> vertices{{-50.0, 0.0, -50.0},
                                   {50.0, 0.0, -50.0},
                                   {50.0, 0.0, 50.0},
                                   {-50.0, 0.0, 50.0}};
  const std::vector<std::uint32_t> indices{0, 1, 2, 0, 2, 3};
  auto meshHandle = world.createBody(
      body(Shape::triangleMesh(vertices, indices), BodyMotion::Static,
           {400.0, 0.0, 0.0}));

  CHECK(boxHandle.valid());
  CHECK(capsuleHandle.valid());
  CHECK(meshHandle.valid());
  CHECK(world.bodyCount() == 3);
  CHECK(world.raycastClosest({{400.0, 100.0, 0.0},
                              {400.0, -100.0, 0.0}})
            .has_value());
}

TEST_CASE("raycast results are nearest-first and honor groups and masks") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);

  BodyDesc nearDesc = body(Shape::box({20.0, 20.0, 20.0}),
                           BodyMotion::Static, {100.0, 0.0, 0.0});
  nearDesc.group = CollisionGroup::World;
  nearDesc.metadata = {11, BodyType::PhysicalObject, 101};
  auto nearBody = world.createBody(nearDesc);

  BodyDesc farDesc = body(Shape::box({20.0, 20.0, 20.0}),
                          BodyMotion::Static, {300.0, 0.0, 0.0});
  farDesc.group = CollisionGroup::Player;
  farDesc.metadata = {22, BodyType::Player, 202};
  auto farBody = world.createBody(farDesc);

  RaycastQuery all{{0.0, 0.0, 0.0}, {500.0, 0.0, 0.0}};
  const auto hits = world.raycastAll(all);
  REQUIRE(hits.size() == 2);
  CHECK(hits[0].fraction < hits[1].fraction);
  CHECK(hits[0].body == nearBody.id());
  CHECK(hits[1].body == farBody.id());
  CHECK(hits[0].metadata.entityId == 11);

  RaycastQuery players = all;
  players.mask = run3::physics::collisionMask(CollisionGroup::Player);
  const auto playerHit = world.raycastClosest(players);
  REQUIRE(playerHit.has_value());
  CHECK(playerHit->body == farBody.id());
}

TEST_CASE("trigger contacts are copied into a safe begin persist end queue") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);

  BodyDesc triggerDesc = body(Shape::box({100.0, 100.0, 100.0}),
                              BodyMotion::Static, {});
  triggerDesc.trigger = true;
  triggerDesc.group = CollisionGroup::Trigger;
  triggerDesc.mask = run3::physics::collisionMask(CollisionGroup::Player);
  triggerDesc.metadata = {7, BodyType::Trigger, 70};
  auto trigger = world.createBody(triggerDesc);

  BodyDesc actorDesc = body(Shape::box({20.0, 20.0, 20.0}),
                            BodyMotion::Dynamic, {}, 1.0);
  actorDesc.group = CollisionGroup::Player;
  actorDesc.mask = run3::physics::collisionMask(CollisionGroup::Trigger);
  actorDesc.metadata = {8, BodyType::Player, 80};
  auto actor = world.createBody(actorDesc);

  static_cast<void>(world.advance(fixedStep));
  auto events = world.drainContactEvents();
  REQUIRE_FALSE(events.empty());
  CHECK(events.front().phase == ContactPhase::Began);
  CHECK(events.front().trigger);
  CHECK((events.front().first == trigger.id() ||
         events.front().second == trigger.id()));

  static_cast<void>(world.advance(fixedStep));
  events = world.drainContactEvents();
  REQUIRE_FALSE(events.empty());
  CHECK(events.front().phase == ContactPhase::Persisted);

  world.setTransform(actor, Transform{{500.0, 0.0, 0.0}, {}});
  static_cast<void>(world.advance(fixedStep));
  events = world.drainContactEvents();
  REQUIRE(std::any_of(events.begin(), events.end(), [](const auto &event) {
    return event.phase == ContactPhase::Ended;
  }));
}

TEST_CASE("center force and impulse use game-unit velocities") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  auto bodyHandle = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic, {}, 2.0));

  world.applyCentralForce(bodyHandle, {1200.0, 0.0, 0.0});
  static_cast<void>(world.advance(fixedStep));
  CHECK_THAT(world.linearVelocity(bodyHandle).x, WithinAbs(10.0, 0.2));

  world.applyCentralImpulse(bodyHandle, {200.0, 0.0, 0.0});
  CHECK_THAT(world.linearVelocity(bodyHandle).x, WithinAbs(110.0, 0.2));
}

TEST_CASE("sleeping bodies wake on impulse and can be disabled explicitly") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  auto bodyHandle = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic, {}, 1.0));

  advance(world, 3.0);
  CHECK(world.isSleeping(bodyHandle));
  world.applyCentralImpulse(bodyHandle, {100.0, 0.0, 0.0});
  CHECK_FALSE(world.isSleeping(bodyHandle));

  world.setEnabled(bodyHandle, false);
  CHECK_FALSE(world.isEnabled(bodyHandle));
  world.setEnabled(bodyHandle, true);
  CHECK(world.isEnabled(bodyHandle));
}

TEST_CASE("body and constraint handles release backend objects with RAII") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  auto first = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic, {}, 1.0));
  auto second = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic,
           {100.0, 0.0, 0.0}, 1.0));
  auto constraint = world.createPointConstraint(first, second, {}, {});
  CHECK(world.bodyCount() == 2);
  CHECK(world.constraintCount() == 1);

  constraint.reset();
  CHECK(world.constraintCount() == 0);
  constraint = world.createPointConstraint(first, second, {}, {});
  first.reset();
  CHECK(world.bodyCount() == 1);
  CHECK(world.constraintCount() == 0);
  CHECK_FALSE(constraint.valid());

  PhysicsWorld temporary = run3::physics::createBulletPhysicsWorld(config);
  auto survivor = temporary.createBody(
      body(Shape::box({1.0, 1.0, 1.0}), BodyMotion::Dynamic, {}, 1.0));
  temporary = {};
  CHECK_FALSE(survivor.valid());
  survivor.reset();
}

TEST_CASE("current and previous transforms are interpolation ready") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  BodyDesc movingDesc = body(Shape::box({1.0, 1.0, 1.0}),
                             BodyMotion::Dynamic, {}, 1.0);
  movingDesc.linearVelocity = {60.0, 0.0, 0.0};
  auto moving = world.createBody(movingDesc);

  static_cast<void>(world.advance(fixedStep));
  static_cast<void>(world.advance(fixedStep / 2.0));
  CHECK_THAT(world.transform(moving).position.x, WithinAbs(1.0, 0.05));
  CHECK_THAT(world.interpolatedTransform(moving).position.x,
             WithinAbs(0.5, 0.05));
}

TEST_CASE("null backend retains handles without simulating collisions") {
  PhysicsWorld world = run3::physics::createNullPhysicsWorld();
  auto bodyHandle = world.createBody(
      body(Shape::box({10.0, 10.0, 10.0}), BodyMotion::Dynamic,
           {1.0, 2.0, 3.0}, 1.0));
  static_cast<void>(world.advance(1.0));
  CHECK(world.transform(bodyHandle).position == Vec3{1.0, 2.0, 3.0});
  CHECK(world.raycastAll({{}, {100.0, 0.0, 0.0}}).empty());
  CHECK(world.drainContactEvents().empty());
}
