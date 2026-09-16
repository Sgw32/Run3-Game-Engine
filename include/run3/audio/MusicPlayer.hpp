#pragma once

#include <run3/audio/Audio.hpp>

#include <filesystem>
#include <optional>

namespace run3::audio {

class MusicPlayer {
public:
  explicit MusicPlayer(IAudioEngine &engine) noexcept : engine_(engine) {}

  bool play(const std::filesystem::path &file, bool loop,
            float transitionSeconds = 0.0F);
  void toggle(const std::filesystem::path &file, bool loop,
              float transitionSeconds = 0.0F);
  void stop(float fadeSeconds = 0.0F);
  void setVolume(float volume);
  void setPitch(float pitch);
  void update(float seconds);
  void clear() noexcept;

  [[nodiscard]] bool active() const noexcept { return current_.valid(); }
  [[nodiscard]] const SoundHandle &current() const noexcept { return current_; }

private:
  struct PendingTrack {
    std::filesystem::path file;
    bool loop{};
    float delay{};
    float fadeInSeconds{};
  };

  bool start(const std::filesystem::path &file, bool loop,
             float fadeInSeconds);

  IAudioEngine &engine_;
  SoundHandle current_;
  std::optional<PendingTrack> pending_;
  float volume_{1.0F};
  float pitch_{1.0F};
};

} // namespace run3::audio
