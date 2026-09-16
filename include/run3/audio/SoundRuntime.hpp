#pragma once

#include <run3/audio/Audio.hpp>

#include <filesystem>
#include <vector>

namespace run3::audio {

// Map-scoped replacement for Run3SoundRuntime. Ownership of each voice is
// explicit; destroying or clearing this object returns every pool slot.
class SoundRuntime {
public:
  explicit SoundRuntime(IAudioEngine &engine) noexcept : engine_(engine) {}

  [[nodiscard]] bool emit(const std::filesystem::path &file,
                          float lifetimeSeconds, bool loop = false,
                          Bus bus = Bus::effects);
  [[nodiscard]] bool emit3D(const std::filesystem::path &file,
                            float lifetimeSeconds, Vec3 position,
                            float minDistance, float maxDistance,
                            bool loop = false, Bus bus = Bus::effects);
  void update(float seconds);
  void clear() noexcept;
  [[nodiscard]] std::size_t activeCount() const noexcept {
    return sounds_.size();
  }

private:
  struct TimedSound {
    SoundHandle handle;
    float remaining{};
  };

  IAudioEngine &engine_;
  std::vector<TimedSound> sounds_;
};

} // namespace run3::audio
