#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace run3::audio {

// Dedicated non-positional legacy music player.  Audiere owns its own output
// device, exactly as it did in the original engine, so tracker music never
// consumes a voice in the effects/voice mixer.
class AudiereMusicPlayer final {
public:
  AudiereMusicPlayer();
  ~AudiereMusicPlayer();
  AudiereMusicPlayer(const AudiereMusicPlayer &) = delete;
  AudiereMusicPlayer &operator=(const AudiereMusicPlayer &) = delete;

  bool play(const std::filesystem::path &file, bool loop,
            float transitionSeconds = 0.0F);
  void stop(float fadeSeconds = 0.0F);
  void setVolume(float volume);
  void setPitch(float pitch);
  void update(float seconds);
  void clear() noexcept;
  [[nodiscard]] bool active() const noexcept;
  [[nodiscard]] std::optional<float> playbackSeconds() const;
  bool seekSeconds(float seconds);
  [[nodiscard]] const std::string &lastError() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace run3::audio
