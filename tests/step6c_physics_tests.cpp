#include <run3/air3/AirPathFind.hpp>
#include <run3/gameplay/DynamicPhysicsScene.hpp>
#include <run3/physics/PhysicsQuery.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {

using Catch::Matchers::WithinAbs;
using run3::gameplay::DynamicEntityDesc;
using run3::gameplay::DynamicPhysicsScene;
using run3::gameplay::GameplayPhysicsEventType;
using run3::physics::BodyDesc;
using run3::physics::BodyMotion;
using run3::physics::BodyType;
using run3::physics::CollisionGroup;
using run3::physics::PhysicsConfig;
using run3::physics::PhysicsWorld;
using run3::physics::Shape;
using run3::physics::Transform;
using run3::physics::Vec3;

constexpr double step = PhysicsWorld::fixedStepSeconds;

BodyDesc playerBody(Vec3 position = {}) {
  BodyDesc description{Shape::capsule(30.0, 120.0)};
  description.motion = BodyMotion::Kinematic;
  description.transform.position = position;
  description.group = CollisionGroup::Player;
  description.metadata = {900, BodyType::Player, 0};
  return description;
}

bool hasEvent(const std::vector<run3::gameplay::GameplayPhysicsEvent> &events,
              GameplayPhysicsEventType type) {
  return std::any_of(events.begin(), events.end(),
                     [type](const auto &event) { return event.type == type; });
}

class BlockingQuery final : public run3::physics::IPhysicsQuery {
public:
  std::vector<run3::physics::RaycastHit>
  raycastAll(const run3::physics::RaycastQuery &query) const override {
    ++calls;
    // The direct route at z=0 is blocked. Routes through the z=100 waypoint
    // are clear, demonstrating that AIR3 consumes only the injected seam.
    if (query.from.z == 0.0 && query.to.z == 0.0 &&
        query.from.x != query.to.x) {
      return {{1, {1, BodyType::World, 0}, {}, {}, 0.5, false}};
    }
    return {};
  }
  mutable int calls{};
};

std::size_t countTags(const std::filesystem::path &file,
                      const std::string &tag) {
  std::ifstream stream(file);
  if (!stream) {
    return 0;
  }
  std::ostringstream contents;
  contents << stream.rdbuf();
  const std::string text = contents.str();
  const std::regex expression("<\\s*" + tag + "\\b",
                              std::regex::icase);
  return static_cast<std::size_t>(
      std::distance(std::sregex_iterator(text.begin(), text.end(), expression),
                    std::sregex_iterator{}));
}

} // namespace

TEST_CASE("AIR3 path raycasts through the injected Run3 query interface") {
  BlockingQuery query;
  run3::air3::AirPathFind pathfinder(query);
  pathfinder.setNodes({{10, {50.0, 0.0, 100.0}}});
  const auto path =
      pathfinder.search({1, {0.0, 0.0, 0.0}}, {2, {100.0, 0.0, 0.0}});
  REQUIRE(path.size() == 3);
  CHECK(path[1].id == 10);
  CHECK(query.calls > 1);
}

TEST_CASE("pickup button and trigger callbacks become queued typed events") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  DynamicPhysicsScene scene(world);
  auto player = world.createBody(playerBody());

  DynamicEntityDesc pickup{"health", BodyType::Pickup,
                           Shape::box({20.0, 20.0, 20.0})};
  pickup.motion = BodyMotion::Static;
  pickup.massKg = 0.0;
  pickup.group = CollisionGroup::Pickup;
  pickup.trigger = true;
  const auto pickupId = scene.createEntity(std::move(pickup));

  DynamicEntityDesc button{"switch", BodyType::Button,
                           Shape::box({20.0, 20.0, 20.0})};
  button.motion = BodyMotion::Static;
  button.massKg = 0.0;
  button.group = CollisionGroup::Button;
  button.trigger = true;
  static_cast<void>(scene.createEntity(std::move(button)));

  DynamicEntityDesc trigger{"zone", BodyType::Trigger,
                            Shape::box({40.0, 40.0, 40.0})};
  trigger.motion = BodyMotion::Static;
  trigger.massKg = 0.0;
  trigger.group = CollisionGroup::Trigger;
  trigger.trigger = true;
  static_cast<void>(scene.createEntity(std::move(trigger)));

  static_cast<void>(world.advance(step));
  const auto events = scene.processContactEvents();
  CHECK(hasEvent(events, GameplayPhysicsEventType::PickupCollected));
  CHECK(hasEvent(events, GameplayPhysicsEventType::ButtonPressed));
  CHECK(hasEvent(events, GameplayPhysicsEventType::TriggerEntered));
  CHECK_FALSE(scene.enabled(pickupId));

  world.setTransform(player, Transform{{500.0, 0.0, 0.0}, {}});
  static_cast<void>(world.advance(step));
  CHECK(hasEvent(scene.processContactEvents(),
                 GameplayPhysicsEventType::TriggerExited));
}

TEST_CASE("projectile contacts damage breakables and NPC bodies by typed id") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  DynamicPhysicsScene scene(world);

  DynamicEntityDesc crate{"crate", BodyType::Breakable,
                          Shape::box({30.0, 30.0, 30.0})};
  crate.health = 20.0;
  const auto crateId = scene.createEntity(std::move(crate));
  DynamicEntityDesc projectile{"round", BodyType::Projectile,
                               Shape::box({5.0, 5.0, 5.0})};
  projectile.group = CollisionGroup::Projectile;
  const auto projectileId = scene.createEntity(std::move(projectile));

  static_cast<void>(world.advance(step));
  const auto events = scene.processContactEvents(25.0);
  CHECK(hasEvent(events, GameplayPhysicsEventType::ProjectileImpact));
  CHECK(hasEvent(events, GameplayPhysicsEventType::Damage));
  CHECK(hasEvent(events, GameplayPhysicsEventType::Broken));
  CHECK(scene.health(crateId) <= 0.0);
  CHECK_FALSE(scene.enabled(crateId));
  CHECK_FALSE(scene.enabled(projectileId));
}

