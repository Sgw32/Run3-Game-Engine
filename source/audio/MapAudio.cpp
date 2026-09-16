#include <run3/audio/MapAudio.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace run3::audio {
namespace fs = std::filesystem;

namespace {

using Attributes = std::unordered_map<std::string, std::string>;

std::string readText(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("cannot read map audio input: " + path.string());
  }
  std::ostringstream text;
  text << input.rdbuf();
  return text.str();
}

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string safeMapName(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  if (value == "tlwhome2") {
    value = "tlwhome02";
  }
  if (value.empty() ||
      !std::all_of(value.begin(), value.end(), [](const unsigned char c) {
        return std::isalnum(c) != 0 || c == '_';
      })) {
    throw std::invalid_argument("map name is not a safe content directory name");
  }
  return value;
}

Attributes attributes(const std::string &text) {
  static const std::regex expression(
      R"ATTR(([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s/>]+)))ATTR");
  Attributes result;
  for (std::sregex_iterator it(text.begin(), text.end(), expression), end;
       it != end; ++it) {
    std::string value = (*it)[2].matched ? (*it)[2].str()
                        : (*it)[3].matched ? (*it)[3].str()
                                           : (*it)[4].str();
    result.emplace((*it)[1].str(), std::move(value));
  }
  return result;
}

float number(const Attributes &values, const char *name, const float fallback) {
  const auto found = values.find(name);
  if (found == values.end()) {
    return fallback;
  }
  std::size_t consumed{};
  const float result = std::stof(found->second, &consumed);
  if (consumed != found->second.size() || !std::isfinite(result)) {
    throw std::runtime_error("invalid numeric audio attribute '" +
                             std::string(name) + "'");
  }
  return result;
}

bool boolean(const Attributes &values, const char *name, const bool fallback) {
  const auto found = values.find(name);
  if (found == values.end()) {
    return fallback;
  }
  return found->second == "true" || found->second == "1" ||
         found->second == "yes";
}

std::string configValue(const fs::path &path, const std::string &wanted) {
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    line = trim(std::move(line));
    const std::string prefix = wanted + "=";
    if (line.rfind(prefix, 0) == 0) {
      return trim(line.substr(prefix.size()));
    }
  }
  return {};
}

void warnIfMissing(MapAudioLoadResult &result, const fs::path &path) {
  std::error_code error;
  if (!fs::is_regular_file(path, error)) {
    result.warnings.push_back("missing audio file: " + path.string());
  }
}

void readStartupMusic(MapAudioLoadResult &result, const fs::path &contentRoot,
                      const std::string &map, const std::string &quality) {
  const fs::path chapter = contentRoot / "run3" / "lua" / "chapters" / map;
  fs::path startup = chapter / (quality == "low" ? "startup_low.lua"
                                                  : "startup.lua");
  if (!fs::is_regular_file(startup)) {
    startup = chapter / "startup.lua";
  }
  if (!fs::is_regular_file(startup)) {
    return;
  }

  static const std::regex playExpression(
      R"LUA(playMusic\s*\(\s*"([^"]+)"\s*,\s*"?(true|false)"?\s*\))LUA");
  static const std::regex volumeExpression(
      R"LUA(setMusicVolume\s*\(\s*"?([0-9]+(?:\.[0-9]+)?)"?\s*\))LUA");
  std::ifstream input(startup);
  std::string line;
  while (std::getline(input, line)) {
    line = trim(std::move(line));
    if (line.empty() || line.rfind("--", 0) == 0) {
      continue;
    }
    std::smatch match;
    if (std::regex_search(line, match, playExpression)) {
      result.definition.musicFile = contentRoot / fs::path(match[1].str());
      result.definition.musicLoop = match[2].str() == "true";
    } else if (std::regex_search(line, match, volumeExpression)) {
      result.definition.musicGain = std::max(0.0F, std::stof(match[1].str()));
    }
  }
  if (!result.definition.musicFile.empty()) {
    warnIfMissing(result, result.definition.musicFile);
  }
}

} // namespace

