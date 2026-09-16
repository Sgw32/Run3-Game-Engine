#include <run3/audio/SoundRuntime.hpp>

#include <algorithm>

namespace run3::audio {

bool SoundRuntime::emit(const std::filesystem::path &file,
                        const float lifetimeSeconds, const bool loop,
                        const Bus bus) {
  if (lifetimeSeconds <= 0.0F || file.empty()) {
    return false;
  }
  PlayOptions options;
  options.file = file;
  options.bus = bus;
  options.loop = loop;
  SoundHandle handle = engine_.play(options);
  if (!handle.valid()) {
    return false;
  }
  sounds_.push_back({std::move(handle), lifetimeSeconds});
  return true;
}

bool SoundRuntime::emit3D(const std::filesystem::path &file,
                          const float lifetimeSeconds, const Vec3 position,
                          const float minDistance, const float maxDistance,
                          const bool loop, const Bus bus, const float gain) {
  if (lifetimeSeconds <= 0.0F || file.empty()) {
    return false;
  }
  PlayOptions options;
  options.file = file;
  options.bus = bus;
  options.loop = loop;
  options.spatial = true;
  options.position = position;
  options.minDistance = minDistance;
  options.maxDistance = maxDistance;
  options.gain = gain;
  SoundHandle handle = engine_.play(options);
  if (!handle.valid()) {
    return false;
  }
  sounds_.push_back({std::move(handle), lifetimeSeconds});
  return true;
}

void SoundRuntime::update(const float seconds) {
  for (TimedSound &sound : sounds_) {
    sound.remaining -= seconds;
  }
  sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
                               [](const TimedSound &sound) {
                                 return sound.remaining <= 0.0F;
                               }),
                sounds_.end());
}

void SoundRuntime::clear() noexcept { sounds_.clear(); }

} // namespace run3::audio
