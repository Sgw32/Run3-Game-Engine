#include <run3/content/XmlParser.hpp>
#include <run3/scripting/ScriptEngine.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};
const fs::path fixtureRoot = sourceRoot / "tests/fixtures/step8";
const fs::path goldenRoot = sourceRoot / "tests/golden";

std::string read(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

run3::scripting::ScriptEngine makeEngine(std::size_t budget = 1'000'000) {
  return run3::scripting::ScriptEngine(
      {fixtureRoot / "scripts", fixtureRoot / "user", budget});
}

} // namespace

TEST_CASE("Step 8 XML schemas match pre-migration golden semantics",
          "[step8][xml]") {
  using run3::content::XmlSchema;
  const std::vector<std::pair<std::string, XmlSchema>> cases{
      {"representative.scene", XmlSchema::scene},
      {"representative-sequence.xml", XmlSchema::sequence},
      {"representative-save.xml", XmlSchema::save},
      {"representative-facial.xml", XmlSchema::facialAnimation},
      {"representative-config.xml", XmlSchema::configAdjacent},
  };
  for (const auto &[name, schema] : cases) {
    CAPTURE(name);
    const auto parsed =
        run3::content::parseXmlFile(fixtureRoot / "xml" / name, schema);
    CHECK(run3::content::canonicalXmlSnapshot(parsed) ==
          read(goldenRoot / "xml" / (name + ".golden")));
  }
}

TEST_CASE("Step 8 XML compatibility normalizes only legacy attribute syntax",
          "[step8][xml]") {
  const auto parsed = run3::content::parseXmlFile(
      fixtureRoot / "xml/representative-sequence.xml",
      run3::content::XmlSchema::sequence);
  REQUIRE(parsed.legacyAttributeNormalization);
  const auto *adents = parsed.root.firstChild("adents");
  REQUIRE(adents != nullptr);
  REQUIRE(adents->children.size() == 3);
  REQUIRE(adents->children[1].attribute("period") != nullptr);
  CHECK(*adents->children[1].attribute("period") == "28");
}

TEST_CASE("Malformed XML has source, schema, line, and cause", "[step8][xml]") {
  try {
    (void)run3::content::parseXmlFile(
        fixtureRoot / "xml/malformed.xml",
        run3::content::XmlSchema::configAdjacent);
    FAIL("malformed XML unexpectedly parsed");
  } catch (const run3::content::XmlParseError &error) {
    CHECK(error.source().filename() == "malformed.xml");
    CHECK(error.schema() == run3::content::XmlSchema::configAdjacent);
    CHECK(error.line() == 3);
    CHECK(error.column() == 1);
    CHECK_FALSE(error.cause().empty());
    CHECK(std::string(error.what()).find("malformed.xml:3:1") !=
          std::string::npos);
    std::string folded = error.cause();
    std::transform(folded.begin(), folded.end(), folded.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::tolower(character));
                   });
    const std::string snapshot =
        "schema=config-adjacent\nline=" + std::to_string(error.line()) +
        "\ncause-contains=" +
        (folded.find("mismatch") != std::string::npos ? "mismatched" :
                                                        "other") +
        "\n";
    CHECK(snapshot == read(goldenRoot / "xml/malformed.xml.golden"));
  }
}

TEST_CASE("Attached representative The Long Way XML schemas parse",
          "[step8][xml][content]") {
  using run3::content::XmlSchema;
  const fs::path contentRoot =
      sourceRoot / "Games/The Long Way/TheLongWay/run3";
  if (!fs::is_directory(contentRoot)) {
    SKIP("Optional author-provided The Long Way content is not attached");
  }
  const std::vector<std::pair<fs::path, XmlSchema>> cases{
      {"maps/low/tlwcao/tlwcao.xml", XmlSchema::scene},
      {"maps/low/tlwcao/tlwcaos.xml", XmlSchema::sequence},
      {"game/savefile.xml", XmlSchema::save},
      {"sounds/pogran/document01_facial.xml", XmlSchema::facialAnimation},
      {"core/start.xml", XmlSchema::configAdjacent},
  };
  for (const auto &[relative, schema] : cases) {
    CAPTURE(relative.generic_string());
    const auto parsed =
        run3::content::parseXmlFile(contentRoot / relative, schema);
    CHECK_FALSE(parsed.root.name.empty());
  }
}

TEST_CASE("Exported Lua API matches the captured compatibility snapshot",
          "[step8][lua]") {
  const auto &bindings = run3::scripting::legacyBindingCatalog();
  REQUIRE(bindings.size() == 202);
  std::set<std::string> names;
  std::set<std::string> groups;
  for (const auto &binding : bindings) {
    CHECK(binding.signature == "int(lua_State*)");
    CHECK(names.insert(binding.name).second);
    groups.insert(binding.group);
  }
  CHECK(groups == std::set<std::string>{"audio", "devices", "npc", "physics",
                                        "player", "rendering", "sequence",
                                        "ui", "world"});
  CHECK(run3::scripting::exportedApiSnapshot() ==
        read(goldenRoot / "lua-exported-api.txt"));
}

