#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/content/XmlParser.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/gameplay/MapRuntimeAdapter.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};
const fs::path fixtureContent = sourceRoot / "tests/fixtures/step8b/content";

run3::AppPaths fixturePaths() {
  return run3::AppPaths::resolve(sourceRoot / "build/step8b-fixture.exe",
                                 fixtureContent,
                                 sourceRoot / "build/step8b-user");
}

std::map<std::string, std::size_t>
counts(const std::vector<const run3::content::AuthoredElement *> &elements) {
  std::map<std::string, std::size_t> result;
  for (const run3::content::AuthoredElement *element : elements) {
    ++result[element->tag];
  }
  return result;
}

} // namespace

TEST_CASE("Step 8B definitions preserve config, source order, and sequences",
          "[step8b][entities][xml]") {
  const run3::content::MapDefinition definition =
      run3::content::loadMapDefinition(fixturePaths(), "fixture", "low");

  CHECK(definition.mapName == "fixture");
  CHECK(definition.scene.tag == "scene");
  REQUIRE(definition.scene.attribute("multiplier") != nullptr);
  CHECK(*definition.scene.attribute("multiplier") == "2");
  REQUIRE(definition.externalSequenceFile.has_value());
  CHECK(definition.externalSequenceFile->filename() == "fixture-sequence.xml");
  REQUIRE(definition.sequences.size() == 2);
  CHECK(definition.sequences[0].origin ==
        run3::content::SequenceOrigin::Integrated);
  CHECK(definition.sequences[1].origin ==
        run3::content::SequenceOrigin::External);
  REQUIRE(definition.sequences[0].declarations.size() == 1);
  CHECK(definition.sequences[0].declarations[0].tag == "timer");
  CHECK(definition.sequences[0].declarations[0].source.line == 4);
  CHECK(definition.sequences[0].declarations[0].order <
        definition.sequences[1].declarations[0].order);

  const auto declarationCounts =
      counts(run3::content::sequenceDeclarations(definition));
  CHECK(declarationCounts.at("timer") == 1);
  CHECK(declarationCounts.at("door") == 1);
  CHECK(declarationCounts.at("trigger") == 1);
  CHECK(declarationCounts.at("button") == 1);
  CHECK(declarationCounts.at("npc") == 1);
  CHECK(counts(run3::content::sequenceEvents(definition)).at("trigger") == 1);
}

TEST_CASE("Step 8B registry preserves duplicates and invalidates stale handles",
          "[step8b][entities][lifetime]") {
  run3::gameplay::EntityRegistry registry;
  std::vector<int> destructionOrder;
  const run3::content::SourceLocation location{"duplicate.xml", 7, 1};
  const auto first = registry.create(
      {"same", "same", "door", run3::gameplay::EntityOwner::Sequence,
       location, 1},
      [&destructionOrder] { destructionOrder.push_back(1); });
  const auto second = registry.create(
      {"same", "same", "door", run3::gameplay::EntityOwner::Sequence,
       location, 2},
      [&destructionOrder] { destructionOrder.push_back(2); });

  REQUIRE(registry.findAll("same").size() == 2);
  CHECK(registry.findFirst("same") == first);
  registry.bindPresentation(first, 101);
  registry.bindPhysics(first, 202);
  CHECK(registry.get(first).presentationKey == 101);
  CHECK(registry.get(first).physicsKey == 202);

  registry.clear();
  CHECK(registry.size() == 0);
  CHECK_FALSE(registry.valid(first));
  CHECK_FALSE(registry.valid(second));
  CHECK(destructionOrder == std::vector<int>{2, 1});

  const auto reused = registry.create(
      {"new", "new", "trigger", run3::gameplay::EntityOwner::Sequence,
       location, 3});
  CHECK(reused.id == first.id);
  CHECK(reused.generation != first.generation);
}

TEST_CASE("Step 8B runtime ignores renderables outside exact scene nodes",
          "[step8b][entities][regression]") {
  run3::content::MapDefinition definition;
  definition.scene.tag = "scene";

  run3::content::AuthoredElement nodes;
  nodes.tag = "nodes";
  run3::content::AuthoredElement disabled;
  disabled.tag = "nodev";
  run3::content::AuthoredElement disabledPhysics;
  disabledPhysics.tag = "phys";
  disabledPhysics.attributes.emplace_back("meshFile", "disabled.mesh");
  disabled.children.push_back(std::move(disabledPhysics));
  run3::content::AuthoredElement disabledNestedNode;
  disabledNestedNode.tag = "node";
  run3::content::AuthoredElement disabledNestedPhysics;
  disabledNestedPhysics.tag = "phys";
  disabledNestedPhysics.attributes.emplace_back("meshFile",
                                                "disabled-nested.mesh");
  disabledNestedNode.children.push_back(std::move(disabledNestedPhysics));
  disabled.children.push_back(std::move(disabledNestedNode));

  run3::content::AuthoredElement active;
  active.tag = "node";
  run3::content::AuthoredElement activeEntity;
  activeEntity.tag = "entity";
  activeEntity.attributes.emplace_back("meshFile", "active.mesh");
  active.children.push_back(std::move(activeEntity));

  nodes.children.push_back(std::move(disabled));
  nodes.children.push_back(std::move(active));
  definition.scene.children.push_back(std::move(nodes));

  const auto renderables = run3::gameplay::activeMapRenderables(definition);
  REQUIRE(renderables.size() == 1);
  CHECK(renderables[0]->tag == "entity");
  REQUIRE(renderables[0]->attribute("meshFile") != nullptr);
  CHECK(*renderables[0]->attribute("meshFile") == "active.mesh");
}

