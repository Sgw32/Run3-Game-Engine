#include <run3/app/Run3App.hpp>

#include <run3/audio/Audio.hpp>
#include <run3/audio/MapAudio.hpp>
#include <run3/core/Log.hpp>
#include <run3/content/AssetValidation.hpp>
#include <run3/content/OgreAssetValidation.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/OgreSequenceServices.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>
#include <run3/gameplay/StaticMap.hpp>
#include <run3/physics/Physics.hpp>
#include <run3/ui/OgreMyGui.hpp>
#include <run3/ui/Ui.hpp>

#include <OgreCamera.h>
#include <OgreColourValue.h>
#include <OgreEntity.h>
#include <OgreException.h>
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
#include <charconv>
#include <cctype>
#include <filesystem>
#include <exception>
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

std::string configuredQuality(const Configuration &configuration,
                              std::string_view key,
                              std::string fallback) {
  std::string value = lower(configuration.valueOr(key, std::move(fallback)));
  if (value == "med") value = "medium";
  if (value != "low" && value != "medium" && value != "high") {
    throw std::runtime_error("Configuration value for '" + std::string(key) +
                             "' must be low, medium, or high: " + value);
  }
  return value;
}

std::pair<std::uint32_t, std::uint32_t>
configuredResolution(const Configuration &configuration) {
  const std::string value = configuration.valueOr("resolution", "1280x720");
  const std::size_t separator = value.find_first_of("xX");
  if (separator == std::string::npos || separator == 0 ||
      separator + 1 == value.size()) {
    throw std::runtime_error(
        "resolution must use WIDTHxHEIGHT syntax: " + value);
  }
  const auto parsePart = [&value](std::string_view part) {
    std::uint32_t result{};
    const auto parsed = std::from_chars(part.data(),
                                        part.data() + part.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != part.data() + part.size()) {
      throw std::runtime_error(
          "resolution must use WIDTHxHEIGHT syntax: " + value);
    }
    return result;
  };
  const std::uint32_t width = parsePart(
      std::string_view(value).substr(0, separator));
  const std::uint32_t height = parsePart(
      std::string_view(value).substr(separator + 1));
  if (width < 640 || height < 480 || width > 16384 || height > 16384) {
    throw std::runtime_error(
        "resolution must be between 640x480 and 16384x16384: " + value);
  }
  return {width, height};
}

std::string profileQualityToken(const std::string &quality) {
  return quality == "medium" ? "med" : quality;
}

double meshLodBias(const std::string &quality) {
  if (quality == "high") return 2.0;
  if (quality == "low") return 0.5;
  return 1.0;
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
  options.audioBackend = lower(configuration.valueOr("audio-backend", "auto"));
  if (options.audioBackend != "auto" && options.audioBackend != "miniaudio" &&
      options.audioBackend != "null") {
    throw std::runtime_error(
        "audio-backend must be auto, miniaudio, or null");
  }
  options.frameLimit = frames;
  options.explicitContentRoot = contentRoot.has_value();
  options.validateContent = validateContent;
  options.manifestPath = configuredPath(std::move(manifestPath), executableDir);
  options.reportPath = configuredPath(std::move(reportPath), executableDir);
  options.renderFixture = installedShare / "validation-fixture" / "step5.scene";
  options.mapName = configuration.valueOr("map", "");
  options.mapQuality = configuredQuality(
      configuration, "scene-quality",
      configuration.valueOr("map-quality", "low"));
  options.textureQuality =
      configuredQuality(configuration, "texture-quality", "low");
  options.modelQuality =
      configuredQuality(configuration, "model-quality", "medium");
  if (const auto profile = configuration.find("resource-profile")) {
    options.resourceProfile = *profile;
  } else {
    options.resourceProfile =
        "resources_" + profileQualityToken(options.textureQuality) + "_" +
        profileQualityToken(options.mapQuality) + ".cfg";
  }
  options.playerHeightCm =
      configuredDouble(configuration, "player-height-cm", 180.0);
  if (options.playerHeightCm < 120.0 || options.playerHeightCm > 240.0) {
    throw std::runtime_error(
        "player-height-cm must be between 120 and 240 centimetres");
  }
  options.verticalFovDegrees = configuredDouble(configuration, "fov", 75.0);
  if (options.verticalFovDegrees < 35.0 ||
      options.verticalFovDegrees > 120.0) {
    throw std::runtime_error("fov must be between 35 and 120 degrees");
  }
  const auto [width, height] = configuredResolution(configuration);
  options.windowWidth = width;
  options.windowHeight = height;
  options.renderHz = configuredDouble(configuration, "render-hz");
  options.fullscreen = configuredBool(configuration, "fullscreen");
  options.startNoclip = configuredBool(configuration, "noclip");
  options.physicsDebug = configuredBool(configuration, "physics-debug");
  options.introEnabled = configuredBool(configuration, "intro", false);
  options.uiScale = static_cast<float>(
      configuredDouble(configuration, "ui-scale", 1.0));
  if (options.uiScale < 0.75F || options.uiScale > 3.0F)
    throw std::runtime_error("ui-scale must be between 0.75 and 3.0");
  options.newGameMap = configuration.valueOr("new-game-map", "tlwintro");
  if (options.newGameMap.empty())
    throw std::runtime_error("new-game-map must not be empty");
  return options;
}

