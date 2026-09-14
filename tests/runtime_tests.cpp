#include <run3/app/AppPaths.hpp>
#include <run3/app/Configuration.hpp>
#include <run3/app/EngineClock.hpp>
#include <run3/input/Input.hpp>
#include <run3/input/OgreBitesInputAdapter.hpp>
#include <run3/platform/OptionalDevices.hpp>

#include <OgreInput.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

TEST_CASE("AppPaths resolve relative roots from the executable") {
  const fs::path base = fs::temp_directory_path() / "run3-path-test";
  const run3::AppPaths paths = run3::AppPaths::resolve(
      base / "bin" / "run3_shell", fs::path("../content"),
      fs::path("../user"));

  CHECK(paths.contentRoot() == (base / "content").lexically_normal());
  CHECK(paths.userRoot() == (base / "user").lexically_normal());
  CHECK(paths.saveDir() == paths.userRoot() / "saves");
  CHECK(paths.logDir() == paths.userRoot() / "logs");
  CHECK(paths.configDir() == paths.userRoot() / "config");
  CHECK(paths.contentPath("maps/start.scene") ==
        paths.contentRoot() / "maps/start.scene");
  CHECK_THROWS_AS(paths.userPath("../content/file"), std::invalid_argument);
  CHECK(paths.saveDir().string().find(paths.contentRoot().string()) != 0);
}

TEST_CASE("Configuration precedence is CLI then user then content") {
  const run3::ConfigValues content{{"renderer", "gl3plus"},
                                   {"frames", "10"},
                                   {"quality", "low"}};
  const run3::ConfigValues user{{"renderer", "d3d11"},
                                {"quality", "high"}};
  const run3::ConfigValues cli{{"renderer", "test-renderer"}};
  const run3::Configuration result =
      run3::Configuration::merge(content, user, cli);

  CHECK(result.valueOr("renderer", "") == "test-renderer");
  CHECK(result.valueOr("quality", "") == "high");
  CHECK(result.unsignedOr("frames", 0) == 10);
  CHECK(result.valueOr("missing", "fallback") == "fallback");
}

TEST_CASE("OgreBites key and mouse values translate at the platform edge") {
  CHECK(run3::translateOgreBitesKey('w') == run3::Key::W);
  CHECK(run3::translateOgreBitesKey('7') == run3::Key::Num7);
  CHECK(run3::translateOgreBitesKey(OgreBites::SDLK_ESCAPE) ==
        run3::Key::Escape);
  CHECK(run3::translateOgreBitesKey(OgreBites::SDLK_UP) == run3::Key::Up);
  CHECK(run3::translateOgreBitesKey((1 << 30) | 60) == run3::Key::F3);
  CHECK(run3::translateOgreBitesKey(-12345) == run3::Key::Unknown);
  CHECK(run3::translateOgreBitesMouseButton(OgreBites::BUTTON_LEFT) ==
        run3::MouseButton::Left);
}

TEST_CASE("Focus loss releases held input") {
  run3::InputEvent keyDown;
  keyDown.type = run3::InputEventType::KeyPressed;
  keyDown.key = run3::Key::W;
  run3::InputEvent mouseDown;
  mouseDown.type = run3::InputEventType::MousePressed;
  mouseDown.mouseButton = run3::MouseButton::Left;
  run3::InputEvent focusLost;
  focusLost.type = run3::InputEventType::FocusLost;
  run3::InputEvent resized;
  resized.type = run3::InputEventType::Resized;
  resized.width = 1920;
  resized.height = 1080;

  run3::ReplayInput input({{keyDown, mouseDown}, {focusLost, resized}});
  input.poll();
  REQUIRE(input.state().keyDown(run3::Key::W));
  REQUIRE(input.state().mouseButtonDown(run3::MouseButton::Left));
  input.poll();
  CHECK_FALSE(input.state().focused());
  CHECK_FALSE(input.state().keyDown(run3::Key::W));
  CHECK_FALSE(input.state().mouseButtonDown(run3::MouseButton::Left));
  CHECK(input.state().windowWidth() == 1920);
  CHECK(input.state().windowHeight() == 1080);
}

TEST_CASE("Disabled optional devices use visible no-op implementations") {
#if RUN3_ENABLE_OPTIONAL_DEVICES
  SUCCEED("Hardware backends are enabled; no device is touched by tests");
#else
  auto serial = run3::createSerialDevice();
  auto pipe = run3::createNamedPipeDevice();
  CHECK_FALSE(serial->open(2, 9600));
  CHECK_FALSE(serial->isOpen());
  CHECK_FALSE(pipe->connect("run3-test"));
  CHECK_FALSE(pipe->isConnected());
#endif
}

TEST_CASE("Variable EngineClock returns one clamped simulation step") {
  run3::EngineClock clock(run3::ClockMode::Variable);
  const run3::ClockFrame frame = clock.advance(10ms);
  CHECK(frame.simulationSteps == 1);
  CHECK(frame.elapsed == run3::EngineClock::Duration(10ms));
  CHECK(frame.simulationStep == frame.elapsed);
}

TEST_CASE("Fixed EngineClock accumulates and bounds catch-up") {
  run3::EngineClock clock(run3::ClockMode::Fixed, 10ms, 3, 100ms);
  CHECK(clock.advance(6ms).simulationSteps == 0);
  const run3::ClockFrame accumulated = clock.advance(24ms);
  CHECK(accumulated.simulationSteps == 3);
  CHECK(accumulated.interpolationAlpha == 0.0);
  const run3::ClockFrame bounded = clock.advance(100ms);
  CHECK(bounded.simulationSteps == 3);
  CHECK(bounded.interpolationAlpha == 0.0);
}
