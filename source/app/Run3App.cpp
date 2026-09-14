#include <run3/app/Run3App.hpp>

#include <run3/core/Log.hpp>
#include <run3/content/AssetValidation.hpp>
#include <run3/content/OgreAssetValidation.hpp>

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
    return Run3AppOptions{AppPaths::resolve(executable), {}, 0, false, false,
                          {}, {}, {}};
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
  return Run3AppOptions{
      std::move(paths), configuration.valueOr("renderer", defaultRenderer),
      frames, contentRoot.has_value(), validateContent,
      configuredPath(std::move(manifestPath), executableDir),
      configuredPath(std::move(reportPath), executableDir),
      installedShare / "validation-fixture" / "step5.scene"};
}

void printRun3AppUsage() {
  std::cout
      << "Usage: run3_shell [--renderer d3d11|gl3plus] [--frames N]"
         " [--user-dir PATH] [--content-root PATH]\n"
      << "       [--validate-content] [--manifest PATH] [--report PATH]\n"
      << "Precedence: command line > user config > content defaults.\n"
      << "Relative paths are resolved from the executable directory.\n";
}

Run3App::Run3App(Run3AppOptions options)
    : OgreBites::ApplicationContext("Run3 renderer shell"),
      options_(std::move(options)), inputAdapter_(input_),
      clock_(ClockMode::Variable) {}

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
      const ClockFrame frame = clock_.tick();
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
    closeApp();
    throw;
  }
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
  Ogre::Entity *cube =
      sceneManager_->createEntity("Run3ShellCube", Ogre::SceneManager::PT_CUBE);
  cubeNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
  cubeNode_->attachObject(cube);

  camera_ = sceneManager_->createCamera("Run3ShellCamera");
  camera_->setNearClipDistance(5.0F);
  Ogre::SceneNode *cameraNode =
      sceneManager_->getRootSceneNode()->createChildSceneNode();
  cameraNode->setPosition(0.0F, 75.0F, 300.0F);
  cameraNode->lookAt(Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD);
  cameraNode->attachObject(camera_);
  Ogre::Viewport *viewport = getRenderWindow()->addViewport(camera_);
  viewport->setBackgroundColour(Ogre::ColourValue(0.04F, 0.06F, 0.1F));
  updateAspectRatio();

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
