#include <run3/scripting/ScriptEngine.hpp>
#include <run3/ui/UiModel.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace {

std::string read(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

class TestUiFacade final : public run3::ui::IScriptUiFacade {
public:
  TestUiFacade() { openComputer("fixture-a"); }

  void openComputer(std::string owner) {
    registry_.beginContext(run3::ui::Context::Computer, std::move(owner));
    root_ = registry_.add({run3::ui::Context::Computer, {},
                           run3::ui::WidgetType::Panel, "computer.root",
                           {0, 0, 640, 480}, {}});
  }
  void closeComputer() { registry_.clearContext(run3::ui::Context::Computer); }

  run3::ui::WidgetHandle loadLayout(run3::ui::Context context,
                                     std::string_view key) override {
    if (context != run3::ui::Context::Computer || key != "default")
      throw std::invalid_argument("unapproved scoped UI layout key");
    return root_;
  }
  run3::ui::WidgetHandle createWidget(
      const run3::ui::WidgetSpec &spec) override {
    const auto handle = registry_.add(spec);
    text_[handle.id] = spec.text;
    return handle;
  }
  std::optional<run3::ui::WidgetHandle> findWidget(
      run3::ui::Context context, std::string_view name) const override {
    return registry_.find(context, name);
  }
  void destroyWidget(run3::ui::WidgetHandle handle) override {
    for (const auto removed : registry_.erase(handle)) text_.erase(removed.id);
  }
  void setText(run3::ui::WidgetHandle handle, std::string text) override {
    static_cast<void>(registry_.require(handle));
    text_[handle.id] = std::move(text);
  }
  void setVisible(run3::ui::WidgetHandle handle, bool) override {
    static_cast<void>(registry_.require(handle));
  }
  void setEnabled(run3::ui::WidgetHandle handle, bool) override {
    static_cast<void>(registry_.require(handle));
  }
  void setProperty(run3::ui::WidgetHandle handle, std::string_view name,
                   std::string) override {
    static_cast<void>(registry_.require(handle));
    if (name != "Alpha" && name != "TextAlign")
      throw std::invalid_argument("unsafe MyGUI property");
  }
  void focus(run3::ui::WidgetHandle handle) override {
    static_cast<void>(registry_.require(handle));
  }
  std::uint64_t setCallback(run3::ui::WidgetHandle handle,
                            run3::ui::UiEvent event,
                            run3::ui::UiCallback callback) override {
    return registry_.bind(handle, event, std::move(callback));
  }
  void clearCallback(std::uint64_t callback) noexcept override {
    registry_.unbind(callback);
  }
  void clearScriptCallbacks() noexcept override { registry_.clearCallbacks(); }

  void click(std::string_view name, std::string value = {}) {
    const auto handle = registry_.find(run3::ui::Context::Computer, name);
    REQUIRE(handle.has_value());
    registry_.emit(*handle, run3::ui::UiEvent::Click, std::move(value));
  }
  std::size_t callbacks() const noexcept { return registry_.callbackCount(); }

private:
  run3::ui::UiRegistry registry_;
  run3::ui::WidgetHandle root_;
  std::unordered_map<std::uint64_t, std::string> text_;
};

} // namespace

TEST_CASE("Step 9A UI handles are scoped and stale after teardown", "[step9a][ui]") {
  run3::ui::UiRegistry registry;
  registry.beginContext(run3::ui::Context::Computer, "first");
  const auto root = registry.add({run3::ui::Context::Computer, {},
                                  run3::ui::WidgetType::Panel, "root", {}, {}});
  const auto button = registry.add({run3::ui::Context::Computer, root,
                                    run3::ui::WidgetType::Button, "start", {}, {}});
  REQUIRE(run3::ui::WidgetHandle::parse(button.token()) == button);
  CHECK_THROWS(registry.add({run3::ui::Context::Computer, root,
                             run3::ui::WidgetType::Button, "start", {}, {}}));
  int calls = 0;
  static_cast<void>(registry.bind(button, run3::ui::UiEvent::Click,
                                  [&calls](std::string) { ++calls; }));
  registry.emit(button, run3::ui::UiEvent::Click);
  CHECK(calls == 1);
  registry.clearContext(run3::ui::Context::Computer);
  CHECK(registry.callbackCount() == 0);
  registry.beginContext(run3::ui::Context::Computer, "second");
  CHECK_THROWS(registry.require(button));
}

