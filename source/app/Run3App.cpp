#include <run3/app/Run3App.hpp>

#include <run3/audio/Audio.hpp>
#include <run3/audio/MapAudio.hpp>
#include <run3/core/Log.hpp>
#include <run3/content/AssetValidation.hpp>
#include <run3/content/OgreAssetValidation.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/OgreSequenceServices.hpp>
#include <run3/gameplay/SequenceRuntime.hpp>
#include <run3/gameplay/SemiInfiniteWorld.hpp>
#include <run3/gameplay/StaticMap.hpp>
#include <run3/physics/Physics.hpp>
#include <run3/rendering/Run3SceneManager.hpp>
#include <run3/ui/OgreMyGui.hpp>
#include <run3/ui/Ui.hpp>

#include <OgreCamera.h>
#include <OgreColourValue.h>
#include <OgreEntity.h>
#include <OgreException.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgreOverlaySystem.h>
#include <OgrePanelOverlayElement.h>
#include <OgrePass.h>
#include <OgreRenderSystem.h>
#include <OgreRenderQueue.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreTextureUnitState.h>
#include <OgreViewport.h>
#include <OgreWindowEventUtilities.h>

#include <SDL.h>

#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
#include <OgreRTShaderSystem.h>
#endif

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <cmath>
#include <utility>

namespace run3 {
namespace fs = std::filesystem;

namespace {

void showSystemCursor(const bool visible) noexcept {
  // OgreBites toggles SDL relative mode but does not hide SDL's native
  // cursor on Windows/Linux. Run3 always renders its own MyGUI pointer while
  // a UI context is active, so the OS pointer must stay hidden at runtime.
  static_cast<void>(SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE));
}

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

const content::ConfigEntry *mapConfig(const content::MapDefinition &definition,
                                      const std::string_view key) {
  const auto found = std::find_if(
      definition.config.begin(), definition.config.end(),
      [key](const content::ConfigEntry &entry) { return entry.key == key; });
  return found == definition.config.end() ? nullptr : &*found;
}

std::string trimCopy(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

struct LoadingScreenSettings {
  std::string material;
  std::array<float, 3> barColour{0.12F, 0.0F, 0.0F};
};

std::array<float, 3> parseLoadingColour(std::string value) {
  value = trimCopy(std::move(value));
  if (value.size() == 7 && value.front() == '#') {
    const auto channel = [&value](const std::size_t offset) {
      std::size_t consumed{};
      const int parsed = std::stoi(value.substr(offset, 2), &consumed, 16);
      if (consumed != 2) throw std::invalid_argument("invalid hex channel");
      return static_cast<float>(parsed) / 255.0F;
    };
    return {channel(1), channel(3), channel(5)};
  }
  std::replace(value.begin(), value.end(), ',', ' ');
  std::istringstream stream(value);
  std::array<float, 3> result{};
  if (!(stream >> result[0] >> result[1] >> result[2]))
    throw std::invalid_argument("expected R G B or #RRGGBB");
  std::string trailing;
  if (stream >> trailing)
    throw std::invalid_argument("unexpected value after blue channel");
  const float maximum = std::max({result[0], result[1], result[2]});
  if (maximum > 1.0F) {
    for (float &channel : result) channel /= 255.0F;
  }
  if (std::any_of(result.begin(), result.end(), [](const float channel) {
        return !std::isfinite(channel) || channel < 0.0F || channel > 1.0F;
      }))
    throw std::invalid_argument("colour channels must be 0..1 or 0..255");
  return result;
}

LoadingScreenSettings readLoadingScreenSettings(
    const AppPaths &paths, const std::string &mapName,
    const std::string &quality) {
  LoadingScreenSettings result;
  const fs::path configPath = paths.contentPath(
      fs::path("run3") / "maps" / quality / mapName / "scene.cfg");
  std::ifstream stream(configPath, std::ios::binary);
  if (!stream) return result;
  std::string line;
  while (std::getline(stream, line)) {
    line = trimCopy(std::move(line));
    if (line.empty() || line.front() == '#' || line.front() == ';') continue;
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) continue;
    const std::string key = trimCopy(line.substr(0, equals));
    const std::string value = trimCopy(line.substr(equals + 1));
    if (key == "LoadingMat") result.material = value;
    if (key == "LoadingBarColor" || key == "LoadingBarColour") {
      try {
        result.barColour = parseLoadingColour(value);
      } catch (const std::exception &error) {
        Ogre::LogManager::getSingleton().logMessage(
            "Loading screen: invalid " + key + " in " +
            configPath.generic_string() + ": " + error.what() +
            "; using dark red");
      }
    }
  }
  return result;
}

std::optional<std::string> loadingTextureForMaterial(
    const fs::path &loadingRoot, const std::string_view materialName) {
  if (materialName.empty() || !fs::is_directory(loadingRoot)) return std::nullopt;
  std::error_code iteratorError;
  for (fs::recursive_directory_iterator iterator(
           loadingRoot, fs::directory_options::skip_permission_denied,
           iteratorError), end;
       iterator != end; iterator.increment(iteratorError)) {
    if (iteratorError) {
      iteratorError.clear();
      continue;
    }
    if (!iterator->is_regular_file() ||
        lower(iterator->path().extension().string()) != ".material")
      continue;

    std::ifstream stream(iterator->path(), std::ios::binary);
    if (!stream) continue;
    bool selected = false;
    bool opened = false;
    int depth = 0;
    std::string line;
    while (std::getline(stream, line)) {
      if (const std::size_t comment = line.find("//");
          comment != std::string::npos)
        line.erase(comment);
      const std::string trimmed = trimCopy(line);
      if (!selected) {
        std::istringstream tokens(trimmed);
        std::string keyword;
        std::string candidate;
        if (tokens >> keyword >> candidate;
            keyword == "material" && candidate == materialName)
          selected = true;
      }
      if (!selected) continue;

      depth += static_cast<int>(std::count(line.begin(), line.end(), '{'));
      if (depth > 0) opened = true;
      depth -= static_cast<int>(std::count(line.begin(), line.end(), '}'));

      std::istringstream tokens(trimmed);
      std::string keyword;
      std::string texture;
      if (tokens >> keyword >> texture; keyword == "texture") {
        if (!texture.empty() && texture.front() == '"' &&
            texture.back() == '"' && texture.size() >= 2)
          texture = texture.substr(1, texture.size() - 2);
        return texture;
      }
      if (opened && depth <= 0) break;
    }
  }
  return std::nullopt;
}

class LoadingScreen final {
public:
  LoadingScreen(Ogre::Root &root, Ogre::RenderWindow &window,
                const AppPaths &paths)
      : root_(&root), window_(&window), paths_(&paths) {}
  ~LoadingScreen() { dismiss(); }