void printRun3AppUsage() {
  std::cout
      << "Usage: run3_shell [--renderer d3d11|gl3plus]"
         " [--frames N]"
         " [--user-dir PATH] [--content-root PATH]\n"
      << "       [--validate-content] [--manifest PATH] [--report PATH]\n"
      << "       [--map NAME] [--scene-quality low|medium|high]\n"
      << "       [--texture-quality low|medium|high]"
         " [--model-quality low|medium|high]\n"
      << "       [--resource-profile FILE] [--player-height-cm N]\n"
      << "       [--fov 35..120] [--resolution WIDTHxHEIGHT]\n"
      << "       [--fullscreen|--windowed] [--noclip] [--physics-debug]\n"
      << "       [--intro|--skip-intro] [--ui-scale 0.75..3]\n"
      << "       [--new-game-map NAME]\n"
      << "       [--audio-backend auto|miniaudio|null]\n"
      << "       [--render-hz 30|60|144]\n"
      << "Precedence: command line > user config > content defaults.\n"
      << "Relative paths are resolved from the executable directory.\n";
}

Run3App::Run3App(Run3AppOptions options)
    : OgreBites::ApplicationContext("Run3 renderer shell"),
      options_(std::move(options)), inputAdapter_(input_),
      clock_(ClockMode::Fixed) {}
Run3App::~Run3App() {
  // initApp() may throw before run() reaches its cleanup block.
  try { unloadMap(false); } catch (...) {}
}