TEST_CASE("Step 9A layout covers required aspect ratios and high DPI",
          "[step9a][ui][layout]") {
  const auto wide = run3::ui::calculateLayout(1920, 1080, 1.0F);
  const auto ten = run3::ui::calculateLayout(1920, 1200, 1.0F);
  const auto classic = run3::ui::calculateLayout(1024, 768, 1.0F);
  const auto dpi = run3::ui::calculateLayout(2560, 1440, 2.0F);
  CHECK(wide.scale == 1.5F);
  CHECK(ten.offsetY > 0.0F);
  CHECK(classic.offsetY > 0.0F);
  CHECK(dpi.scale == 2.0F);
  CHECK_THROWS(run3::ui::calculateLayout(0, 720, 1.0F));
}

TEST_CASE("Step 9A typed MyGUI Lua facade is snapshotted and bounded",
          "[step9a][ui][lua]") {
  const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};
  CHECK(run3::scripting::myGuiApiSnapshot() ==
        read(sourceRoot / "tests/golden/mygui-exported-api.txt"));

  TestUiFacade facade;
  run3::scripting::ScriptEngine engine(
      {sourceRoot / "tests/fixtures/step8/scripts",
       sourceRoot / "tests/fixtures/step8/user", 10'000, &facade});
  engine.executeText(R"lua(
    local root = mygui.load_layout("computer", "default")
    button = mygui.create("computer", root, "button", "lua.start",
                          10, 20, 120, 32)
    callback = mygui.on(button, "click", function(value)
      playMusic(value)
    end)
  )lua", "typed-mygui-fixture.lua");
  REQUIRE(facade.callbacks() == 1);
  facade.click("lua.start", "music/click.wav");
  REQUIRE(engine.calls().size() == 1);
  CHECK(engine.calls().front().arguments ==
        std::vector<std::string>{"music/click.wav"});
  CHECK_THROWS(engine.executeText(
      "mygui.load_layout('computer', '../outside.layout')",
      "bad-layout.lua"));
  facade.closeComputer();
  facade.openComputer("fixture-b");
  CHECK_THROWS(engine.executeText("mygui.set_text(button, 'stale')",
                                  "stale-widget.lua"));
  CHECK(facade.callbacks() == 0);
}

TEST_CASE("Step 9A Lua UI callback instruction budget is enforced",
          "[step9a][ui][lua]") {
  const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};
  TestUiFacade facade;
  run3::scripting::ScriptEngine engine(
      {sourceRoot / "tests/fixtures/step8/scripts",
       sourceRoot / "tests/fixtures/step8/user", 1000, &facade});
  engine.executeText(R"lua(
    local root = mygui.load_layout("computer", "default")
    local widget = mygui.create("computer", root, "button", "loop", 0,0,1,1)
    mygui.on(widget, "click", function() while true do end end)
  )lua", "budgeted-ui.lua");
  CHECK_THROWS_AS(facade.click("loop"), run3::scripting::ScriptError);
}

TEST_CASE("Step 9A buttonGUI remains a scoped MyGUI compatibility facade",
          "[step9a][ui][lua][button-gui]") {
  const fs::path sourceRoot{RUN3_TEST_SOURCE_DIR};
  TestUiFacade facade;
  run3::scripting::ScriptEngine engine(
      {sourceRoot / "tests/fixtures/step8/scripts",
       sourceRoot / "tests/fixtures/step8/user", 10'000, &facade});
  engine.executeText(R"lua(
    buttonGUI_activateTopLeftComp640()
    legacy = buttonGUI_createButton("launch", "unused", "10 20",
                                    "120 32", "")
  )lua", "button-gui-fixture.lua");
  CHECK(facade.findWidget(run3::ui::Context::Computer,
                          "buttonGUI.launch").has_value());
  engine.executeText("buttonGUI_deleteAllButtons()", "button-gui-cleanup.lua");
  CHECK_FALSE(facade.findWidget(run3::ui::Context::Computer,
                                "buttonGUI.launch").has_value());
  CHECK(facade.callbacks() == 0);
}
