#include <run3/app/Run3App.hpp>

#include <run3/core/Log.hpp>
#include <run3/content/AssetValidation.hpp>
#include <run3/content/OgreAssetValidation.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/StaticMap.hpp>
#include <run3/physics/Physics.hpp>

#include <OgreCamera.h>
#include <OgreColourValue.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreOverlaySystem.h>
#include <OgreRenderSystem.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>

#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
#include <OgreRTShaderSystem.h>
#endif

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <cmath>
#include <utility>

namespace run3 {
namespace fs = std::filesystem;

namespace {

constexpr char ogreVersion[] = "14.5.2";
static_assert(OGRE_VERSION_MAJOR == 14 && OGRE_VERSION_MINOR == 5 &&
                  OGRE_VERSION_PATCH == 2,
              "Run3App must be built with pinned Ogre 14.5.2");

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

std::optional<fs::path> configuredRoot(const Configuration &configuration,
                                       std::string_view key,
                                       const fs::path &executableDir) {
  const auto value = configuration.find(key);
  if (!value || value->empty()) {
    return std::nullopt;
  }
  fs::path path(*value);
  if (!path.is_absolute()) {
    path = executableDir / path;
  }
  return fs::absolute(path).lexically_normal();
}

fs::path configuredPath(fs::path path, const fs::path &executableDir) {
  if (!path.is_absolute()) {
    path = executableDir / path;
  }
  return fs::absolute(path).lexically_normal();
}

bool configuredBool(const Configuration &configuration, std::string_view key,
                    bool fallback = false) {
  const std::string value = lower(configuration.valueOr(key, fallback ? "true" : "false"));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    return false;
  }
  throw std::runtime_error("Configuration value for '" + std::string(key) +
                           "' is not a boolean: " + value);
}

double configuredDouble(const Configuration &configuration,
                        std::string_view key, double fallback = 0.0) {
  const auto value = configuration.find(key);
  if (!value) {
    return fallback;
  }
  std::size_t consumed{};
  const double result = std::stod(*value, &consumed);
  if (consumed != value->size() || !std::isfinite(result) || result < 0.0) {
    throw std::runtime_error("Configuration value for '" + std::string(key) +
                             "' is invalid: " + *value);
  }
  return result;
}

} // namespace