int Run3App::run() {
  options_.paths.createWritableDirectories();
  initApp();
  if (getRoot() == nullptr || getRoot()->getRenderSystem() == nullptr) {
    throw std::runtime_error("Ogre application initialization failed");
  }

  clock_.reset();
  std::string runtimeFailure;
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
        const bool frozen = sequenceRuntime_ &&
                            sequenceRuntime_->presentation().playerFrozen;
        command.forward = frozen ? 0.0 : (state.keyDown(Key::W) || state.keyDown(Key::Up) ? 1.0 : 0.0) -
                          (state.keyDown(Key::S) || state.keyDown(Key::Down) ? 1.0 : 0.0);
        command.strafe = frozen ? 0.0 : (state.keyDown(Key::D) || state.keyDown(Key::Right) ? 1.0 : 0.0) -
                         (state.keyDown(Key::A) || state.keyDown(Key::Left) ? 1.0 : 0.0);
        command.run = !frozen && (state.keyDown(Key::LeftShift) || state.keyDown(Key::RightShift));
        command.jump = !frozen && state.keyDown(Key::Space);
        command.crouch = !frozen && (state.keyDown(Key::LeftControl) || state.keyDown(Key::RightControl));
        if (player_->state().noclip && !frozen) {
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
        if (sequenceRuntime_) {
          onLadder = onLadder || sequenceRuntime_->playerOnLadder();
        }
        player_->setOnLadder(onLadder);
        player_->setCommand(command);
        player_->setYawRadians(yawRadians_);
        for (std::size_t step = 0; step < frame.simulationSteps; ++step) {
          if (sequenceRuntime_) {
            sequenceRuntime_->fixedUpdate();
          }
          if (npcSystem_) npcSystem_->fixedUpdate();
          static_cast<void>(player_->simulateFixedStep());
          ++simulatedSteps_;
        }
        if (staticMap_) {
          staticMap_->syncDynamicTransforms();
        }
        const physics::Vec3 eye = player_->eyePosition();
        const auto &presentation = sequenceRuntime_
            ? sequenceRuntime_->presentation()
            : gameplay::SequencePresentationState{};
        if (presentation.camera) {
          const auto &pose = *presentation.camera;
          cameraNode_->setPosition(static_cast<Ogre::Real>(pose.position.x),
                                   static_cast<Ogre::Real>(pose.position.y),
                                   static_cast<Ogre::Real>(pose.position.z));
          cameraNode_->setOrientation(
              static_cast<Ogre::Real>(pose.rotation.w),
              static_cast<Ogre::Real>(pose.rotation.x),
              static_cast<Ogre::Real>(pose.rotation.y),
              static_cast<Ogre::Real>(pose.rotation.z));
        } else {
          cameraNode_->setPosition(static_cast<Ogre::Real>(eye.x),
                                   static_cast<Ogre::Real>(eye.y),
                                   static_cast<Ogre::Real>(eye.z));
          cameraNode_->setOrientation(
              Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(yawRadians_)),
                               Ogre::Vector3::UNIT_Y) *
              Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(pitchRadians_)),
                               Ogre::Vector3::UNIT_X));
        }
      }
      if (pendingMapChange_) {
        std::string nextMap = std::move(*pendingMapChange_);
        pendingMapChange_.reset();
        Ogre::LogManager::getSingleton().logMessage(
            "Step 8E map transition: " + options_.mapName + " -> " +
            nextMap);
        unloadMap(true);
        loadMap(nextMap);
        continue;
      }
      if (cubeNode_ != nullptr) {
        cubeNode_->yaw(Ogre::Degree(
            30.0F * static_cast<float>(frame.elapsed.count())));
      }
      if (audioEngine_ != nullptr && cameraNode_ != nullptr) {
        const Ogre::Vector3 position = cameraNode_->getPosition();
        const Ogre::Quaternion orientation = cameraNode_->getOrientation();
        const Ogre::Vector3 forward = orientation * Ogre::Vector3::NEGATIVE_UNIT_Z;
        const Ogre::Vector3 up = orientation * Ogre::Vector3::UNIT_Y;
        audio::ListenerTransform listener;
        listener.position = {position.x, position.y, position.z};
        if (player_) {
          const physics::Vec3 velocity = player_->state().velocity;
          listener.velocity = {static_cast<float>(velocity.x),
                               static_cast<float>(velocity.y),
                               static_cast<float>(velocity.z)};
        }
        listener.forward = {forward.x, forward.y, forward.z};
        listener.up = {up.x, up.y, up.z};
        audioEngine_->setListener(listener);
        if (mapAudio_) {
          std::optional<audio::FootstepState> footstep;
          if (player_) {
            const gameplay::PlayerState state = player_->state();
            footstep = audio::FootstepState{
                {static_cast<float>(state.position.x),
                 static_cast<float>(state.position.y),
                 static_cast<float>(state.position.z)},
                {static_cast<float>(state.velocity.x),
                 static_cast<float>(state.velocity.y),
                 static_cast<float>(state.velocity.z)},
                state.grounded, state.noclip};
          }
          mapAudio_->update(static_cast<float>(frame.elapsed.count()),
                            footstep ? &*footstep : nullptr);
        }
        if (sequenceServices_) {
          sequenceServices_->updateAudio(
              static_cast<float>(frame.elapsed.count()));
        }
        audioEngine_->update(static_cast<float>(frame.elapsed.count()));
      }
      if (ui_) {
        ui_->update(static_cast<float>(frame.elapsed.count()));
        ui_->renderComputerSurface();
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
  } catch (const Ogre::Exception &error) {
    // Copy plugin-owned exception text before closeApp() unloads the render
    // system DLL which supplied the exception's code and RTTI.
    runtimeFailure = "Ogre main-loop error: " + error.getFullDescription();
  } catch (const std::exception &error) {
    runtimeFailure = "Run3 main-loop error: " + std::string(error.what());
  } catch (...) {
    runtimeFailure = "Run3 main-loop error: unknown exception";
  }
  if (!runtimeFailure.empty()) {
    logError(runtimeFailure);
    if (gameplayMouseCapture_) {
      setGameplayMouseCapture(false);
    }
    try {
      unloadMap(false);
    } catch (const std::exception &error) {
      logError("Run3 cleanup error after main-loop failure: " +
               std::string(error.what()));
    } catch (...) {
      logError("Run3 cleanup error after main-loop failure: unknown exception");
    }
    ui_.reset();
    audioEngine_.reset();
    closeApp();
    return 1;
  }
  if (player_) {
    const gameplay::PlayerState finalState = player_->state();
    Ogre::LogManager::getSingleton().logMessage(
        "Step 6B final player: steps=" + std::to_string(simulatedSteps_) +
        " position=" + std::to_string(finalState.position.x) + "," +
        std::to_string(finalState.position.y) + "," +
        std::to_string(finalState.position.z));
  }
  if (gameplayMouseCapture_) {
    setGameplayMouseCapture(false);
  }
  std::exception_ptr exitFailure;
  try {
    unloadMap(true);
  } catch (...) {
    exitFailure = std::current_exception();
  }
  ui_.reset();
  audioEngine_.reset();
  closeApp();
  if (exitFailure) {
    std::rethrow_exception(exitFailure);
  }
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
    selected->setConfigOption("Full Screen",
                              options_.fullscreen ? "Yes" : "No");
  }
  if (config.find("VSync") != config.end()) {
    selected->setConfigOption("VSync", "No");
  }
  if (const auto videoMode = config.find("Video Mode");
      videoMode != config.end()) {
    const std::string requestedDimensions =
        std::to_string(options_.windowWidth) + "x" +
        std::to_string(options_.windowHeight);
    const auto matching = std::find_if(
        videoMode->second.possibleValues.begin(),
        videoMode->second.possibleValues.end(),
        [&requestedDimensions](const Ogre::String &value) {
          std::string compact;
          std::copy_if(value.begin(), value.end(),
                       std::back_inserter(compact),
                       [](unsigned char character) {
                         return !std::isspace(character);
                       });
          return compact.rfind(requestedDimensions, 0) == 0;
        });
    if (matching == videoMode->second.possibleValues.end()) {
      throw std::runtime_error("Requested resolution " +
                               requestedDimensions +
                               " is unavailable for renderer '" +
                               selected->getName() + "'");
    }
    selected->setConfigOption("Video Mode", *matching);
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
  resources.addResourceLocation((media / "MyGUI_Media").string(), "FileSystem",
                                "Run3MyGUI");
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
  camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(
      options_.verticalFovDegrees)));
  cameraNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
  cameraNode_->setPosition(0.0F, 75.0F, 300.0F);
  cameraNode_->lookAt(Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD);
  cameraNode_->attachObject(camera_);
  Ogre::Viewport *viewport = getRenderWindow()->addViewport(camera_);
  viewport->setBackgroundColour(Ogre::ColourValue(0.04F, 0.06F, 0.1F));
  updateAspectRatio();
  ui_ = ui::createMyGuiUiSystem(
      *getRenderWindow(), *sceneManager_, options_.paths.logDir(),
      [this](const ui::MenuAction &action) { handleMenuAction(action); },
      options_.uiScale);
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 display: " + std::to_string(options_.windowWidth) + "x" +
      std::to_string(options_.windowHeight) +
      (options_.fullscreen ? " fullscreen" : " windowed") +
      ", vertical FOV=" + std::to_string(options_.verticalFovDegrees));
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 content quality: textures=" + options_.textureQuality +
      " models=" + options_.modelQuality + " scenes=" +
      options_.mapQuality + " profile=" + options_.resourceProfile);

  const audio::AudioEngineConfig audioConfig{32, false};
  if (options_.audioBackend == "null") {
    audioEngine_ = audio::createNullAudioEngine(audioConfig);
  } else {
    audioEngine_ = audio::createAudioEngineWithFallback(
        audioConfig, audio::createMiniaudioEngine,
        [](const std::string_view message) {
          Ogre::LogManager::getSingleton().logMessage(std::string(message));
        });
  }
  Ogre::LogManager::getSingleton().logMessage(
      "Run3 audio backend: " + std::string(audioEngine_->backendName()) +
      (audioEngine_->hasOutputDevice() ? " (output device ready)"
                                       : " (no output device)"));

  if (!options_.mapName.empty()) {
    loadMap(options_.mapName);
  }

  ui_->showMenu(options_.mapName.empty());
  if (options_.introEnabled) {
    Ogre::LogManager::getSingleton().logMessage(
        "Intro video requested, but DirectShow/WMV is retired from the portable "
        "runtime; continuing immediately (intro is disabled by default)");
  }

  refreshMouseCapture();

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