TEST_CASE("Step 8B deferred references resolve after construction",
          "[step8b][entities][references]") {
  const run3::content::MapDefinition valid =
      run3::content::loadMapDefinition(fixturePaths(), "fixture", "low");
  run3::gameplay::EntityRegistry validRegistry;
  const auto population =
      run3::gameplay::populateEntityRegistry(valid, validRegistry, true);
  CHECK(population.sequenceDeclarations == 5);
  CHECK(population.eventBindings == 1);
  REQUIRE(validRegistry.references().size() == 1);
  CHECK(validRegistry.references()[0].target.has_value());
  CHECK(validRegistry.get(*validRegistry.references()[0].target)
            .descriptor.tag == "trigger");

  const run3::content::MapDefinition unresolved =
      run3::content::loadMapDefinition(fixturePaths(), "unresolved", "low");
  run3::gameplay::EntityRegistry unresolvedRegistry;
  CHECK_THROWS_WITH(
      run3::gameplay::populateEntityRegistry(unresolved, unresolvedRegistry,
                                             true),
      Catch::Matchers::ContainsSubstring("missing required target 'missing'"));
}

TEST_CASE("Step 8B reports malformed, case-mismatched, and unknown content",
          "[step8b][entities][diagnostics]") {
  CHECK_THROWS_AS(
      run3::content::loadMapDefinition(fixturePaths(), "malformed", "low"),
      run3::content::XmlParseError);
  CHECK_THROWS_WITH(
      run3::content::loadMapDefinition(fixturePaths(), "badcase", "low"),
      Catch::Matchers::ContainsSubstring("case mismatch"));

  const auto preserved =
      run3::content::loadMapDefinition(fixturePaths(), "unknown", "low");
  REQUIRE(preserved.issues.size() == 1);
  CHECK(preserved.issues[0].kind ==
        run3::content::DefinitionIssueKind::UnknownElement);
  CHECK(preserved.issues[0].context == "mysteryRequired");
  const auto *nodes = preserved.scene.firstChild("nodes");
  REQUIRE(nodes != nullptr);
  REQUIRE(nodes->children.size() == 1);
  REQUIRE(nodes->children[0].attribute("required") != nullptr);
  CHECK(*nodes->children[0].attribute("required") == "true");
  CHECK_THROWS_WITH(run3::content::loadMapDefinition(
                        fixturePaths(), "unknown", "low", {true}),
                    Catch::Matchers::ContainsSubstring(
                        "preserved unknown element <mysteryRequired>"));
}

TEST_CASE("Step 8B attached TLW maps match reviewed declaration counts",
          "[step8b][entities][content]") {
  const fs::path contentRoot =
      sourceRoot / "Games/The Long Way/TheLongWay";
  if (!fs::is_directory(contentRoot)) {
    SKIP("Optional author-provided The Long Way content is not attached");
  }
  const run3::AppPaths paths = run3::AppPaths::resolve(
      sourceRoot / "build/step8b-content.exe", contentRoot,
      sourceRoot / "build/step8b-content-user");
  const std::vector<std::pair<std::string, std::map<std::string, std::size_t>>>
      expected{{"tlwcao",
                {{"button", 21}, {"computer", 4}, {"cutscene", 1},
                 {"darkzone", 1}, {"door", 29}, {"ladder", 2}, {"lua", 1},
                 {"npc", 19}, {"rot", 2}, {"timer", 33}, {"train", 2},
                 {"trigger", 20}}},
               {"tlwhome02",
                {{"button", 11}, {"computer", 5}, {"cutscene", 3},
                 {"darkzone", 2}, {"door", 50}, {"lua", 1}, {"npc", 28},
                 {"rodt", 1}, {"rot", 40}, {"rotd", 1}, {"timer", 3},
                 {"train", 14}, {"traind", 1}, {"trainv", 1},
                 {"trigger", 23}, {"triggerd", 1}, {"triggerv", 1}}}};
  for (const auto &[map, wanted] : expected) {
    CAPTURE(map);
    const auto definition = run3::content::loadMapDefinition(paths, map, "low");
    const auto actual = counts(run3::content::sequenceDeclarations(definition));
    for (const auto &[tag, count] : actual) {
      UNSCOPED_INFO(tag << "=" << count);
    }
    CHECK(actual == wanted);
    const auto eventCounts = counts(run3::content::sequenceEvents(definition));
    CHECK(eventCounts.at("trigger") == (map == "tlwcao" ? 10 : 19));
    CHECK(eventCounts.at("cutscene") == (map == "tlwcao" ? 1 : 2));
  }
}

TEST_CASE("Step 8B inventories every attached low scene.cfg sequence",
          "[step8b][entities][content]") {
  const fs::path contentRoot =
      sourceRoot / "Games/The Long Way/TheLongWay";
  const fs::path mapsRoot = contentRoot / "run3/maps/low";
  if (!fs::is_directory(mapsRoot)) {
    SKIP("Optional author-provided The Long Way content is not attached");
  }
  const run3::AppPaths paths = run3::AppPaths::resolve(
      sourceRoot / "build/step8b-all-content.exe", contentRoot,
      sourceRoot / "build/step8b-all-content-user");
  std::size_t mapCount{};
  for (const fs::directory_entry &entry : fs::directory_iterator(mapsRoot)) {
    if (!entry.is_directory() ||
        !fs::is_regular_file(entry.path() / "scene.cfg")) {
      continue;
    }
    CAPTURE(entry.path().filename().string());
    const auto definition = run3::content::loadMapDefinition(
        paths, entry.path().filename().string(), "low");
    CHECK_FALSE(definition.scene.tag.empty());
    ++mapCount;
  }
  CHECK(mapCount == 18);
}