MapAudioLoadResult loadLegacyMapAudio(const fs::path &contentRoot,
                                      const std::string &mapName,
                                      const std::string &quality) {
  MapAudioLoadResult result;
  const std::string map = safeMapName(mapName);
  const fs::path mapDirectory =
      contentRoot / "run3" / "maps" / quality / map;
  const fs::path sceneConfig = mapDirectory / "scene.cfg";
  const std::string sceneName = configValue(sceneConfig, "Scene");
  if (sceneName.empty()) {
    throw std::runtime_error("scene.cfg has no Scene entry: " +
                             sceneConfig.string());
  }
  result.sceneFile = mapDirectory / sceneName;
  const std::string scene = readText(result.sceneFile);

  float multiplier = 1.0F;
  static const std::regex sceneExpression(R"(<\s*scene\b([^>]*)>)",
                                          std::regex::icase);
  std::smatch sceneMatch;
  if (std::regex_search(scene, sceneMatch, sceneExpression)) {
    multiplier = number(attributes(sceneMatch[1].str()), "multiplier", 1.0F);
  }

  static const std::regex ambientExpression(R"(<\s*ambient\b([^>]*)>)",
                                            std::regex::icase);
  for (std::sregex_iterator it(scene.begin(), scene.end(), ambientExpression),
       end;
       it != end; ++it) {
    const Attributes values = attributes((*it)[1].str());
    const auto file = values.find("name");
    if (file == values.end() || file->second.empty()) {
      result.warnings.push_back("ambient declaration has no name attribute");
      continue;
    }
    const auto scriptName = values.find("objname");
    if (scriptName != values.end() && !scriptName->second.empty()) {
      ++result.scriptControlledSounds;
      continue;
    }

    AmbientSoundDefinition sound;
    sound.file = contentRoot / "run3" / "sounds" / file->second;
    sound.position = {number(values, "x", 0.0F) * multiplier,
                      number(values, "y", 0.0F) * multiplier,
                      number(values, "z", 0.0F) * multiplier};
    sound.minDistance = std::max(0.0F, number(values, "distance", 1.0F));
    sound.maxDistance =
        std::max(sound.minDistance, number(values, "maxDistance", 1000.0F));
    sound.gain = std::max(0.0F, number(values, "maxGain", 1.0F));
    sound.loop = boolean(values, "loop", true);
    warnIfMissing(result, sound.file);
    result.definition.ambientSounds.push_back(std::move(sound));
  }

  readStartupMusic(result, contentRoot, map, quality);

  // Trigger-driven surface selection belongs to Step 8. tlwcao's known
  // compatibility default is concrete, matching its legacy trigger and the
  // four authored samples used by Player.cpp.
  if (map == "tlwcao") {
    for (int index = 1; index <= 4; ++index) {
      fs::path footstep = contentRoot / "run3" / "sounds" / "footsteps" /
                          ("concrete" + std::to_string(index) + ".wav");
      warnIfMissing(result, footstep);
      result.definition.footsteps.push_back(std::move(footstep));
    }
  }
  return result;
}

MapAudioRuntime::MapAudioRuntime(IAudioEngine &engine)
    : engine_(engine), music_(engine), oneShots_(engine) {}

MapAudioRuntime::~MapAudioRuntime() { clear(); }

MapAudioStartResult MapAudioRuntime::start(MapAudioDefinition definition) {
  clear();
  definition_ = std::move(definition);
  MapAudioStartResult result;
  ambient_.reserve(definition_.ambientSounds.size());
  for (const AmbientSoundDefinition &sound : definition_.ambientSounds) {
    PlayOptions options;
    options.file = sound.file;
    options.bus = Bus::effects;
    options.loop = sound.loop;
    options.spatial = true;
    options.gain = sound.gain;
    options.position = sound.position;
    options.minDistance = sound.minDistance;
    options.maxDistance = sound.maxDistance;
    SoundHandle handle = engine_.play(options);
    if (handle.valid()) {
      ambient_.push_back(std::move(handle));
      ++result.ambientStarted;
    } else {
      ++result.ambientFailed;
    }
  }
  if (!definition_.musicFile.empty()) {
    music_.setVolume(definition_.musicGain);
    result.musicStarted =
        music_.play(definition_.musicFile, definition_.musicLoop, 0.25F);
  }
  return result;
}

void MapAudioRuntime::update(const float seconds, const FootstepState *player) {
  music_.update(seconds);
  oneShots_.update(seconds);
  if (player != nullptr) {
    updateFootsteps(seconds, *player);
  } else {
    footstepTimer_ = 0.0F;
  }
}

void MapAudioRuntime::clear() noexcept {
  oneShots_.clear();
  music_.clear();
  ambient_.clear();
  definition_ = {};
  footstepTimer_ = 0.0F;
  nextFootstep_ = 0;
  footstepCount_ = 0;
}

void MapAudioRuntime::updateFootsteps(const float seconds,
                                      const FootstepState &player) {
  const float speed = std::sqrt(player.velocity.x * player.velocity.x +
                                player.velocity.z * player.velocity.z);
  if (!player.grounded || player.noclip || speed < 20.0F ||
      definition_.footsteps.empty()) {
    footstepTimer_ = 0.0F;
    return;
  }
  footstepTimer_ += std::max(0.0F, seconds);
  const float interval = speed >= 400.0F ? 0.32F : 0.48F;
  while (footstepTimer_ >= interval) {
    footstepTimer_ -= interval;
    const fs::path &file =
        definition_.footsteps[nextFootstep_ % definition_.footsteps.size()];
    ++nextFootstep_;
    if (oneShots_.emit3D(file, 1.25F, player.position, 50.0F, 1200.0F,
                         false, Bus::effects, 0.3F)) {
      ++footstepCount_;
    }
  }
}

} // namespace run3::audio