Run3AppOptions loadRun3AppOptions(int argc, char **argv,
                                 bool validationByDefault) {
  std::vector<std::string> arguments;
  arguments.reserve(static_cast<std::size_t>(argc));
  for (int index = 0; index < argc; ++index) {
    arguments.emplace_back(argv[index] == nullptr ? "" : argv[index]);
  }

  const fs::path executable =
      AppPaths::executablePath(argc > 0 ? argv[0] : nullptr);
  const fs::path executableDir =
      fs::absolute(executable).lexically_normal().parent_path();
  const CommandLine commandLine = parseCommandLine(arguments, executableDir);
  if (commandLine.help) {
    printRun3AppUsage();
    Run3AppOptions helpOptions;
    helpOptions.paths = AppPaths::resolve(executable);
    return helpOptions;
  }

  const bool validateContent =
      validationByDefault || commandLine.validateContent;
  std::optional<fs::path> requestedContentRoot = commandLine.contentRoot;
  if (validateContent && !requestedContentRoot) {
    requestedContentRoot =
        (executableDir / ".." / "share" / "run3" / "validation-fixture")
            .lexically_normal();
  }

  AppPaths paths = AppPaths::resolve(executable, requestedContentRoot,
                                     commandLine.userRoot);
  const ConfigValues contentDefaults =
      Configuration::readFile(paths.contentPath("config/run3.cfg"));
  const ConfigValues userValues =
      Configuration::readFile(paths.configDir() / "run3.cfg");
  const Configuration configuration = Configuration::merge(
      contentDefaults, userValues, commandLine.values);

  std::optional<fs::path> contentRoot = requestedContentRoot;
  std::optional<fs::path> userRoot = commandLine.userRoot;
  if (!contentRoot) {
    contentRoot = configuredRoot(configuration, "content-root", executableDir);
  }
  if (!userRoot) {
    userRoot = configuredRoot(configuration, "user-root", executableDir);
  }
  if (contentRoot || userRoot) {
    paths = AppPaths::resolve(executable, contentRoot, userRoot);
  }

#ifdef _WIN32
  const std::string defaultRenderer = "d3d11";
#else
  const std::string defaultRenderer = "gl3plus";
#endif
  std::uint64_t frames = configuration.unsignedOr("frames", 0);
  if (validateContent && frames == 0) {
    frames = 1;
  }
  const fs::path installedShare =
      (executableDir / ".." / "share" / "run3").lexically_normal();
  fs::path manifestPath = configuration.valueOr("manifest", "");
  if (manifestPath.empty()) {
    const fs::path localManifest = paths.contentRoot() / "manifest.json";
    manifestPath = fs::is_regular_file(localManifest)
                       ? localManifest
                       : installedShare / "manifests" / "the-long-way-v1.json";
  }
  fs::path reportPath = configuration.valueOr("report", "");
  if (reportPath.empty()) {
    reportPath = paths.logDir() / "asset-report.json";
  }
  Run3AppOptions options;
  options.paths = std::move(paths);
  options.renderer = configuration.valueOr("renderer", defaultRenderer);
  options.frameLimit = frames;
  options.explicitContentRoot = contentRoot.has_value();
  options.validateContent = validateContent;
  options.manifestPath = configuredPath(std::move(manifestPath), executableDir);
  options.reportPath = configuredPath(std::move(reportPath), executableDir);
  options.renderFixture = installedShare / "validation-fixture" / "step5.scene";
  options.mapName = configuration.valueOr("map", "");
  options.mapQuality = configuration.valueOr("map-quality", "low");
  options.resourceProfile =
      configuration.valueOr("resource-profile", "resources_low_low.cfg");
  options.renderHz = configuredDouble(configuration, "render-hz");
  options.startNoclip = configuredBool(configuration, "noclip");
  options.physicsDebug = configuredBool(configuration, "physics-debug");
  return options;
}

void printRun3AppUsage() {
  std::cout
      << "Usage: run3_shell [--renderer d3d11|gl3plus] [--frames N]"
         " [--user-dir PATH] [--content-root PATH]\n"
      << "       [--validate-content] [--manifest PATH] [--report PATH]\n"
      << "       [--map tlwcao|tlwhome02] [--map-quality low|medium|high]\n"
      << "       [--resource-profile FILE] [--noclip] [--physics-debug]\n"
      << "       [--render-hz 30|60|144]\n"
      << "Precedence: command line > user config > content defaults.\n"
      << "Relative paths are resolved from the executable directory.\n";
}

Run3App::Run3App(Run3AppOptions options)
    : OgreBites::ApplicationContext("Run3 renderer shell"),
      options_(std::move(options)), inputAdapter_(input_),
      clock_(ClockMode::Fixed) {}