void Run3App::loadMap(const std::string &mapName) {
    options_.mapName = mapName;
    physicsWorld_ = std::make_unique<physics::PhysicsWorld>(
        physics::createBulletPhysicsWorld());
    staticMap_ = std::make_unique<gameplay::StaticMap>(*sceneManager_,
                                                       *physicsWorld_);
    const gameplay::StaticMapStats mapStats = staticMap_->load(
        {&options_.paths, options_.mapName, options_.mapQuality,
         options_.resourceProfile, options_.textureQuality,
         meshLodBias(options_.modelQuality)});
    static_cast<void>(mapStats);
    gameplay::PlayerConfig playerConfig;
    playerConfig.standingHeight = options_.playerHeightCm;
    playerConfig.crouchingHeight = options_.playerHeightCm * (11.0 / 18.0);
    playerConfig.eyeOffset = options_.playerHeightCm * (5.0 / 12.0);
    playerConfig.crouchingEyeOffset =
        playerConfig.crouchingHeight * (5.0 / 22.0);
    player_ = std::make_unique<gameplay::PlayerController>(*physicsWorld_,
                                                           playerConfig);
    player_->spawn(staticMap_->spawnPosition());
    player_->setNoclip(options_.startNoclip);
    physicsDebug_ = options_.physicsDebug;
    staticMap_->setDebugDraw(physicsDebug_);
    camera_->setNearClipDistance(5.0F);
    camera_->setFarClipDistance(100000.0F);
    camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(
        options_.verticalFovDegrees)));

    try {
      audio::MapAudioLoadResult loaded = audio::loadLegacyMapAudio(
          options_.paths.contentRoot(), options_.mapName, options_.mapQuality);
      for (const std::string &warning : loaded.warnings) {
        Ogre::LogManager::getSingleton().logMessage("Map audio: " + warning);
      }
      mapAudio_ = std::make_unique<audio::MapAudioRuntime>(*audioEngine_);
      const audio::MapAudioStartResult started =
          mapAudio_->start(std::move(loaded.definition));
      Ogre::LogManager::getSingleton().logMessage(
          "Map audio: ambient=" + std::to_string(started.ambientStarted) +
          " failed=" + std::to_string(started.ambientFailed) +
          " script-controlled-deferred=" +
          std::to_string(loaded.scriptControlledSounds) + " music=" +
          (started.musicStarted ? "started" : "not started"));
      if (started.ambientFailed != 0 ||
          (!started.musicStarted && !audioEngine_->lastError().empty())) {
        Ogre::LogManager::getSingleton().logMessage(
            "Map audio backend detail: " + audioEngine_->lastError());
      }
    } catch (const std::exception &error) {
      Ogre::LogManager::getSingleton().logMessage(
          "Map audio disabled: " + std::string(error.what()));
    }

    sequenceServices_ = std::make_unique<gameplay::OgreSequenceServices>(
        options_.paths, *sceneManager_, *camera_, *physicsWorld_, *staticMap_,
        *player_, *audioEngine_, *ui_,
        [this](std::string map) { requestMapChange(std::move(map)); },
        meshLodBias(options_.modelQuality), options_.verticalFovDegrees);
    if (mapAudio_) {
      sequenceServices_->attachMapAudio(*mapAudio_);
    }
    sequenceRuntime_ = std::make_unique<gameplay::SequenceRuntime>(
        staticMap_->definition(), staticMap_->registry(), *sequenceServices_);
    sequenceServices_->attach(*sequenceRuntime_);
    npcPhysicsQuery_ = std::make_unique<physics::WorldPhysicsQuery>(*physicsWorld_);
    npcSystem_ = std::make_unique<gameplay::NpcSystem>(
        staticMap_->definition(), staticMap_->registry(), *npcPhysicsQuery_,
        *sequenceServices_);
    sequenceServices_->attachNpcSystem(*npcSystem_);
    npcSystem_->start();
    sequenceRuntime_->start();
}