  void show(const LoadingScreenSettings &settings) noexcept {
    dismiss();
    try {
      auto &resources = Ogre::ResourceGroupManager::getSingleton();
      if (resources.resourceGroupExists(resourceGroup))
        resources.destroyResourceGroup(resourceGroup);
      resources.createResourceGroup(resourceGroup);
      const fs::path loadingRoot =
          paths_->contentPath("run3/game/loadingscrs");
      if (fs::is_directory(loadingRoot)) {
        resources.addResourceLocation(loadingRoot.string(), "FileSystem",
                                      resourceGroup, false, true);
      }

      auto &materials = Ogre::MaterialManager::getSingleton();
      Ogre::MaterialPtr backgroundMaterial = materials.create(
          backgroundMaterialName, resourceGroup);
      Ogre::Pass *backgroundPass =
          backgroundMaterial->getTechnique(0)->getPass(0);
      backgroundPass->setLightingEnabled(false);
      backgroundPass->setDepthCheckEnabled(false);
      backgroundPass->setDepthWriteEnabled(false);
      backgroundPass->setDiffuse(0.015F, 0.015F, 0.015F, 1.0F);
      backgroundPass->setSelfIllumination(0.015F, 0.015F, 0.015F);
      const std::optional<std::string> loadingTexture =
          loadingTextureForMaterial(loadingRoot, settings.material);
      if (loadingTexture) {
        Ogre::TextureUnitState *textureUnit =
            backgroundPass->createTextureUnitState(*loadingTexture);
        textureUnit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
      } else if (!settings.material.empty()) {
        Ogre::LogManager::getSingleton().logMessage(
            "Loading screen: material '" + settings.material +
            "' has no resolvable texture; using a dark fallback");
      }
      Ogre::MaterialPtr barMaterial = materials.create(barMaterialName,
                                                       resourceGroup);
      Ogre::Pass *barPass = barMaterial->getTechnique(0)->getPass(0);
      barPass->setLightingEnabled(false);
      barPass->setDepthCheckEnabled(false);
      barPass->setDepthWriteEnabled(false);
      const auto &colour = settings.barColour;
      barPass->setDiffuse(colour[0], colour[1], colour[2], 1.0F);
      barPass->setSelfIllumination(colour[0], colour[1], colour[2]);

      Ogre::OverlayManager &manager = Ogre::OverlayManager::getSingleton();
      overlay_ = manager.create(overlayName);
      overlay_->setZOrder(650);
      background_ = static_cast<Ogre::PanelOverlayElement *>(
          manager.createOverlayElement("Panel", backgroundElementName));
      background_->setMetricsMode(Ogre::GMM_RELATIVE);
      background_->setPosition(0.0F, 0.0F);
      background_->setDimensions(1.0F, 1.0F);
      background_->setMaterialName(backgroundMaterial->getName(),
                                   resourceGroup);
      bar_ = static_cast<Ogre::PanelOverlayElement *>(
          manager.createOverlayElement("Panel", barElementName));
      bar_->setMetricsMode(Ogre::GMM_RELATIVE);
      bar_->setPosition(0.0F, 0.85F);
      bar_->setDimensions(0.001F, 0.05F);
      bar_->setMaterialName(barMaterialName, resourceGroup);
      background_->addChild(bar_);
      overlay_->add2D(background_);
      overlay_->show();
      if (window_->getNumViewports() != 0) {
        viewport_ = window_->getViewport(0);
        previousVisibilityMask_ = viewport_->getVisibilityMask();
        viewport_->setVisibilityMask(0);
      }
      visible_ = true;
      Ogre::LogManager::getSingleton().logMessage(
          "Loading screen: material=" +
          (settings.material.empty() ? std::string("<fallback>") :
                                       settings.material) +
          " texture=" +
          (loadingTexture ? *loadingTexture : std::string("<fallback>")));
      update(0.01F, true);
    } catch (const std::exception &error) {
      Ogre::LogManager::getSingleton().logMessage(
          "Loading screen disabled: " + std::string(error.what()));
      dismiss();
    }
  }

  void update(float progress, const bool force = false) noexcept {
    if (!visible_ || bar_ == nullptr) return;
    progress = std::clamp(progress, 0.0F, 1.0F);
    if (!force && progress < lastRenderedProgress_ + 0.015F) return;
    try {
      bar_->setWidth(std::max(0.001F, progress));
      Ogre::WindowEventUtilities::messagePump();
      root_->renderOneFrame(0.0F);
      lastRenderedProgress_ = progress;
    } catch (const Ogre::Exception &error) {
      Ogre::LogManager::getSingleton().logMessage(
          "Loading screen update failed: " + error.getDescription());
      dismiss();
    } catch (...) {
      dismiss();
    }
  }