int Run3App::run() {
  options_.paths.createWritableDirectories();
  initApp();
  if (getRoot() == nullptr || getRoot()->getRenderSystem() == nullptr) {
    throw std::runtime_error("Ogre application initialization failed");
  }

  clock_.reset();
  try {
    while (!quitRequested_ && !getRoot()->endRenderingQueued()) {
      pollEvents();
      handleInput(input_.poll());
      if (quitRequested_) {
        break;
      }
      const ClockFrame frame = options_.renderHz > 0.0
                                   ? clock_.advance(EngineClock::Duration{
                                         1.0 / options_.renderHz})
                                   : clock_.tick();
      if (player_) {
        gameplay::PlayerCommand command;
        const InputState &state = input_.state();
        command.forward = (state.keyDown(Key::W) || state.keyDown(Key::Up) ? 1.0 : 0.0) -
                          (state.keyDown(Key::S) || state.keyDown(Key::Down) ? 1.0 : 0.0);
        command.strafe = (state.keyDown(Key::D) || state.keyDown(Key::Right) ? 1.0 : 0.0) -
                         (state.keyDown(Key::A) || state.keyDown(Key::Left) ? 1.0 : 0.0);
        command.run = state.keyDown(Key::LeftShift) || state.keyDown(Key::RightShift);
        command.jump = state.keyDown(Key::Space);
        command.crouch = state.keyDown(Key::LeftControl) || state.keyDown(Key::RightControl);
        if (player_->state().noclip) {
          command.vertical = (state.keyDown(Key::Space) ? 1.0 : 0.0) -
                             (command.crouch ? 1.0 : 0.0);
          command.jump = false;
          command.crouch = false;
        }
        bool onLadder = false;
        if (staticMap_) {
          for (const auto &volume : staticMap_->ladderVolumes()) {
            onLadder = onLadder || volume.contains(player_->state().position);
          }
        }
        player_->setOnLadder(onLadder);
        player_->setCommand(command);
        player_->setYawRadians(yawRadians_);
        for (std::size_t step = 0; step < frame.simulationSteps; ++step) {
          static_cast<void>(player_->simulateFixedStep());
          ++simulatedSteps_;
        }
        const physics::Vec3 eye = player_->eyePosition();
        cameraNode_->setPosition(static_cast<Ogre::Real>(eye.x),
                                 static_cast<Ogre::Real>(eye.y),
                                 static_cast<Ogre::Real>(eye.z));
        cameraNode_->setOrientation(
            Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(yawRadians_)),
                             Ogre::Vector3::UNIT_Y) *
            Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(pitchRadians_)),
                             Ogre::Vector3::UNIT_X));
      }
      if (cubeNode_ != nullptr) {
        cubeNode_->yaw(Ogre::Degree(
            30.0F * static_cast<float>(frame.elapsed.count())));
      }
      if (!getRoot()->renderOneFrame(
              static_cast<Ogre::Real>(frame.elapsed.count()))) {
        break;
      }
      ++renderedFrames_;
      if (options_.frameLimit != 0 && renderedFrames_ >= options_.frameLimit) {
        requestQuit();
      }
    }
  } catch (...) {
    player_.reset();
    staticMap_.reset();
    physicsWorld_.reset();
    closeApp();
    throw;
  }
  if (player_) {
    const gameplay::PlayerState finalState = player_->state();
    Ogre::LogManager::getSingleton().logMessage(
        "Step 6B final player: steps=" + std::to_string(simulatedSteps_) +
        " position=" + std::to_string(finalState.position.x) + "," +
        std::to_string(finalState.position.y) + "," +
        std::to_string(finalState.position.z));
  }
  player_.reset();
  staticMap_.reset();
  physicsWorld_.reset();
  closeApp();
  return validationFailed_ ? 2 : 0;
}

void Run3App::createRoot() {
  const fs::path pluginConfig = options_.paths.executableDir() / "plugins.cfg";
  if (!fs::is_regular_file(pluginConfig)) {
    throw std::runtime_error("Missing installed Ogre plugin configuration: " +
                             pluginConfig.string());
  }
  mRoot = OGRE_NEW Ogre::Root(
      pluginConfig.string(),
      (options_.paths.configDir() / "ogre.cfg").string(),
      (options_.paths.logDir() / "ogre.log").string());
  mOverlaySystem = OGRE_NEW Ogre::OverlaySystem();
}

bool Run3App::oneTimeConfig() {
  const std::string requested = lower(options_.renderer);
  const std::string wanted =
      requested == "d3d11" ? "direct3d11"
      : (requested == "gl3+" || requested == "gl3plus") ? "opengl 3+"
                                                           : requested;
  Ogre::RenderSystem *selected{};
  for (Ogre::RenderSystem *renderer : mRoot->getAvailableRenderers()) {
    if (lower(renderer->getName()).find(wanted) != std::string::npos) {
      selected = renderer;
      break;
    }
  }
  if (selected == nullptr) {
    std::string available;
    for (const Ogre::RenderSystem *renderer : mRoot->getAvailableRenderers()) {
      available += available.empty() ? "" : ", ";
      available += renderer->getName();
    }
    throw std::runtime_error("Requested renderer '" + options_.renderer +
                             "' is unavailable; available: " + available);
  }
  const Ogre::ConfigOptionMap &config = selected->getConfigOptions();
  if (config.find("Full Screen") != config.end()) {
    selected->setConfigOption("Full Screen", "No");
  }
  if (config.find("VSync") != config.end()) {
    selected->setConfigOption("VSync", "No");
  }
  mRoot->setRenderSystem(selected);
  return true;
}