void Run3App::unloadMap(const bool runOnExit) {
  std::exception_ptr failure;
  if (sequenceRuntime_) {
    try { sequenceRuntime_->unload(runOnExit); }
    catch (...) { if (!failure) failure = std::current_exception(); }
  }
  if (npcSystem_) {
    try { npcSystem_->unload(); }
    catch (...) { if (!failure) failure = std::current_exception(); }
  }
  if (sequenceServices_) {
    try {
      // Also covers partial load failures where no SequenceRuntime reached
      // start()/unload() but the presentation root was already allocated.
      sequenceServices_->submit(gameplay::DestroyRuntimeEntities{});
    } catch (...) {
      if (!failure) failure = std::current_exception();
    }
    const gameplay::OgreSequenceResourceCounts resources =
        sequenceServices_->resourceCounts();
    if (!resources.empty() && !failure) {
      failure = std::make_exception_ptr(std::runtime_error(
          "map teardown retained Sequence resources: presentations=" +
          std::to_string(resources.presentations) + " parts=" +
          std::to_string(resources.visualParts) + " particles=" +
          std::to_string(resources.particles) + " physics=" +
          std::to_string(resources.physicsBindings) + " audio=" +
          std::to_string(resources.audioHandles) + " attachments=" +
          std::to_string(resources.attachments) + " ragdolls=" +
          std::to_string(resources.ragdolls) + " root=" +
          (resources.rootNode ? "1" : "0")));
    }
  }
  npcSystem_.reset();
  npcPhysicsQuery_.reset();
  sequenceRuntime_.reset();
  sequenceServices_.reset();
  mapAudio_.reset();
  player_.reset();
  if (staticMap_) {
    staticMap_->unload();
    const gameplay::StaticMapResourceCounts resources =
        staticMap_->resourceCounts();
    if (!resources.empty() && !failure) {
      failure = std::make_exception_ptr(std::runtime_error(
          "map teardown retained StaticMap resources: entities=" +
          std::to_string(resources.entities) + " particles=" +
          std::to_string(resources.particles) + " physics=" +
          std::to_string(resources.physicsBodies) + " root=" +
          (resources.rootNode ? "1" : "0")));
    }
  }
  staticMap_.reset();
  physicsWorld_.reset();
  if (ui_) ui_->resetMapState();
  if (failure) std::rethrow_exception(failure);
}

