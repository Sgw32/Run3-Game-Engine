#pragma once

#include <run3/audio/Audio.hpp>
#include <run3/audio/MusicPlayer.hpp>
#include <run3/audio/SoundRuntime.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace run3::audio {

struct AmbientSoundDefinition {
  std::string logicalName;
  std::filesystem::path file;
  Vec3 position{};
  float minDistance{1.0F};
  float maxDistance{1000.0F};
  float gain{1.0F};
  bool loop{true};
};

struct MapAudioDefinition {
  std::vector<AmbientSoundDefinition> ambientSounds;
  std::vector<AmbientSoundDefinition> namedAmbientSounds;
  std::filesystem::path musicFile;
  bool musicLoop{};
  float musicGain{1.0F};
  std::vector<std::filesystem::path> footsteps;
};

struct MapAudioLoadResult {
  MapAudioDefinition definition;
  std::filesystem::path sceneFile;
  std::vector<std::string> warnings;
  std::size_t scriptControlledSounds{};
};

// Reads only static audio declarations. Lua execution and event-controlled
// named sounds remain Step 8 work; named ambient entries are counted but not
// started early.
[[nodiscard]] MapAudioLoadResult loadLegacyMapAudio(
    const std::filesystem::path &contentRoot, const std::string &mapName,
    const std::string &quality);

struct FootstepState {
  Vec3 position{};
  Vec3 velocity{};
  bool grounded{};
  bool noclip{};
};

struct MapAudioStartResult {
  std::size_t ambientStarted{};
  std::size_t ambientFailed{};
  bool musicStarted{};
};

class MapAudioRuntime final {
public:
  explicit MapAudioRuntime(IAudioEngine &engine);
  ~MapAudioRuntime();

  MapAudioRuntime(const MapAudioRuntime &) = delete;
  MapAudioRuntime &operator=(const MapAudioRuntime &) = delete;

  [[nodiscard]] MapAudioStartResult start(MapAudioDefinition definition);
  [[nodiscard]] bool setNamedAmbientEnabled(std::string_view name, bool enabled);
  [[nodiscard]] bool playMusic(const std::filesystem::path &file, bool loop);
  void stopMusic(float fadeSeconds = 0.0F);
  void setMusicVolume(float gain);
  [[nodiscard]] const std::filesystem::path &musicFile() const noexcept;
  void update(float seconds, const FootstepState *player = nullptr);
  void clear() noexcept;

  [[nodiscard]] std::size_t ambientCount() const noexcept {
    return ambient_.size();
  }
  [[nodiscard]] std::size_t footstepCount() const noexcept {
    return footstepCount_;
  }
  [[nodiscard]] bool musicActive() const noexcept { return music_.active(); }

private:
  void updateFootsteps(float seconds, const FootstepState &player);

  IAudioEngine &engine_;
  MusicPlayer music_;
  SoundRuntime oneShots_;
  MapAudioDefinition definition_;
  std::vector<SoundHandle> ambient_;
  std::vector<SoundHandle> namedAmbient_;
  float footstepTimer_{};
  std::size_t nextFootstep_{};
  std::size_t footstepCount_{};
};

} // namespace run3::audio