TEST_CASE("ScriptEngine exposes grouped bindings without host libraries",
          "[step8][lua]") {
  auto engine = makeEngine();
  engine.executeFile(fixtureRoot / "scripts/representative.lua");
  const auto &calls = engine.calls();
  REQUIRE(calls.size() == 5);
  CHECK(calls[0].name == "playMusic");
  CHECK(calls[0].group == "audio");
  CHECK(calls[0].arguments == std::vector<std::string>{"music/theme.mp3"});
  CHECK(calls[4].name == "changeLevel");
}

TEST_CASE("ScriptEngine sandbox rejects paths outside approved roots",
          "[step8][lua]") {
  auto engine = makeEngine();
  CHECK_THROWS_WITH(engine.checkFile(sourceRoot / "PORTING.md"),
                    Catch::Matchers::ContainsSubstring("sandbox rejected"));
}

TEST_CASE("ScriptEngine reports syntax errors and protected tracebacks",
          "[step8][lua]") {
  auto engine = makeEngine();
  CHECK_THROWS_AS(
      engine.checkFile(fixtureRoot / "scripts/malformed.lua"),
      run3::scripting::ScriptError);
  try {
    engine.executeText(
        "local function nested() error('visible failure') end\nnested()",
        "traceback-fixture.lua");
    FAIL("runtime error was discarded");
  } catch (const run3::scripting::ScriptError &error) {
    CHECK(error.cause().find("visible failure") != std::string::npos);
    CHECK(error.cause().find("stack traceback") != std::string::npos);
    CHECK(std::string(error.what()).find("traceback-fixture.lua") !=
          std::string::npos);
  }

  run3::scripting::ScriptEngine dispatchFailure(
      {fixtureRoot / "scripts", fixtureRoot / "user", 1'000'000},
      [](const run3::scripting::ScriptCall &) -> run3::scripting::ScriptValue {
        throw std::runtime_error("visible dispatch failure");
      });
  CHECK_THROWS_WITH(
      dispatchFailure.executeText("playMusic('track')", "dispatch.lua"),
      Catch::Matchers::ContainsSubstring("visible dispatch failure"));
}

TEST_CASE("ScriptEngine terminates scripts over the instruction budget",
          "[step8][lua]") {
  auto engine = makeEngine(2'000);
  CHECK_THROWS_WITH(engine.executeText("while true do end", "budget.lua"),
                    Catch::Matchers::ContainsSubstring(
                        "instruction budget exceeded"));
}

TEST_CASE("All attached The Long Way scripts parse under Lua 5.4",
          "[step8][lua][content]") {
  const fs::path contentRoot =
      sourceRoot / "Games/The Long Way/TheLongWay/run3";
  if (!fs::is_directory(contentRoot)) {
    SKIP("Optional author-provided The Long Way content is not attached");
  }

  run3::scripting::ScriptEngine engine(
      {contentRoot, sourceRoot / "build/step8-test-user", 1'000'000});
  std::vector<std::string> failures;
  std::vector<std::string> shimmed;
  std::size_t count{};
  for (const auto &entry : fs::recursive_directory_iterator(contentRoot)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
      continue;
    }
    ++count;
    const std::string relative =
        entry.path().lexically_relative(contentRoot).generic_string();
    try {
      const auto result = engine.checkFile(entry.path());
      if (!result.compatibilityShims.empty()) {
        shimmed.push_back(relative);
      }
    } catch (const run3::scripting::ScriptError &) {
      failures.push_back(relative);
    }
  }
  std::sort(failures.begin(), failures.end());
  std::sort(shimmed.begin(), shimmed.end());
  CHECK(count == 955);
  CHECK(failures ==
        std::vector<std::string>{
            "lua/chapters/aeroxo/timer.lua",
            "lua/chapters/aeroxo2/timer.lua",
            "lua/chapters/manip/timer.lua"});
  CHECK(shimmed ==
        std::vector<std::string>{"lua/computers/demo_comp5.lua"});

  engine.clearCalls();
  engine.executeFile(contentRoot / "lua/chapters/tlwcao/startup_low.lua");
  engine.executeFile(contentRoot / "lua/chapters/aeroxo/but5.lua");
  engine.executeFile(contentRoot / "lua/chapters/tlwcao/lemz2.lua");
  engine.executeFile(contentRoot / "lua/chapters/tlwcao/cng.lua");
  std::set<std::string> called;
  for (const auto &call : engine.calls()) {
    called.insert(call.name);
  }
  CHECK(called.count("npcEvent") == 1);
  CHECK(called.count("enableTrigger") == 1);
  CHECK(called.count("playMusic") == 1);
  CHECK(called.count("gameText") == 1);
  CHECK(called.count("changeLevel") == 1);
}