void Run3App::requestMapChange(std::string mapName) {
  if (mapName.empty()) {
    Ogre::LogManager::getSingleton().logMessage(
        "Step 8E ignored empty map transition");
    return;
  }
  pendingMapChange_ = std::move(mapName);
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
  if (gameplayMouseCapture_) {
    setWindowGrab(window->isActive());
  }
  InputEvent event;
  event.type = window->isActive() ? InputEventType::FocusGained
                                  : InputEventType::FocusLost;
  input_.push(std::move(event));
}

void Run3App::setGameplayMouseCapture(const bool enabled) {
  if (getRenderWindow() == nullptr) {
    return;
  }
  setWindowGrab(enabled);
  gameplayMouseCapture_ = enabled;
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
    if (event.type == InputEventType::Quit) {
      requestQuit();
      continue;
    }
    const bool escape = event.type == InputEventType::KeyPressed &&
                        !event.repeated && event.key == Key::Escape;
    if (ui_ && ui_->computerActive() && !escape) {
      static_cast<void>(ui_->handleInput(event));
      if (sequenceRuntime_) static_cast<void>(sequenceRuntime_->handleInput(event));
      refreshMouseCapture();
      continue;
    }
    if (sequenceRuntime_ && sequenceRuntime_->handleInput(event)) {
      refreshMouseCapture();
      continue;
    }
    if (escape) {
      if (ui_) ui_->showMenu(!ui_->menuVisible());
      else requestQuit();
      refreshMouseCapture();
      continue;
    }
    if (ui_ && ui_->handleInput(event)) {
      refreshMouseCapture();
      continue;
    }
    if (player_ && event.type == InputEventType::MouseMoved &&
        !(sequenceRuntime_ && sequenceRuntime_->presentation().playerFrozen)) {
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
      bool handled = false;
      if (hit && sequenceServices_ && sequenceRuntime_) {
        const auto handle = sequenceServices_->handleForPhysicsEntity(
            hit->metadata.entityId);
        handled = handle && (sequenceRuntime_->interact(*handle) ||
                             (npcSystem_ && npcSystem_->use(*handle)));
      }
      Ogre::LogManager::getSingleton().logMessage(
          handled ? "Use activated sequence entity"
          : hit ? "Use ray hit body " + std::to_string(hit->body)
                : "Use ray missed");
    } else if (player_ && event.type == InputEventType::MousePressed &&
               event.mouseButton == MouseButton::Left) {
      const auto hit = player_->weaponRaycast(pitchRadians_);
      if (hit && sequenceServices_ && npcSystem_) {
        const auto handle = sequenceServices_->handleForPhysicsEntity(
            hit->metadata.entityId);
        if (handle) static_cast<void>(npcSystem_->damage(*handle, 25.0,
                                                         hit->point));
      }
      Ogre::LogManager::getSingleton().logMessage(
          hit ? "Weapon ray hit body " + std::to_string(hit->body)
              : "Weapon ray missed");
    }
    refreshMouseCapture();
  }
}