void Run3App::locateResources() {
  Ogre::ResourceGroupManager &resources =
      Ogre::ResourceGroupManager::getSingleton();
  const fs::path media =
      (options_.paths.executableDir() / ".." / "share" / "run3" / "Media")
          .lexically_normal();
  resources.addResourceLocation((media / "Main").string(), "FileSystem",
                                Ogre::RGN_INTERNAL);
  resources.addResourceLocation((media / "RTShaderLib").string(), "FileSystem",
                                Ogre::RGN_INTERNAL);
  // Content validation registers only the locations selected by resource.cfg
  // in its own temporary group. Registering the root here as well makes Ogre's
  // archive manager reject the same filesystem archive under two groups.
  if (fs::is_directory(options_.paths.contentRoot())) {
    if (!options_.validateContent) {
      resources.addResourceLocation(options_.paths.contentRoot().string(),
                                    "FileSystem", "Run3Content");
    }
  } else if (options_.explicitContentRoot) {
    throw std::runtime_error("Content root is not a directory: " +
                             options_.paths.contentRoot().string());
  }
}

void Run3App::setup() {
  OgreBites::ApplicationContext::setup();
  addInputListener(&inputAdapter_);
  sceneManager_ = mRoot->createSceneManager();
  sceneManager_->addRenderQueueListener(mOverlaySystem);
#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
  mShaderGenerator->addSceneManager(sceneManager_);
#endif
  sceneManager_->setAmbientLight(Ogre::ColourValue(0.25F, 0.25F, 0.25F));

  Ogre::Light *light = sceneManager_->createLight("Run3ShellLight");
  light->setType(Ogre::Light::LT_DIRECTIONAL);
  Ogre::SceneNode *lightNode =
      sceneManager_->getRootSceneNode()->createChildSceneNode();
  lightNode->setDirection(Ogre::Vector3(-1.0F, -1.0F, -1.0F).normalisedCopy(),
                          Ogre::Node::TS_WORLD);
  lightNode->attachObject(light);
  if (options_.mapName.empty()) {
    Ogre::Entity *cube =
        sceneManager_->createEntity("Run3ShellCube", Ogre::SceneManager::PT_CUBE);
    cubeNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
    cubeNode_->attachObject(cube);
  }

  camera_ = sceneManager_->createCamera("Run3ShellCamera");
  camera_->setNearClipDistance(5.0F);
  cameraNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
  cameraNode_->setPosition(0.0F, 75.0F, 300.0F);
  cameraNode_->lookAt(Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD);
  cameraNode_->attachObject(camera_);
  Ogre::Viewport *viewport = getRenderWindow()->addViewport(camera_);
  viewport->setBackgroundColour(Ogre::ColourValue(0.04F, 0.06F, 0.1F));
  updateAspectRatio();

  if (!options_.mapName.empty()) {
    physicsWorld_ = std::make_unique<physics::PhysicsWorld>(
        physics::createBulletPhysicsWorld());
    staticMap_ = std::make_unique<gameplay::StaticMap>(*sceneManager_,
                                                       *physicsWorld_);
    const gameplay::StaticMapStats mapStats = staticMap_->load(
        {options_.paths.contentRoot(), options_.mapName, options_.mapQuality,
         options_.resourceProfile});
    static_cast<void>(mapStats);
    player_ = std::make_unique<gameplay::PlayerController>(*physicsWorld_);
    player_->spawn(staticMap_->spawnPosition());
    player_->setNoclip(options_.startNoclip);
    physicsDebug_ = options_.physicsDebug;
    staticMap_->setDebugDraw(physicsDebug_);
    camera_->setNearClipDistance(5.0F);
    camera_->setFarClipDistance(100000.0F);
  }

  Ogre::LogManager::getSingleton().logMessage(
      "Run3 shell pinned Ogre version: " + std::string(ogreVersion));
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 shell selected render system: " + mRoot->getRenderSystem()->getName());
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 shell content root (read-only): " + options_.paths.contentRoot().string());
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 shell user root (writable): " + options_.paths.userRoot().string());

  if (options_.validateContent) {
    AssetReport report = validateContent(
        {options_.paths.contentRoot(), options_.manifestPath});
    validateOgreContent(report, *sceneManager_, options_.renderFixture);
    report.writeJson(options_.reportPath);
    std::cout << report.conciseReport();
    std::cout << "  JSON report: " << options_.reportPath.string() << '\n';
    validationFailed_ = !report.passed();
  }
}