TEST_CASE("doors and trains use deterministic kinematic transforms") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  DynamicPhysicsScene scene(world);
  DynamicEntityDesc door{"door", BodyType::Door,
                         Shape::box({50.0, 100.0, 10.0})};
  const auto doorId =
      scene.createDoor(std::move(door), Transform{{100.0, 0.0, 0.0}, {}}, 50.0);
  scene.setDoorOpen(doorId, true);
  scene.update(1.0);
  CHECK_THAT(scene.transform(doorId).position.x, WithinAbs(50.0, 1e-9));
  scene.update(1.0);
  CHECK_THAT(scene.transform(doorId).position.x, WithinAbs(100.0, 1e-9));

  DynamicEntityDesc train{"train", BodyType::Train,
                          Shape::box({200.0, 30.0, 80.0})};
  const auto trainId = scene.createTrain(std::move(train));
  scene.setTrainTransform(trainId, Transform{{400.0, 20.0, -10.0}, {}});
  CHECK(scene.transform(trainId).position == Vec3{400.0, 20.0, -10.0});
}

TEST_CASE("NPC contact is preserved without pointer or string casts") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  DynamicPhysicsScene scene(world);
  auto player = world.createBody(playerBody());
  DynamicEntityDesc npc{"guard", BodyType::Npc,
                       Shape::capsule(30.0, 120.0)};
  npc.group = CollisionGroup::Npc;
  const auto npcId = scene.createEntity(std::move(npc));
  static_cast<void>(world.advance(step));
  const auto events = scene.processContactEvents();
  REQUIRE(hasEvent(events, GameplayPhysicsEventType::NpcContact));
  const auto found = std::find_if(events.begin(), events.end(), [](const auto &e) {
    return e.type == GameplayPhysicsEventType::NpcContact;
  });
  CHECK(found->entity == npcId);
  CHECK(found->other == 900);
  CHECK(player.valid());
}

TEST_CASE("ragdoll constraints and map unload have explicit RAII lifetimes") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  {
    DynamicPhysicsScene scene(world);
    const auto ragdoll = scene.createRagdoll({"npc-ragdoll", {}, 70.0, 0.5});
    CHECK(scene.bodyCount() == 3);
    CHECK(scene.constraintCount() == 2);
    CHECK(world.bodyCount() == 3);
    CHECK(world.constraintCount() == 2);
    CHECK(scene.transform(ragdoll, 1).position.y == 58.0);
    scene.update(0.5);
    CHECK(scene.entityCount() == 0);
    CHECK(world.bodyCount() == 0);
    CHECK(world.constraintCount() == 0);

    static_cast<void>(scene.createRagdoll({"reload", {}, 60.0, 10.0}));
    scene.unload();
    CHECK(world.bodyCount() == 0);
    CHECK(world.constraintCount() == 0);
  }
  CHECK(world.bodyCount() == 0);
}

TEST_CASE("hinge limits are owned by the world and release with either body") {
  PhysicsConfig config;
  config.gravity = {};
  PhysicsWorld world = run3::physics::createBulletPhysicsWorld(config);
  BodyDesc first{Shape::box({10.0, 10.0, 10.0})};
  first.motion = BodyMotion::Dynamic;
  first.massKg = 1.0;
  auto firstBody = world.createBody(first);
  auto secondBody = world.createBody(first);
  auto hinge = world.createHingeConstraint(
      firstBody, secondBody, {}, {}, {0.0, 2.0, 0.0}, {0.0, 1.0, 0.0},
      -0.5, 0.5);
  CHECK(hinge.valid());
  CHECK(world.constraintCount() == 1);
  secondBody.reset();
  CHECK_FALSE(hinge.valid());
  CHECK(world.constraintCount() == 0);
}

TEST_CASE("The Long Way representative maps contain the migrated slices") {
#ifdef RUN3_TEST_SOURCE_DIR
  const std::filesystem::path maps =
      std::filesystem::path(RUN3_TEST_SOURCE_DIR) / "Games" / "The Long Way" /
      "TheLongWay" / "run3" / "maps" / "high";
  const auto caoMain = maps / "tlwcao" / "tlwcao.xml";
  const auto caoSequence = maps / "tlwcao" / "tlwcaos.xml";
  const auto homeMain = maps / "tlwhome02" / "tlwhome2m2.xml";
  const auto homeSequence = maps / "tlwhome02" / "tlwhome2s.xml";
  if (!std::filesystem::is_regular_file(caoMain) ||
      !std::filesystem::is_regular_file(homeMain)) {
    SKIP("The Long Way local content is not attached");
  }
  CHECK(countTags(caoMain, "phys") >= 10);
  CHECK(countTags(homeMain, "phys") >= 10);
  CHECK(countTags(homeMain, "breakable") >= 8);
  CHECK(countTags(caoSequence, "door") >= 20);
  CHECK(countTags(caoSequence, "button") >= 15);
  CHECK(countTags(caoSequence, "trigger") >= 25);
  CHECK(countTags(caoSequence, "train") >= 1);
  CHECK(countTags(caoSequence, "npc") >= 15);
  CHECK(countTags(homeSequence, "door") >= 40);
  CHECK(countTags(homeSequence, "trigger") >= 35);
  CHECK(countTags(homeSequence, "train") >= 10);
  CHECK(countTags(homeSequence, "npc") >= 20);
#else
  SKIP("source path was not supplied by the build");
#endif
}