  void dismiss() noexcept {
    if (viewport_ != nullptr) {
      try { viewport_->setVisibilityMask(previousVisibilityMask_); } catch (...) {}
      viewport_ = nullptr;
    }
    try {
      Ogre::OverlayManager &manager = Ogre::OverlayManager::getSingleton();
      if (overlay_ != nullptr) {
        overlay_->hide();
        if (background_ != nullptr) overlay_->remove2D(background_);
        manager.destroy(overlay_);
      }
      overlay_ = nullptr;
      if (background_ != nullptr && bar_ != nullptr)
        background_->removeChild(bar_->getName());
      if (bar_ != nullptr) manager.destroyOverlayElement(bar_);
      bar_ = nullptr;
      if (background_ != nullptr) manager.destroyOverlayElement(background_);
      background_ = nullptr;
      auto &resources = Ogre::ResourceGroupManager::getSingleton();
      if (resources.resourceGroupExists(resourceGroup))
        resources.destroyResourceGroup(resourceGroup);
    } catch (...) {}
    visible_ = false;
    lastRenderedProgress_ = -1.0F;
  }

private:
  inline static const Ogre::String resourceGroup{"Run3LoadingScreen"};
  inline static const Ogre::String overlayName{"Run3/LoadingScreen"};
  inline static const Ogre::String backgroundElementName{
      "Run3/LoadingScreen/Background"};
  inline static const Ogre::String barElementName{"Run3/LoadingScreen/Bar"};
  inline static const Ogre::String backgroundMaterialName{
      "Run3/LoadingScreen/Fallback"};
  inline static const Ogre::String barMaterialName{
      "Run3/LoadingScreen/BarMaterial"};
  Ogre::Root *root_{};
  Ogre::RenderWindow *window_{};
  const AppPaths *paths_{};
  Ogre::Viewport *viewport_{};
  std::uint32_t previousVisibilityMask_{0xffffffffU};
  Ogre::Overlay *overlay_{};
  Ogre::PanelOverlayElement *background_{};
  Ogre::PanelOverlayElement *bar_{};
  float lastRenderedProgress_{-1.0F};
  bool visible_{};
};

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

void overlayConfig(ConfigValues &target, const ConfigValues &source) {
  for (const auto &[key, value] : source) target[key] = value;
}

std::vector<std::string> splitConsoleCommand(const std::string &command) {
  std::vector<std::string> result;
  std::string token;
  bool quoted = false;
  for (const char character : command) {
    if (character == '"') {
      quoted = !quoted;
    } else if (std::isspace(static_cast<unsigned char>(character)) && !quoted) {
      if (!token.empty()) { result.push_back(std::move(token)); token.clear(); }
    } else {
      token.push_back(character);
    }
  }
  if (quoted) throw std::invalid_argument("unterminated quote");
  if (!token.empty()) result.push_back(std::move(token));
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
  const fs::path installedShare =
      (executableDir / ".." / "share" / "run3").lexically_normal();
  const ConfigValues contentDefaults =
      Configuration::readFile(paths.contentPath("config/run3.cfg"));
  const ConfigValues userValues =
      Configuration::readFile(paths.configDir() / "run3.cfg");
  const Configuration bootstrapConfiguration = Configuration::merge(
      contentDefaults, userValues, commandLine.values);

  std::optional<fs::path> contentRoot = requestedContentRoot;
  std::optional<fs::path> userRoot = commandLine.userRoot;
  if (!contentRoot) {
    contentRoot = configuredRoot(bootstrapConfiguration, "content-root", executableDir);
  }
  if (!userRoot) {
    userRoot = configuredRoot(bootstrapConfiguration, "user-root", executableDir);
  }
  if (contentRoot || userRoot) {
    paths = AppPaths::resolve(executable, contentRoot, userRoot);
  }

  const auto variant = bootstrapConfiguration.valueOr("content.variant", "original");
  if (const auto overlay = bootstrapConfiguration.find("content.overlay"))
    paths.setContentOverlay(configuredPath(*overlay, executableDir));
  else if (variant == "nextgen")
    paths.setContentOverlay(paths.userPath("derived-content/nextgen"));
  else if (variant == "tlwrm")
    paths.setContentOverlay(paths.userPath("derived-content/tlwrm"));
  else if (variant == "luniverse")
    paths.setContentOverlay(paths.userPath("derived-content/luniverse"));

  ConfigValues defaults = Configuration::readFile(installedShare / "config" /
                                                   "performance.cfg");
  overlayConfig(defaults, Configuration::readFile(installedShare / "config" /
                                                  "player.cfg"));
  overlayConfig(defaults, Configuration::readFile(paths.contentRoot() /
                                                  "config" / "run3.cfg"));
  if (!paths.contentOverlay().empty()) {
    overlayConfig(defaults, Configuration::readFile(
        paths.contentOverlay() / "run3" / "core" / "performance.cfg"));
    overlayConfig(defaults, Configuration::readFile(
        paths.contentOverlay() / "run3" / "core" / "player.cfg"));
  }
  const Configuration configuration = Configuration::merge(
      defaults, Configuration::readFile(paths.configDir() / "run3.cfg"),
      commandLine.values);

#ifdef _WIN32
  const std::string defaultRenderer = "d3d11";
#else
  const std::string defaultRenderer = "gl3plus";
#endif
  std::uint64_t frames = configuration.unsignedOr("frames", 0);
  if (validateContent && frames == 0) {
    frames = 1;
  }
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
  options.lighting.pipeline = rendering::parseLightingPipeline(
      configuration.valueOr("render.lighting_pipeline", "legacy-forward"));
  options.lighting.shadows = rendering::parseShadowQuality(
      configuration.valueOr("render.shadow_quality", "off"));
  options.lighting.exposure = static_cast<float>(
      configuredDouble(configuration, "render.exposure", 1.0));
  if (options.lighting.exposure <= 0 || options.lighting.exposure > 32)
    throw std::runtime_error("render.exposure must be in (0,32]");
  options.lighting.directionalShadowDistances = {
      static_cast<float>(configuredDouble(
          configuration, "render.shadow_cascade_1_distance", 6000.0)),
      static_cast<float>(configuredDouble(
          configuration, "render.shadow_cascade_2_distance", 100000.0)),
      static_cast<float>(configuredDouble(
          configuration, "render.shadow_cascade_3_distance", 500000.0))};
  const auto &shadowDistances = options.lighting.directionalShadowDistances;
  if (shadowDistances[0] <= 5.0F ||
      shadowDistances[0] >= shadowDistances[1] ||
      shadowDistances[1] >= shadowDistances[2]) {
    throw std::runtime_error(
        "directional shadow cascade distances must be ordered, with "
        "cascade 1 greater than the 5-unit camera near plane");
  }
  const bool forwardPipeline =
      options.lighting.pipeline == rendering::LightingPipeline::LegacyForward ||
      options.lighting.pipeline == rendering::LightingPipeline::FastForward;
  const std::uint64_t shadowUpdateInterval = configuration.unsignedOr(
      "render.shadow_update_interval", forwardPipeline ? 2U : 1U);
  if (shadowUpdateInterval < 1 || shadowUpdateInterval > 8)
    throw std::runtime_error("shadow-update-interval must be in [1,8]");
  options.lighting.shadowUpdateInterval =
      static_cast<unsigned>(shadowUpdateInterval);
  options.lightingLab = configuredBool(configuration, "lighting-lab");
  options.glowLab = configuredBool(configuration, "glow-lab");
  options.glowLabNoEffect =
      configuredBool(configuration, "glow-lab-no-effect");
  options.lightingCapture = configuredBool(configuration, "lighting-capture");
  options.lightingReload = configuredBool(configuration, "lighting-reload");
  options.lightingResize = configuredBool(configuration, "lighting-resize");
  if ((options.lightingLab || options.glowLab) && !options.mapName.empty())
    throw std::runtime_error(
        "--lighting-lab/--glow-lab and --map are mutually exclusive");
  if (options.lightingLab && options.glowLab)
    throw std::runtime_error(
        "--lighting-lab and --glow-lab are mutually exclusive");
  if (options.glowLabNoEffect && !options.glowLab)
    throw std::runtime_error("--glow-lab-no-effect requires --glow-lab");
  if (variant != "original" && variant != "nextgen" && variant != "tlwrm" &&
      variant != "luniverse")
    throw std::runtime_error(
        "content.variant must be original, nextgen, tlwrm or luniverse");
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
  options.player.radius = configuredDouble(configuration, "player.radius", 20.0);
  options.player.standingHeight = configuredDouble(
      configuration, "player.standing-height", options.playerHeightCm);
  if (commandLine.values.find("player-height-cm") != commandLine.values.end())
    options.player.standingHeight = options.playerHeightCm;
  options.player.crouchingHeight = configuredDouble(
      configuration, "player.crouching-height",
      options.player.standingHeight * (11.0 / 18.0));
  options.player.eyeOffset = configuredDouble(
      configuration, "player.eye-offset",
      options.player.standingHeight * (5.0 / 12.0));
  options.player.crouchingEyeOffset = configuredDouble(
      configuration, "player.crouching-eye-offset",
      options.player.crouchingHeight * (5.0 / 22.0));
  options.player.massKg = configuredDouble(configuration, "player.mass", 40.0);
  options.player.walkSpeed = configuredDouble(configuration, "player.walk-speed", 300.0);
  options.player.runSpeed = configuredDouble(configuration, "player.run-speed", 520.0);
  options.player.noclipSpeed = configuredDouble(configuration, "player.noclip-speed", 650.0);
  options.player.jumpSpeed = configuredDouble(configuration, "player.jump-speed", 360.0);
  options.player.ladderSpeed = configuredDouble(configuration, "player.ladder-speed", 220.0);
  options.player.stepHeight = configuredDouble(configuration, "player.step-height", 35.0);
  options.player.groundProbe = configuredDouble(configuration, "player.ground-probe", 8.0);
  options.player.interactionDistance = configuredDouble(configuration, "player.use-distance", 250.0);
  options.player.weaponDistance = configuredDouble(configuration, "player.weapon-distance", 100000.0);
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
  options.devMode = configuredBool(configuration, "dev-mode");
  options.introEnabled = configuredBool(configuration, "intro", false);
  options.uiScale = static_cast<float>(
      configuredDouble(configuration, "ui-scale", 1.0));
  if (options.uiScale < 0.75F || options.uiScale > 3.0F)
    throw std::runtime_error("ui-scale must be between 0.75 and 3.0");
  options.mouseSensitivity =
      configuredDouble(configuration, "input.mouse-sensitivity", 1.0);
  if (options.mouseSensitivity < 0.1 || options.mouseSensitivity > 4.0)
    throw std::runtime_error(
        "input.mouse-sensitivity must be between 0.1 and 4.0");
  for (const auto &definition : inputActionDefinitions()) {
    const std::string configKey =
        "input.bind." + std::string(definition.id);
    const auto configured = configuration.find(configKey);
    if (!configured) continue;
    const auto key = parseKey(*configured);
    if (!key || !isBindableKey(*key))
      throw std::runtime_error("Configuration value for '" + configKey +
                               "' is not a bindable key: " + *configured);
    options.inputBindings.rebind(definition.action, *key);
  }
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
      << "       [--lighting-pipeline legacy-forward|deferred|pbr|fast-forward]\n"
      << "       [--shadow-quality off|low|medium|high|ultra] [--exposure N]\n"
      << "       [--shadow-update-interval 1..8]\n"
      << "       [--lighting-lab|--glow-lab] [--lighting-capture]\n"
      << "       [--content-variant original|nextgen|tlwrm|luniverse] [--content-overlay PATH]\n"
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
  ui_.reset();
  lighting_.reset();
  showSystemCursor(true);
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
      if (options_.lightingReload && renderedFrames_ == 3 && !options_.mapName.empty()) {
        const auto map=options_.mapName;
        unloadMap(false);
        loadMap(map);
        options_.lightingReload=false;
      }
      if(options_.lightingResize && options_.lightingLab && renderedFrames_==3)
        getRenderWindow()->resize(800,600);
      if(options_.lightingResize && options_.lightingLab && renderedFrames_==6)
        getRenderWindow()->resize(options_.windowWidth,options_.windowHeight);
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
        const bool uiBlocked = ui_ && (ui_->menuVisible() ||
                                       ui_->computerActive() ||
                                       ui_->inventoryVisible() ||
                                       ui_->consoleVisible());
        // Menus consume controls, but they do not stop the physical world. In
        // particular, a player riding a train must continue to follow it while
        // Escape is open. Only an authored sequence freeze changes the Bullet
        // body into a frozen collider.
        const bool physicallyFrozen = sequenceRuntime_ &&
            sequenceRuntime_->presentation().playerFrozen;
        const bool controlsBlocked = uiBlocked || physicallyFrozen;
        player_->setFrozen(physicallyFrozen);
        command.forward = controlsBlocked ? 0.0
            : (options_.inputBindings.down(state, InputAction::MoveForward) ? 1.0 : 0.0) -
              (options_.inputBindings.down(state, InputAction::MoveBackward) ? 1.0 : 0.0);
        command.strafe = controlsBlocked ? 0.0
            : (options_.inputBindings.down(state, InputAction::MoveRight) ? 1.0 : 0.0) -
              (options_.inputBindings.down(state, InputAction::MoveLeft) ? 1.0 : 0.0);
        command.run = !controlsBlocked &&
            options_.inputBindings.down(state, InputAction::Run);
        command.jump = !controlsBlocked &&
            options_.inputBindings.down(state, InputAction::Jump);
        command.crouch = !controlsBlocked &&
            options_.inputBindings.down(state, InputAction::Crouch);
        if (player_->state().noclip && !controlsBlocked) {
          command.vertical =
              (options_.inputBindings.down(state, InputAction::Jump) ? 1.0 : 0.0) -
                             (command.crouch ? 1.0 : 0.0);
          command.jump = false;
          command.crouch = false;
        }
        if (legacyNoclip_ && !controlsBlocked) {
          const double horizontal = std::cos(pitchRadians_);
          const physics::Vec3 forward{-std::sin(yawRadians_) * horizontal,
                                      std::sin(pitchRadians_),
                                      -std::cos(yawRadians_) * horizontal};
          // Match Ogre's camera-local +X direction.  The opposite Z sign
          // makes strafing depend on yaw and can even add a forward component.
          const physics::Vec3 right{std::cos(yawRadians_), 0.0,
                                    -std::sin(yawRadians_)};
          physics::Vec3 motion{
              right.x * command.strafe + forward.x * command.forward,
              forward.y * command.forward,
              right.z * command.strafe + forward.z * command.forward};
          motion.y += (options_.inputBindings.down(state, InputAction::Jump) ? 1.0 : 0.0) -
                      (command.crouch ? 1.0 : 0.0);
          const double length = std::sqrt(motion.x * motion.x + motion.y * motion.y +
                                          motion.z * motion.z);
          if (length > 1.0) {
            motion.x /= length; motion.y /= length; motion.z /= length;
          }
          const double distance = options_.player.noclipSpeed *
                                  frame.elapsed.count();
          detachedCameraPosition_.x += motion.x * distance;
          detachedCameraPosition_.y += motion.y * distance;
          detachedCameraPosition_.z += motion.z * distance;
          command = {};
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
          player_->updatePickedBody(pitchRadians_,
                                    physics::PhysicsWorld::fixedStepSeconds);
          static_cast<void>(player_->simulateFixedStep());
          ++simulatedSteps_;
        }
        if (staticMap_) {
          staticMap_->syncDynamicTransforms();
        }
        if (semiInfiniteWorld_ && player_ && physicsWorld_ && staticMap_) {
          physics::Vec3 playerPosition = player_->state().position;
          if (const auto shift =
                  semiInfiniteWorld_->requestedOriginShift(playerPosition)) {
            physicsWorld_->shiftOrigin(*shift);
            staticMap_->shiftVisualOrigin(*shift);
            player_->applyOriginShift(*shift);
            if (legacyNoclip_) {
              detachedCameraPosition_.x -= shift->x;
              detachedCameraPosition_.y -= shift->y;
              detachedCameraPosition_.z -= shift->z;
            }
            semiInfiniteWorld_->applyOriginShift(*shift);
            playerPosition = player_->state().position;
          }
          semiInfiniteWorld_->update(playerPosition);
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
        } else if (legacyNoclip_) {
          cameraNode_->setPosition(
              static_cast<Ogre::Real>(detachedCameraPosition_.x),
              static_cast<Ogre::Real>(detachedCameraPosition_.y),
              static_cast<Ogre::Real>(detachedCameraPosition_.z));
          cameraNode_->setOrientation(
              Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(yawRadians_)),
                               Ogre::Vector3::UNIT_Y) *
              Ogre::Quaternion(Ogre::Radian(static_cast<Ogre::Real>(pitchRadians_)),
                               Ogre::Vector3::UNIT_X));
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
          if (mapMusicStartFrame_ &&
              renderedFrames_ >= *mapMusicStartFrame_) {
            const bool appointed = !mapAudio_->musicFile().empty();
            const bool started = mapAudio_->startPendingMusic();
            Ogre::LogManager::getSingleton().logMessage(
                std::string("Map audio: third-frame music ") +
                (started ? "started" :
                 appointed ? "failed" : "not appointed"));
            if (appointed && !started && !audioEngine_->lastError().empty()) {
              Ogre::LogManager::getSingleton().logMessage(
                  "Map audio backend detail: " + audioEngine_->lastError());
            }
            mapMusicStartFrame_.reset();
          }
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
        if (debugOverlayPage_ != 0U) updateDebugOverlay();
        ui_->update(static_cast<float>(frame.elapsed.count()));
      }
      // UI state can also be changed by Lua during the fixed update, without
      // producing an input event. Keep capture and the gameplay crosshair in
      // sync before every rendered frame.
      refreshMouseCapture();
      updatePhysicsDebugDraw();
      if (lighting_) lighting_->update(static_cast<double>(renderedFrames_) / 60.0);
      // Native Ogre computer overlays are rendered into their isolated RTT
      // before the world samples it. The pass restores all main-view overlay
      // visibility before renderOneFrame().
      if (ui_) ui_->renderComputerSurface();
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
    lighting_.reset();
    audioEngine_.reset();
    showSystemCursor(true);
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
    if (options_.lightingCapture) {
      getRenderWindow()->writeContentsToFile(
          (options_.paths.logDir() / "lighting.png").string());
      lighting_->writeReport(options_.paths.logDir() / "lighting.json", *getRenderWindow());
    }
    unloadMap(true);
  } catch (...) {
    exitFailure = std::current_exception();
  }
  ui_.reset();
  lighting_.reset();
  audioEngine_.reset();
  showSystemCursor(true);
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
  // Preserve the texture policy used by the original Run3 runtime. Ogre 14
  // defaults to MIP_UNLIMITED, which makes it consume every embedded mip from
  // legacy DDS files. Some of those old authored chains (for example
  // box_wood_2.dds) become progressively darker in their smallest levels.
  // The original engine requested five mipmaps, and Ogre clamps custom image
  // mip chains to the requested count while still generating up to five for
  // images that do not contain their own levels.
  Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(0);

  Ogre::ResourceGroupManager &resources =
      Ogre::ResourceGroupManager::getSingleton();
  const fs::path media =
      (options_.paths.executableDir() / ".." / "share" / "run3" / "Media")
          .lexically_normal();
  resources.addResourceLocation((media / "Main").string(), "FileSystem",
                                Ogre::RGN_INTERNAL);
  resources.addResourceLocation((media / "RTShaderLib").string(), "FileSystem",
                                Ogre::RGN_INTERNAL);
  resources.addResourceLocation((media / "Lighting").string(), "FileSystem",
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
  sceneManagerFactory_ = std::make_unique<rendering::Run3SceneManagerFactory>(
      options_.lighting.shadowUpdateInterval);
  mRoot->addSceneManagerFactory(sceneManagerFactory_.get());
  sceneManager_ =
      mRoot->createSceneManager(sceneManagerFactory_->getTypeName());
  sceneManager_->addRenderQueueListener(mOverlaySystem);
#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
  mShaderGenerator->addSceneManager(sceneManager_);
  mShaderGenerator->setShaderCachePath(options_.paths.cacheDir().string());
#endif
  sceneManager_->setAmbientLight(Ogre::ColourValue(0.25F, 0.25F, 0.25F));

  // The shell light is a diagnostic fixture, never part of authored maps.
  if (options_.mapName.empty()) {
    Ogre::Light *light = sceneManager_->createLight("Run3ShellLight");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setCastShadows(true);
    Ogre::SceneNode *lightNode =
        sceneManager_->getRootSceneNode()->createChildSceneNode();
    lightNode->setDirection(Ogre::Vector3(-1.0F, -1.0F, -1.0F).normalisedCopy(),
                            Ogre::Node::TS_WORLD);
    lightNode->attachObject(light);
  }
  if (options_.mapName.empty() && !options_.lightingLab && !options_.glowLab) {
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
  lighting_ = std::make_unique<rendering::OgreLighting>(
      *sceneManager_, *camera_, *viewport, options_.lighting);
  initialisePhysicsDebugDraw();
  const fs::path compositorAssets =
      (options_.paths.executableDir() / ".." / "share" / "run3" / "Media" /
       "LegacyCompositors")
          .lexically_normal();
  lighting_->configureLegacyCompositors(options_.paths.contentRoot(),
                                        options_.paths.contentPath(
                                            "run3/shaders"),
                                        compositorAssets,
                                        options_.paths.cacheDir() /
                                            "legacy-compositors",
                                        options_.textureQuality);
  if (options_.lightingLab) lighting_->createLab();
  if (options_.glowLab) {
    lighting_->createGlowLab();
    if (!options_.glowLabNoEffect)
      lighting_->setCompositorEnabled("Glow", true);
  }
  ui_ = ui::createMyGuiUiSystem(
      *getRenderWindow(), *sceneManager_, *camera_, options_.paths.logDir(),
      [this](const ui::MenuAction &action) { handleMenuAction(action); },
      {std::to_string(options_.windowWidth) + "x" +
           std::to_string(options_.windowHeight),
       options_.verticalFovDegrees, options_.mouseSensitivity,
       options_.inputBindings, options_.devMode},
      options_.uiScale);
  ui_->setConsoleCommandHandler(
      [this](std::string command) { executeConsoleCommand(std::move(command)); });
  registerConsoleCommands();
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

  ui_->showMenu(options_.mapName.empty() && !options_.lightingLab &&
                !options_.glowLab);
  showSystemCursor(false);
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
    const LoadingScreenSettings loadingSettings = readLoadingScreenSettings(
        options_.paths, mapName, options_.mapQuality);
    LoadingScreen loadingScreen(*getRoot(), *getRenderWindow(), options_.paths);
    loadingScreen.show(loadingSettings);
    loadingScreen.update(0.03F, true);
    if (sceneManager_->hasLight("Run3ShellLight")) {
      Ogre::Light *shellLight = sceneManager_->getLight("Run3ShellLight");
      Ogre::SceneNode *node = shellLight->getParentSceneNode();
      shellLight->detachFromParent();
      sceneManager_->destroyLight(shellLight);
      if (node != nullptr)
        sceneManager_->destroySceneNode(node);
    }
    if (cubeNode_ != nullptr) {
      if (sceneManager_->hasEntity("Run3ShellCube"))
        sceneManager_->destroyEntity("Run3ShellCube");
      sceneManager_->destroySceneNode(cubeNode_);
      cubeNode_ = nullptr;
    }
    options_.mapName = mapName;
    physicsWorld_ = std::make_unique<physics::PhysicsWorld>(
        physics::createBulletPhysicsWorld());
    staticMap_ = std::make_unique<gameplay::StaticMap>(*sceneManager_,
                                                       *physicsWorld_);
    const gameplay::StaticMapStats mapStats = staticMap_->load(
        {&options_.paths, options_.mapName, options_.mapQuality,
         options_.resourceProfile, options_.textureQuality,
         meshLodBias(options_.modelQuality), options_.lighting,
         [&loadingScreen](const float progress) {
           loadingScreen.update(0.05F + progress * 0.63F);
         }});
    static_cast<void>(mapStats);
    loadingScreen.update(0.70F, true);
    player_ = std::make_unique<gameplay::PlayerController>(*physicsWorld_,
                                                           options_.player);
    player_->spawn(staticMap_->spawnPosition());
    player_->setNoclip(options_.startNoclip);
    loadingScreen.update(0.74F);
    if (const content::ConfigEntry *seedEntry =
            mapConfig(staticMap_->definition(), "WorldSeed")) {
      gameplay::SemiInfiniteWorldOptions worldOptions;
      const auto [pointer, error] = std::from_chars(
          seedEntry->value.data(),
          seedEntry->value.data() + seedEntry->value.size(),
          worldOptions.seed);
      if (error != std::errc{} ||
          pointer != seedEntry->value.data() + seedEntry->value.size()) {
        throw std::runtime_error(
            "WorldSeed must be an unsigned 64-bit integer");
      }
      const auto value = [this](const std::string_view key,
                                const std::string &fallback) {
        const content::ConfigEntry *entry =
            mapConfig(staticMap_->definition(), key);
        return entry == nullptr ? fallback : entry->value;
      };
      const auto boolean = [&value](const std::string_view key,
                                    const bool fallback) {
        const std::string parsed = lower(
            value(key, fallback ? std::string("true") : std::string("false")));
        if (parsed == "true" || parsed == "1" || parsed == "yes" ||
            parsed == "on")
          return true;
        if (parsed == "false" || parsed == "0" || parsed == "no" ||
            parsed == "off")
          return false;
        throw std::runtime_error("Map configuration value for '" +
                                 std::string(key) +
                                 "' is not a boolean: " + parsed);
      };
      worldOptions.manifest = options_.paths.contentPath(
          value("BlockManifest", "run3/bblocks/block_manifest.json"));
      worldOptions.originBlock = value("WorldOriginBlock", "");
      worldOptions.horizontalLoadRadiusCm =
          std::stod(value("WorldLoadRadius", "20000"));
      worldOptions.verticalLoadRadiusCm =
          std::stod(value("WorldVerticalRadius", "500"));
      worldOptions.rebaseThresholdCm =
          std::stod(value("WorldRebaseThreshold", "50000"));
      worldOptions.logicalExtentCm =
          std::stoll(value("WorldExtent", "100000000"));
      worldOptions.lightBudget = static_cast<unsigned>(
          std::stoul(value("WorldLightBudget", "3")));
      worldOptions.terminators = boolean("WorldTerminators", true);
      worldOptions.lighting = options_.lighting;
      semiInfiniteWorld_ = std::make_unique<gameplay::SemiInfiniteWorld>(
          *sceneManager_, *physicsWorld_, std::move(worldOptions));
      semiInfiniteWorld_->update(player_->state().position);
    }
    loadingScreen.update(0.78F);
    physicsDebug_ = options_.physicsDebug;
    staticMap_->setDebugDraw(physicsDebug_);
    if (semiInfiniteWorld_) semiInfiniteWorld_->setDebugDraw(physicsDebug_);
    camera_->setNearClipDistance(5.0F);
    const rendering::ShadowBudget mapShadowBudget = rendering::shadowBudget(
        options_.lighting.shadows, options_.lighting.pipeline);
    const float configuredShadowFar = mapShadowBudget.splits > 1
        ? options_.lighting.directionalShadowDistances[2]
        : mapShadowBudget.distance;
    camera_->setFarClipDistance(std::max(100000.0F, configuredShadowFar));
    camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(
        options_.verticalFovDegrees)));
    lighting_->configureMapShadows();
    loadingScreen.update(0.81F);

    try {
      audio::MapAudioLoadResult loaded = audio::loadLegacyMapAudio(
          options_.paths.contentRoot(), options_.mapName, options_.mapQuality,
          options_.paths.contentPath(std::filesystem::path("run3") / "maps" /
                                     options_.mapQuality / options_.mapName));
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
          (mapAudio_->musicPending() ? "appointed for frame 3" :
                                       "not appointed"));
      if (started.ambientFailed != 0 && !audioEngine_->lastError().empty()) {
        Ogre::LogManager::getSingleton().logMessage(
            "Map audio backend detail: " + audioEngine_->lastError());
      }
    } catch (const std::exception &error) {
      Ogre::LogManager::getSingleton().logMessage(
          "Map audio disabled: " + std::string(error.what()));
    }
    loadingScreen.update(0.86F);

    sequenceServices_ = std::make_unique<gameplay::OgreSequenceServices>(
        options_.paths, *sceneManager_, *camera_, *physicsWorld_, *staticMap_,
        *player_, *audioEngine_, *ui_,
        [this](std::string map) { requestMapChange(std::move(map)); },
        meshLodBias(options_.modelQuality), options_.verticalFovDegrees);
    sequenceServices_->attachLighting(*lighting_);
    if (mapAudio_) {
      sequenceServices_->attachMapAudio(*mapAudio_);
    }
    loadingScreen.update(0.90F);
    sequenceRuntime_ = std::make_unique<gameplay::SequenceRuntime>(
        staticMap_->definition(), staticMap_->registry(), *sequenceServices_);
    sequenceServices_->attach(*sequenceRuntime_);
    npcPhysicsQuery_ = std::make_unique<physics::WorldPhysicsQuery>(*physicsWorld_);
    npcSystem_ = std::make_unique<gameplay::NpcSystem>(
        staticMap_->definition(), staticMap_->registry(), *npcPhysicsQuery_,
        *sequenceServices_);
    sequenceServices_->attachNpcSystem(*npcSystem_);
    npcSystem_->start();
    // This must be the final loading-only render. Authored startup scripts can
    // enable viewport compositors (Bloom, ColorCorrection, Glow, ...), whose
    // scene passes must never run while the loading screen has temporarily
    // hidden the gameplay visibility mask. The first compositor render is the
    // normal gameplay frame after dismiss() restores that mask.
    loadingScreen.update(1.0F, true);
    sequenceRuntime_->start();
    // renderedFrames_ counts completed frames. At +2, the next call to
    // renderOneFrame is the third frame belonging to this newly loaded map.
    mapMusicStartFrame_ = renderedFrames_ + 2;
    // A map load is the gameplay-state boundary. Never carry the menu layer
    // or its pointer through a chapter transition or cutscene restoration.
    if (ui_) ui_->showMenu(false);
    loadingScreen.dismiss();
}