void Run3App::windowResized(Ogre::RenderWindow *window) {
  updateAspectRatio();
  InputEvent event;
  event.type = InputEventType::Resized;
  event.width = window->getWidth();
  event.height = window->getHeight();
  input_.push(std::move(event));
}

bool Run3App::windowClosing(Ogre::RenderWindow *) {
  InputEvent event;
  event.type = InputEventType::Quit;
  input_.push(std::move(event));
  return true;
}

void Run3App::windowClosed(Ogre::RenderWindow *) { requestQuit(); }

void Run3App::windowFocusChange(Ogre::RenderWindow *window) {
  InputEvent event;
  event.type = window->isActive() ? InputEventType::FocusGained
                                  : InputEventType::FocusLost;
  input_.push(std::move(event));
}

void Run3App::updateAspectRatio() {
  if (camera_ == nullptr || getRenderWindow() == nullptr ||
      getRenderWindow()->getHeight() == 0) {
    return;
  }
  camera_->setAspectRatio(static_cast<Ogre::Real>(getRenderWindow()->getWidth()) /
                          static_cast<Ogre::Real>(getRenderWindow()->getHeight()));
}

void Run3App::handleInput(const std::vector<InputEvent> &events) {
  for (const InputEvent &event : events) {
    if (event.type == InputEventType::Quit ||
        (event.type == InputEventType::KeyPressed && event.key == Key::Escape)) {
      requestQuit();
    } else if (player_ && event.type == InputEventType::MouseMoved) {
      constexpr double sensitivity = 0.0025;
      yawRadians_ -= static_cast<double>(event.deltaX) * sensitivity;
      pitchRadians_ = std::clamp(
          pitchRadians_ - static_cast<double>(event.deltaY) * sensitivity,
          -1.553343034, 1.553343034);
    } else if (player_ && event.type == InputEventType::KeyPressed &&
               !event.repeated && event.key == Key::N) {
      player_->toggleNoclip();
      Ogre::LogManager::getSingleton().logMessage(
          std::string("Player noclip: ") +
          (player_->state().noclip ? "enabled" : "disabled"));
    } else if (staticMap_ && event.type == InputEventType::KeyPressed &&
               !event.repeated && event.key == Key::F3) {
      physicsDebug_ = !physicsDebug_;
      staticMap_->setDebugDraw(physicsDebug_);
    } else if (player_ && event.type == InputEventType::KeyPressed &&
               !event.repeated && event.key == Key::E) {
      const auto hit = player_->useRaycast(pitchRadians_);
      Ogre::LogManager::getSingleton().logMessage(
          hit ? "Use ray hit body " + std::to_string(hit->body)
              : "Use ray missed");
    } else if (player_ && event.type == InputEventType::MousePressed &&
               event.mouseButton == MouseButton::Left) {
      const auto hit = player_->weaponRaycast(pitchRadians_);
      Ogre::LogManager::getSingleton().logMessage(
          hit ? "Weapon ray hit body " + std::to_string(hit->body)
              : "Weapon ray missed");
    }
  }
}

void Run3App::requestQuit() {
  quitRequested_ = true;
  if (mRoot != nullptr) {
    mRoot->queueEndRendering();
  }
}

} // namespace run3
