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
                                      const std::string &quality,
                                      const fs::path &resolvedMapDirectory) {
  MapAudioLoadResult result;
  const std::string map = safeMapName(mapName);
  const fs::path mapDirectory = resolvedMapDirectory.empty()
      ? contentRoot / "run3" / "maps" / quality / map
      : resolvedMapDirectory;
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
    AmbientSoundDefinition sound;
    if (scriptName != values.end() && !scriptName->second.empty()) {
      sound.logicalName = scriptName->second;
      ++result.scriptControlledSounds;
    }
    sound.file = contentRoot / "run3" / "sounds" / file->second;
    sound.position = {number(values, "x", 0.0F) * multiplier,
                      number(values, "y", 0.0F) * multiplier,
                      number(values, "z", 0.0F) * multiplier};
    sound.minDistance = std::max(
        0.0F, number(values, "distance", 1.0F) * multiplier);
    sound.maxDistance =
        std::max(sound.minDistance,
                 number(values, "maxDistance", 1000.0F) * multiplier);
    sound.gain = std::max(0.0F, number(values, "maxGain", 1.0F));
    sound.minimumGain = std::clamp(number(values, "minGain", 0.0F),
                                   0.0F, 1.0F);
    sound.loop = boolean(values, "loop", true);
    warnIfMissing(result, sound.file);
    if (sound.logicalName.empty()) {
      result.definition.ambientSounds.push_back(std::move(sound));
    } else {
      result.definition.namedAmbientSounds.push_back(std::move(sound));
    }
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

MapAudioRuntime::MapAudioRuntime(IAudioEngine &engine,
                                 const bool useAudiereMusic)
    : engine_(engine), music_(engine), oneShots_(engine) {
  if (useAudiereMusic)
    audiereMusic_ = std::make_unique<AudiereMusicPlayer>();
}

MapAudioRuntime::~MapAudioRuntime() { clear(); }

MapAudioStartResult MapAudioRuntime::start(MapAudioDefinition definition) {
  clear();
  definition_ = std::move(definition);
  MapAudioStartResult result;
  // Ambient declarations describe virtual sounds, not permanently occupied
  // mixer voices. A voice is allocated only while its attenuation sphere is
  // relevant to the listener and released after the listener leaves it.
  const std::size_t ambientCount = definition_.ambientSounds.size() +
                                   definition_.namedAmbientSounds.size();
  ambient_.resize(ambientCount);
  ambientTriggered_.assign(ambientCount, false);
  ambientEnabled_.assign(ambientCount, true);
  ambientPlaybackSeconds_.assign(ambientCount, 0.0F);
  if (audiereMusic_) audiereMusic_->setVolume(definition_.musicGain * 10.0F);
  else music_.setVolume(definition_.musicGain * 10.0F);
  return result;
}

bool MapAudioRuntime::startPendingMusic() {
  musicStartAllowed_ = true;
  if (definition_.musicFile.empty()) return false;
  const bool started = audiereMusic_
      ? audiereMusic_->play(definition_.musicFile, definition_.musicLoop, 0.0F)
      : music_.play(definition_.musicFile, definition_.musicLoop, 0.0F);
  if (started && pendingMusicSeek_) {
    static_cast<void>(audiereMusic_
        ? audiereMusic_->seekSeconds(*pendingMusicSeek_)
        : music_.seekSeconds(*pendingMusicSeek_));
    pendingMusicSeek_.reset();
  }
  return started;
}

bool MapAudioRuntime::setNamedAmbientEnabled(std::string_view name,
                                             bool enabled) {
  for (std::size_t index = 0; index < definition_.namedAmbientSounds.size(); ++index) {
    const AmbientSoundDefinition &sound = definition_.namedAmbientSounds[index];
    if (sound.logicalName != name) continue;
    const std::size_t runtimeIndex = definition_.ambientSounds.size() + index;
    ambientEnabled_[runtimeIndex] = enabled;
    if (!enabled) {
      if (ambient_[runtimeIndex].valid())
        ambientPlaybackSeconds_[runtimeIndex] =
            engine_.playbackSeconds(ambient_[runtimeIndex]);
      ambient_[runtimeIndex].reset();
    } else {
      // Match the legacy runtime: enabling makes the named declaration
      // eligible immediately; updateAmbient decides whether it is in range.
      ambientTriggered_[runtimeIndex] = false;
    }
    return true;
  }
  return false;
}

bool MapAudioRuntime::playMusic(const fs::path &file, bool loop,
                                const float transitionSeconds) {
  if (!musicStartAllowed_) {
    definition_.musicFile = file;
    definition_.musicLoop = loop;
    pendingMusicSeek_.reset();
    return !file.empty();
  }
  const bool musicActive = audiereMusic_ ? audiereMusic_->active() : music_.active();
  if (file == definition_.musicFile && musicActive &&
      loop == definition_.musicLoop) {
    return true;
  }
  const bool started = audiereMusic_
      ? audiereMusic_->play(file, loop, transitionSeconds)
      : music_.play(file, loop, transitionSeconds);
  if (started) {
    definition_.musicFile = file;
    definition_.musicLoop = loop;
  }
  return started;
}

void MapAudioRuntime::stopMusic(float fadeSeconds) {
  if (audiereMusic_) audiereMusic_->stop(fadeSeconds);
  else music_.stop(fadeSeconds);
  definition_.musicFile.clear();
  pendingMusicSeek_.reset();
}

void MapAudioRuntime::setMusicVolume(float gain) {
  if (audiereMusic_) audiereMusic_->setVolume(gain * 10.0F);
  else music_.setVolume(gain * 10.0F);
  definition_.musicGain = gain;
}

void MapAudioRuntime::setMusicPitch(const float pitch) {
  if (audiereMusic_) audiereMusic_->setPitch(pitch);
  else music_.setPitch(pitch);
}

const fs::path &MapAudioRuntime::musicFile() const noexcept {
  return definition_.musicFile;
}

std::string MapAudioRuntime::musicLastError() const {
  return audiereMusic_ ? audiereMusic_->lastError() : engine_.lastError();
}

std::optional<float> MapAudioRuntime::musicPlaybackSeconds() const {
  if (!musicStartAllowed_ && !definition_.musicFile.empty()) return 0.0F;
  return audiereMusic_ ? audiereMusic_->playbackSeconds()
                       : music_.playbackSeconds();
}

bool MapAudioRuntime::seekMusicSeconds(const float seconds) {
  if (!musicStartAllowed_ && !definition_.musicFile.empty()) {
    pendingMusicSeek_ = std::max(0.0F, seconds);
    return true;
  }
  return audiereMusic_ ? audiereMusic_->seekSeconds(seconds)
                       : music_.seekSeconds(seconds);
}

void MapAudioRuntime::update(const float seconds, const FootstepState *player) {
  if (audiereMusic_) audiereMusic_->update(seconds);
  else music_.update(seconds);
  oneShots_.update(seconds);
  if (player != nullptr) {
    updateAmbient(player->position);
    updateFootsteps(seconds, *player);
  } else {
    footstepTimer_ = 0.0F;
  }
}

void MapAudioRuntime::clear() noexcept {
  oneShots_.clear();
  if (audiereMusic_) audiereMusic_->clear();
  else music_.clear();
  ambient_.clear();
  ambientTriggered_.clear();
  ambientEnabled_.clear();
  ambientPlaybackSeconds_.clear();
  definition_ = {};
  footstepTimer_ = 0.0F;
  nextFootstep_ = 0;
  footstepCount_ = 0;
  musicStartAllowed_ = false;
  pendingMusicSeek_.reset();
}

void MapAudioRuntime::updateAmbient(const Vec3 listenerPosition) {
  struct Candidate {
    std::size_t index{};
    float distanceSquared{};
  };
  std::vector<Candidate> candidates;
  candidates.reserve(ambient_.size());
  const auto definitionAt = [this](const std::size_t index)
      -> const AmbientSoundDefinition & {
    if (index < definition_.ambientSounds.size())
      return definition_.ambientSounds[index];
    return definition_.namedAmbientSounds[
        index - definition_.ambientSounds.size()];
  };
  for (std::size_t index = 0; index < ambient_.size(); ++index) {
    const AmbientSoundDefinition &sound = definitionAt(index);
    if (!ambientEnabled_[index]) {
      if (ambient_[index].valid())
        ambientPlaybackSeconds_[index] =
            engine_.playbackSeconds(ambient_[index]);
      ambient_[index].reset();
      continue;
    }
    const float x = listenerPosition.x - sound.position.x;
    const float y = listenerPosition.y - sound.position.y;
    const float z = listenerPosition.z - sound.position.z;
    const float distanceSquared = x * x + y * y + z * z;
    const float enterDistance = std::max(sound.minDistance, sound.maxDistance);
    // A small exit margin prevents voice churn while standing on an authored
    // maxDistance boundary.
    const float exitDistance = enterDistance * 1.1F;
    const bool relevant = ambient_[index].valid()
        ? distanceSquared <= exitDistance * exitDistance
        : distanceSquared <= enterDistance * enterDistance;
    if (!relevant) {
      if (ambient_[index].valid())
        ambientPlaybackSeconds_[index] =
            engine_.playbackSeconds(ambient_[index]);
      ambient_[index].reset();
      ambientTriggered_[index] = false;
      continue;
    }
    if (ambient_[index].valid() &&
        engine_.state(ambient_[index]) == SoundState::stopped) {
      ambient_[index].reset();
    }
    candidates.push_back({index, distanceSquared});
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const Candidate &left, const Candidate &right) {
              return left.distanceSquared < right.distanceSquared;
            });
  const AudioStats stats = engine_.stats();
  const std::size_t activeAmbient = ambientCount();
  const std::size_t nonAmbient = stats.activeVoices > activeAmbient
      ? stats.activeVoices - activeAmbient : 0;
  // Keep a small part of the bounded pool available for globally important
  // music, dialogue, footsteps and scripted effects. The remaining slots are
  // dynamically assigned to the nearest audible authored ambience.
  const std::size_t reserve = stats.voiceCapacity > 4 ? 4 :
                              stats.voiceCapacity > 0 ? 1 : 0;
  const std::size_t unavailable = std::max(nonAmbient, reserve);
  const std::size_t budget = stats.voiceCapacity > unavailable
      ? stats.voiceCapacity - unavailable : 0;
  for (std::size_t ordinal = budget; ordinal < candidates.size(); ++ordinal) {
    const std::size_t index = candidates[ordinal].index;
    if (ambient_[index].valid())
      ambientPlaybackSeconds_[index] = engine_.playbackSeconds(ambient_[index]);
    ambient_[index].reset();
  }
  for (std::size_t ordinal = 0;
       ordinal < std::min(budget, candidates.size()); ++ordinal) {
    const std::size_t index = candidates[ordinal].index;
    const AmbientSoundDefinition &sound = definitionAt(index);
    if (ambient_[index].valid() || (!sound.loop && ambientTriggered_[index]))
      continue;
    PlayOptions options;
    options.file = sound.file;
    options.bus = Bus::effects;
    options.loop = sound.loop;
    options.spatial = true;
    options.gain = sound.gain;
    options.position = sound.position;
    options.minDistance = sound.minDistance;
    options.maxDistance = sound.maxDistance;
    options.minimumGain = sound.minimumGain;
    ambient_[index] = engine_.play(options);
    if (ambient_[index].valid()) {
      if (sound.loop && ambientPlaybackSeconds_[index] > 0.0F)
        static_cast<void>(engine_.seekSeconds(
            ambient_[index], ambientPlaybackSeconds_[index]));
      ambientTriggered_[index] = true;
    }
  }
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