void Run3App::unloadMap(const bool runOnExit) {
  mapMusicStartFrame_.reset();
  legacyNoclip_ = false;
  if (player_) player_->releasePickedBody();
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
  // onExit scripts are allowed to change compositors. Clear the final state
  // only after all script-backed services have shut down.
  if (lighting_) lighting_->clearCompositorEffects();
  mapAudio_.reset();
  semiInfiniteWorld_.reset();
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
  showSystemCursor(false);
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

void Run3App::initialisePhysicsDebugDraw() {
  constexpr const char *materialName = "Run3/BulletBounds";
  auto &materials = Ogre::MaterialManager::getSingleton();
  Ogre::MaterialPtr material = materials.getByName(
      materialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  if (!material) {
    material = materials.create(
        materialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->removeAllTechniques();
    Ogre::Pass *sourcePass = material->createTechnique()->createPass();
    // RTSS reads vertex-colour tracking while it generates the technique.
    sourcePass->setVertexColourTracking(Ogre::TVC_DIFFUSE);
    rendering::MaterialDescription description;
    description.name = materialName;
    description.surface = rendering::Surface::Unlit;
    description.lighting = false;
    description.castShadows = false;
    description.receiveShadows = false;
    rendering::OgreLighting::configureMaterial(
        *material, description, options_.lighting);
    // Debug bounds must remain visible when they coincide with or sit inside
    // the rendered mesh. The legacy SceneNode bounding boxes behave the same
    // way, and depth testing made the Bullet overlay appear entirely absent.
    for (unsigned techniqueIndex = 0;
         techniqueIndex < material->getNumTechniques(); ++techniqueIndex) {
      Ogre::Technique *technique = material->getTechnique(techniqueIndex);
      for (unsigned passIndex = 0; passIndex < technique->getNumPasses();
           ++passIndex) {
        Ogre::Pass *pass = technique->getPass(passIndex);
        pass->setDepthCheckEnabled(false);
        pass->setDepthWriteEnabled(false);
      }
    }
    material->load();
  }
  physicsDebugObject_ =
      sceneManager_->createManualObject("Run3BulletBoundsObject");
  physicsDebugObject_->setDynamic(true);
  physicsDebugObject_->setCastShadows(false);
  // Authored compositors render the ordinary scene queues into intermediate
  // textures and may replace their output. Submit physics diagnostics with
  // Ogre's overlays so they are drawn after the compositor chain.
  physicsDebugObject_->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
  physicsDebugNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode(
      "Run3BulletBoundsNode");
  physicsDebugNode_->attachObject(physicsDebugObject_);
}

void Run3App::updatePhysicsDebugDraw() {
  if (physicsDebugObject_ == nullptr) return;
  physicsDebugObject_->clear();
  if (!physicsDebug_ || physicsWorld_ == nullptr) return;
  const auto lines = physicsWorld_->debugLines();
  if (lines.empty()) return;

  physicsDebugObject_->estimateVertexCount(
      static_cast<Ogre::uint32>(lines.size() * 2));
  physicsDebugObject_->begin("Run3/BulletBounds",
                             Ogre::RenderOperation::OT_LINE_LIST);
  for (const physics::PhysicsDebugLine &line : lines) {
    physicsDebugObject_->position(
        static_cast<Ogre::Real>(line.from.x),
        static_cast<Ogre::Real>(line.from.y),
        static_cast<Ogre::Real>(line.from.z));
    physicsDebugObject_->colour(
        static_cast<Ogre::Real>(line.colour.x),
        static_cast<Ogre::Real>(line.colour.y),
        static_cast<Ogre::Real>(line.colour.z));
    physicsDebugObject_->position(
        static_cast<Ogre::Real>(line.to.x),
        static_cast<Ogre::Real>(line.to.y),
        static_cast<Ogre::Real>(line.to.z));
    physicsDebugObject_->colour(
        static_cast<Ogre::Real>(line.colour.x),
        static_cast<Ogre::Real>(line.colour.y),
        static_cast<Ogre::Real>(line.colour.z));
  }
  physicsDebugObject_->end();
}

void Run3App::handleInput(const std::vector<InputEvent> &events) {
  for (const InputEvent &event : events) {
    if (event.type == InputEventType::Quit) {
      requestQuit();
      continue;
    }
    const bool keyPress = event.type == InputEventType::KeyPressed &&
                          !event.repeated;
    if (ui_ && ui_->consoleVisible()) {
      if (consoleToggleTextPending_ &&
          event.type == InputEventType::TextEntered) {
        consoleToggleTextPending_ = false;
        if (event.text == ".") continue;
      }
      if (keyPress && (event.key == Key::Escape || event.key == Key::Period)) {
        ui_->setConsoleVisible(false);
      } else {
        static_cast<void>(ui_->handleInput(event));
      }
      refreshMouseCapture();
      continue;
    }
    if (options_.devMode && keyPress && event.key == Key::Period && ui_) {
      ui_->setConsoleVisible(true);
      consoleToggleTextPending_ = true;
      refreshMouseCapture();
      continue;
    }
    if (options_.devMode && keyPress && event.key == Key::Slash && player_) {
      setLegacyNoclip(!legacyNoclip_);
      continue;
    }
    if (event.type == InputEventType::KeyPressed && !event.repeated &&
        event.key == Key::F5) {
           Ogre::LogManager::getSingleton().logMessage("Screenshot");
      captureScreenshot();
      continue;
    }
    if (event.type == InputEventType::KeyPressed && !event.repeated &&
        event.key == Key::P) {
      cycleDebugOverlay();
      continue;
    }
    if (event.type == InputEventType::KeyPressed && !event.repeated &&
        event.key == options_.inputBindings.key(InputAction::Inventory) &&
        ui_ && !ui_->computerActive() &&
        !ui_->menuVisible()) {
      if (ui_->inventoryVisible()) {
        ui_->closeInventory();
      } else if (sequenceRuntime_ &&
                 !sequenceRuntime_->presentation().playerFrozen &&
                 ui_->openInventory()) {
        try {
          static_cast<void>(sequenceRuntime_->dispatchScriptCall(
              {"world", "runScript", {"run3/lua/funcs/inventory.lua"}}));
        } catch (...) {
          ui_->closeInventory();
          throw;
        }
      }
      refreshMouseCapture();
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
    if (escape && ui_ && ui_->bindingCaptureActive() &&
        ui_->handleInput(event)) {
      refreshMouseCapture();
      continue;
    }
    if (escape) {
      if (ui_ && ui_->inventoryVisible()) ui_->closeInventory();
      else if (ui_) ui_->showMenu(!ui_->menuVisible());
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
      constexpr double baseSensitivity = 0.0025;
      const double sensitivity = baseSensitivity * options_.mouseSensitivity;
      yawRadians_ -= static_cast<double>(event.deltaX) * sensitivity;
      pitchRadians_ = std::clamp(
          pitchRadians_ - static_cast<double>(event.deltaY) * sensitivity,
          -1.553343034, 1.553343034);
    } else if (sequenceServices_ &&
               event.type == InputEventType::KeyPressed && !event.repeated &&
               event.key ==
                   options_.inputBindings.key(InputAction::Flashlight)) {
      sequenceServices_->toggleFlashlight();
    } else if (staticMap_ && event.type == InputEventType::KeyPressed &&
               !event.repeated && event.key == Key::F3) {
      physicsDebug_ = !physicsDebug_;
      staticMap_->setDebugDraw(physicsDebug_);
      if (semiInfiniteWorld_) semiInfiniteWorld_->setDebugDraw(physicsDebug_);
      updatePhysicsDebugDraw();
      const std::size_t bodyCount =
          physicsWorld_ != nullptr ? physicsWorld_->bodyCount() : 0;
      const std::size_t lineCount =
          physicsDebug_ && physicsWorld_ != nullptr
              ? physicsWorld_->debugLines().size()
              : 0;
      Ogre::LogManager::getSingleton().logMessage(
          std::string("Bullet bounds: ") +
          (physicsDebug_ ? "enabled" : "disabled") +
          ", bodies=" + std::to_string(bodyCount) +
          ", lines=" + std::to_string(lineCount));
    } else if (player_ && event.type == InputEventType::KeyPressed &&
               !event.repeated &&
               event.key == options_.inputBindings.key(InputAction::Use)) {
      const auto hit = player_->useRaycast(pitchRadians_);
      bool handled = false;
      gameplay::ObjectPickResult pickResult =
          gameplay::ObjectPickResult::Ignored;
      if (hit && sequenceServices_ && sequenceRuntime_) {
        const auto handle = sequenceServices_->handleForPhysicsEntity(
            hit->metadata.entityId);
        handled = handle && (sequenceRuntime_->interact(*handle) ||
                             (npcSystem_ && npcSystem_->use(*handle)));
      }
      if (hit && !handled) {
        pickResult = player_->togglePickedBody(*hit);
        handled = pickResult != gameplay::ObjectPickResult::Ignored;
      }
      Ogre::LogManager::getSingleton().logMessage(
          pickResult == gameplay::ObjectPickResult::Picked
              ? "Use picked physical object " + std::to_string(hit->body)
          : pickResult == gameplay::ObjectPickResult::Released
              ? "Use released physical object " + std::to_string(hit->body)
          : handled ? "Use activated sequence entity"
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

void Run3App::captureScreenshot() {
  try {
    const fs::path directory = options_.paths.userPath("screenshots");
    fs::create_directories(directory);
    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &timestamp);
#else
    localtime_r(&timestamp, &local);
#endif
    std::string map = options_.mapName.empty() ? "menu" : options_.mapName;
    for (char &character : map) {
      if (!std::isalnum(static_cast<unsigned char>(character)) &&
          character != '-' && character != '_') character = '_';
    }
    std::ostringstream name;
    name << "run3_" << map << '_' << std::put_time(&local, "%Y-%m-%d_%H-%M-%S")
         << '_' << std::setw(3) << std::setfill('0')
         << (std::chrono::duration_cast<std::chrono::milliseconds>(
                 now.time_since_epoch()).count() % 1000)
         << ".png";
    const fs::path output = directory / name.str();
    getRenderWindow()->writeContentsToFile(output.string());
    Ogre::LogManager::getSingleton().logMessage("Screenshot saved: " +
                                                 output.string());
    if (ui_) ui_->appendConsole("Screenshot saved: " + output.string());
  } catch (const std::exception &error) {
    Ogre::LogManager::getSingleton().logMessage(
        "Screenshot failed: " + std::string(error.what()));
  }
}

void Run3App::consoleLine(std::string line) {
  Ogre::LogManager::getSingleton().logMessage("Developer console: " + line);
  if (ui_) ui_->appendConsole(std::move(line));
}

void Run3App::setLegacyNoclip(const bool enabled) {
  if (!player_ || legacyNoclip_ == enabled) return;
  if (enabled && player_->state().noclip) player_->setNoclip(false);
  if (enabled && cameraNode_) {
    const Ogre::Vector3 position = cameraNode_->getPosition();
    detachedCameraPosition_ = {position.x, position.y, position.z};
  }
  legacyNoclip_ = enabled;
  consoleLine(std::string("noclip: ") + (enabled ? "on (camera detached)" : "off"));
}

void Run3App::executeConsoleCommand(std::string command) {
  consoleLine("> " + command);
  if (!options_.devMode) {
    consoleLine("Developer mode is disabled in Options.");
    return;
  }
  try {
    std::vector<std::string> words = splitConsoleCommand(command);
    if (words.empty()) return;
    const std::string name = lower(words.front());
    words.erase(words.begin());
    const auto found = consoleCommands_.find(name);
    if (found == consoleCommands_.end()) {
      consoleLine("Unknown command '" + name + "'. Type help.");
      return;
    }
    found->second(words);
  } catch (const std::exception &error) {
    consoleLine("Error: " + std::string(error.what()));
  }
}

void Run3App::registerConsoleCommands() {
  consoleCommands_.clear();
  const auto requestedState = [](const std::vector<std::string> &arguments,
                                 const bool current) {
    if (arguments.empty() || lower(arguments.front()) == "toggle") return !current;
    if (arguments.size() != 1) throw std::invalid_argument("expected on, off, or toggle");
    const std::string value = lower(arguments.front());
    if (value == "on" || value == "1" || value == "true") return true;
    if (value == "off" || value == "0" || value == "false") return false;
    throw std::invalid_argument("expected on, off, or toggle");
  };
  consoleCommands_["help"] = [this](const std::vector<std::string> &) {
    std::vector<std::string> names;
    names.reserve(consoleCommands_.size());
    for (const auto &[name, command] : consoleCommands_) {
      static_cast<void>(command); names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    std::string line = "Commands:";
    for (const auto &name : names) line += " " + name;
    consoleLine(std::move(line));
  };
  consoleCommands_["clear"] = [this](const std::vector<std::string> &) {
    if (ui_) ui_->clearConsole();
  };
  consoleCommands_["noclip"] = [this, requestedState](const auto &arguments) {
    setLegacyNoclip(requestedState(arguments, legacyNoclip_));
  };
  consoleCommands_["noclip_new"] = [this, requestedState](const auto &arguments) {
    if (!player_) throw std::runtime_error("no player is loaded");
    const bool enabled = requestedState(arguments, player_->state().noclip);
    if (enabled && legacyNoclip_) setLegacyNoclip(false);
    player_->setNoclip(enabled);
    consoleLine(std::string("noclip_new: ") + (enabled ? "on" : "off"));
  };
  consoleCommands_["map"] = [this](const auto &arguments) {
    if (arguments.size() != 1) throw std::invalid_argument("usage: map <name>");
    requestMapChange(arguments.front());
    if (ui_) ui_->setConsoleVisible(false);
  };
  consoleCommands_["fov"] = [this](const auto &arguments) {
    if (arguments.size() != 1) throw std::invalid_argument("usage: fov <35..120>");
    const double value = std::stod(arguments.front());
    if (value < 35.0 || value > 120.0) throw std::invalid_argument("FOV must be 35..120");
    options_.verticalFovDegrees = value;
    camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(value)));
    consoleLine("FOV set to " + std::to_string(value));
  };
  consoleCommands_["screenshot"] = [this](const std::vector<std::string> &) {
    captureScreenshot();
  };
  consoleCommands_["quit"] = [this](const std::vector<std::string> &) {
    requestQuit();
  };
}

void Run3App::cycleDebugOverlay() {
  debugOverlayPage_ = (debugOverlayPage_ + 1U) % 5U;
  updateDebugOverlay();
}

void Run3App::updateDebugOverlay() {
  if (!ui_) return;
  if (debugOverlayPage_ == 0U) {
    ui_->setDebugOverlay(false, {});
    return;
  }
  std::ostringstream out;
  out << std::fixed << std::setprecision(1);
  if (debugOverlayPage_ == 1U) {
    const auto &stats = getRenderWindow()->getStatistics();
    out << "PERFORMANCE  [P: next]\n"
        << "FPS  now " << stats.lastFPS << "  avg " << stats.avgFPS
        << "  best " << stats.bestFPS << "  worst " << stats.worstFPS << '\n'
        << "Frame " << (stats.lastFPS > 0.0F ? 1000.0F / stats.lastFPS : 0.0F)
        << " ms  best " << stats.bestFrameTime << " ms  worst "
        << stats.worstFrameTime << " ms\n"
        << "Batches " << stats.batchCount << "  triangles "
        << stats.triangleCount << "  vblank misses " << stats.vBlankMissCount
        << '\n' << "Viewport " << getRenderWindow()->getWidth() << 'x'
        << getRenderWindow()->getHeight() << "  rendered frames "
        << renderedFrames_;
  } else if (debugOverlayPage_ == 2U) {
    out << "PLAYER  [P: next]\nMap "
        << (options_.mapName.empty() ? "<none>" : options_.mapName)
        << "  quality " << options_.mapQuality << '\n';
    if (player_) {
      const auto state = player_->state();
      out << "Position " << state.position.x << ", " << state.position.y
          << ", " << state.position.z << '\n'
          << "Velocity " << state.velocity.x << ", " << state.velocity.y
          << ", " << state.velocity.z << '\n'
          << "Grounded " << (state.grounded ? "yes" : "no")
          << "  crouched " << (state.crouched ? "yes" : "no")
          << "  noclip " << (state.noclip ? "yes" : "no")
          << "  ladder " << (state.onLadder ? "yes" : "no") << '\n';
    }
    out << "Yaw " << yawRadians_ << "  pitch " << pitchRadians_
        << "  simulation steps " << simulatedSteps_;
    if (sequenceRuntime_) {
      const auto &presentation = sequenceRuntime_->presentation();
      out << "\nComputer "
          << (presentation.activeComputer.empty() ? "<none>"
                                                   : presentation.activeComputer)
          << "  cutscene "
          << (presentation.activeCutscene.empty() ? "<none>"
                                                  : presentation.activeCutscene);
    }
  } else if (debugOverlayPage_ == 3U) {
    out << "SOUND  [P: next]\n";
    if (audioEngine_) {
      const auto stats = audioEngine_->stats();
      out << "Backend " << audioEngine_->backendName() << "  device "
          << (audioEngine_->hasOutputDevice() ? "ready" : "unavailable") << '\n'
          << "Active voices " << stats.activeVoices << '/' << stats.voiceCapacity;
      for (const auto &voice : audioEngine_->activeVoices()) {
        out << "\n  " << voice.file.filename().string();
        if (voice.bus == audio::Bus::music) out << " [music]";
        else if (voice.bus == audio::Bus::voice) out << " [voice]";
        if (voice.spatial) out << " [3D]";
      }
    }
    if (mapAudio_) {
      out << "\nCurrent music "
          << (mapAudio_->musicActive() ? mapAudio_->musicFile().string()
                                       : "<none>")
          << "\nAmbient sounds " << mapAudio_->ambientCount()
          << "  footsteps played " << mapAudio_->footstepCount();
    }
    if (sequenceServices_)
      out << "\nSequence sound handles "
          << sequenceServices_->resourceCounts().audioHandles;
  } else {
    out << "NPC  [P: hide]\n";
    if (!npcSystem_) {
      out << "No NPC system loaded";
    } else {
      std::size_t alive{}, dead{}, navigating{}, blocked{};
      for (const auto &npc : npcSystem_->states()) {
        if (npc.state == gameplay::NpcState::Dead) ++dead; else ++alive;
        if (npc.state == gameplay::NpcState::Navigating) ++navigating;
        if (npc.state == gameplay::NpcState::Blocked) ++blocked;
      }
      out << "Total " << npcSystem_->size() << "  alive " << alive
          << "  dead " << dead << "  navigating " << navigating
          << "  blocked " << blocked;
      if (sequenceServices_) {
        const auto stats = sequenceServices_->npcPresentationStats();
        out << "\nAnimation full/throttled/paused "
            << stats.fullRateAnimations << '/' << stats.throttledAnimations
            << '/' << stats.pausedAnimations << "  shadow casters "
            << stats.shadowCasters << '/' << stats.total;
      }
      std::size_t shown{};
      for (const auto &npc : npcSystem_->states()) {
        if (shown++ == 8) break;
        out << '\n' << npc.name << "  hp " << npc.health << "  pos "
            << npc.transform.position.x << ',' << npc.transform.position.y
            << ',' << npc.transform.position.z;
      }
    }
  }
  ui_->setDebugOverlay(true, out.str());
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
    if (action.mouseSensitivity < 0.1 || action.mouseSensitivity > 4.0) {
      ui_->appendConsole("Mouse sensitivity must be between 0.1x and 4.0x");
      ui_->setConsoleVisible(true);
      break;
    }
    const std::size_t separator = action.resolution.find_first_of("xX");
    unsigned width{};
    unsigned height{};
    try {
      if (separator == std::string::npos) throw std::invalid_argument("format");
      width = static_cast<unsigned>(
          std::stoul(action.resolution.substr(0, separator)));
      height = static_cast<unsigned>(
          std::stoul(action.resolution.substr(separator + 1)));
      if (width < 640 || height < 480 || width > 16384 || height > 16384)
        throw std::out_of_range("range");
    } catch (...) {
      ui_->appendConsole("Resolution must be WIDTHxHEIGHT (minimum 640x480)");
      ui_->setConsoleVisible(true);
      break;
    }

    options_.windowWidth = width;
    options_.windowHeight = height;
    options_.verticalFovDegrees = action.verticalFov;
    options_.mouseSensitivity = action.mouseSensitivity;
    options_.inputBindings = action.bindings;
    options_.devMode = action.devMode;
    if (!options_.devMode) {
      setLegacyNoclip(false);
      if (player_ && player_->state().noclip) player_->setNoclip(false);
      ui_->setConsoleVisible(false);
    }
    camera_->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(action.verticalFov)));
    getRenderWindow()->resize(width, height);
    updateAspectRatio();

    try {
      const fs::path configPath = options_.paths.configDir() / "run3.cfg";
      ConfigValues userValues = Configuration::readFile(configPath);
      userValues["resolution"] = std::to_string(width) + "x" +
                                 std::to_string(height);
      userValues["fov"] = std::to_string(action.verticalFov);
      userValues["input.mouse-sensitivity"] =
          std::to_string(action.mouseSensitivity);
      userValues["dev-mode"] = action.devMode ? "true" : "false";
      for (const auto &definition : inputActionDefinitions()) {
        userValues["input.bind." + std::string(definition.id)] =
            std::string(keyName(action.bindings.key(definition.action)));
      }
      Configuration::writeFile(configPath, userValues);
      ui_->appendConsole("Settings applied and saved");
    } catch (const std::exception &error) {
      ui_->appendConsole("Settings applied but could not be saved: " +
                         std::string(error.what()));
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
  const bool uiObscuresGameplay = ui_ != nullptr &&
      (ui_->menuVisible() || ui_->computerActive() ||
       ui_->inventoryVisible() || ui_->consoleVisible());
  const bool cutsceneActive = sequenceRuntime_ != nullptr &&
      !sequenceRuntime_->presentation().activeCutscene.empty();
  if (sequenceServices_ != nullptr) {
    sequenceServices_->setGameplayCursorVisible(
        player_ != nullptr && !uiObscuresGameplay && !cutsceneActive);
  }
  const bool capture = player_ != nullptr && options_.frameLimit == 0 &&
                       ui_ != nullptr && !uiObscuresGameplay;
  if (capture != gameplayMouseCapture_) setGameplayMouseCapture(capture);
}

void Run3App::requestQuit() {
  quitRequested_ = true;
  if (mRoot != nullptr) {
    mRoot->queueEndRendering();
  }
}

} // namespace run3