void Run3App::handleMenuAction(const ui::MenuAction &action) {
  switch (action.kind) {
  case ui::MenuActionKind::Resume:
    ui_->showMenu(false);
    break;
  case ui::MenuActionKind::NewGame:
    ui_->showMenu(false);
    requestMapChange(options_.newGameMap);
    break;
  case ui::MenuActionKind::SelectChapter:
    if (action.chapter.empty()) {
      ui_->appendConsole("Chapter name must not be empty");
      ui_->setConsoleVisible(true);
    } else {
      ui_->showMenu(false);
      requestMapChange(action.chapter);
    }
    break;
  case ui::MenuActionKind::ApplySettings: {
    if (action.verticalFov < 35.0 || action.verticalFov > 120.0) {
      ui_->appendConsole("FOV must be between 35 and 120 degrees");
      ui_->setConsoleVisible(true);
      break;
    }
    const std::size_t separator = action.resolution.find_first_of("xX");
    try {
      if (separator == std::string::npos) throw std::invalid_argument("format");
      const unsigned width = static_cast<unsigned>(
          std::stoul(action.resolution.substr(0, separator)));
      const unsigned height = static_cast<unsigned>(
          std::stoul(action.resolution.substr(separator + 1)));
      if (width < 640 || height < 480 || width > 16384 || height > 16384)
        throw std::out_of_range("range");
      options_.windowWidth = width;
      options_.windowHeight = height;
      options_.verticalFovDegrees = action.verticalFov;
      camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(action.verticalFov)));
      getRenderWindow()->resize(width, height);
      updateAspectRatio();
      ui_->appendConsole("Settings applied for this session");
    } catch (...) {
      ui_->appendConsole("Resolution must be WIDTHxHEIGHT (minimum 640x480)");
      ui_->setConsoleVisible(true);
    }
    break;
  }
  case ui::MenuActionKind::Quit:
    requestQuit();
    break;
  }
  refreshMouseCapture();
}

void Run3App::refreshMouseCapture() {
  const bool capture = player_ != nullptr && options_.frameLimit == 0 &&
                       ui_ != nullptr && !ui_->menuVisible() &&
                       !ui_->computerActive();
  if (capture != gameplayMouseCapture_) setGameplayMouseCapture(capture);
}

void Run3App::requestQuit() {
  quitRequested_ = true;
  if (mRoot != nullptr) {
    mRoot->queueEndRendering();
  }
}

} // namespace run3
