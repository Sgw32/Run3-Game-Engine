#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace run3::audio {

struct Vec3 {
  float x{};
  float y{};
  float z{};
};

// Run3 and Ogre use +Y up and look down -Z. The legacy OpenAL path passed
// positions, velocities, forward, and up vectors through unchanged. Keep that
// convention in one named boundary instead of scattering axis swaps.
[[nodiscard]] Vec3 fromGameCoordinates(Vec3 value) noexcept;

// Physics and authored content use centimetre-like game units. miniaudio's
// Doppler calculation assumes metres/second, so velocity crosses a distinct,
// centralized unit boundary while positions retain legacy attenuation units.
[[nodiscard]] Vec3 fromGameVelocity(Vec3 value) noexcept;

struct ListenerTransform {
  Vec3 position{};
  Vec3 velocity{};
  Vec3 forward{0.0F, 0.0F, -1.0F};
  Vec3 up{0.0F, 1.0F, 0.0F};
};

enum class Bus : std::uint8_t { master, music, effects, voice };
enum class SoundState : std::uint8_t { invalid, playing, paused, stopped };

struct PlayOptions {
  std::filesystem::path file;
  Bus bus{Bus::effects};
  bool loop{};
  bool streaming{};
  bool spatial{};
  float gain{1.0F};
  float pitch{1.0F};
  Vec3 position{};
  Vec3 velocity{};
  float minDistance{1.0F};
  float maxDistance{1000.0F};
  float fadeInSeconds{};
};

struct AudioEngineConfig {
  std::size_t maxVoices{32};
  // Used by decoder/integration tests. Production miniaudio opens the default
  // output device; a failure is handled by createAudioEngineWithFallback().
  bool noOutputDevice{};
};

struct AudioStats {
  std::size_t activeVoices{};
  std::size_t voiceCapacity{};
};

namespace detail {
class HandleOwner;
}

class SoundHandle {
public:
  SoundHandle() noexcept = default;
  SoundHandle(std::shared_ptr<detail::HandleOwner> owner,
              std::uint64_t token) noexcept;
  ~SoundHandle();

  SoundHandle(const SoundHandle &) = delete;
  SoundHandle &operator=(const SoundHandle &) = delete;
  SoundHandle(SoundHandle &&other) noexcept;
  SoundHandle &operator=(SoundHandle &&other) noexcept;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::uint64_t token() const noexcept { return token_; }
  void reset() noexcept;

private:
  std::shared_ptr<detail::HandleOwner> owner_;
  std::uint64_t token_{};
};

class IAudioEngine {
public:
  virtual ~IAudioEngine() = default;

  [[nodiscard]] virtual std::string_view backendName() const noexcept = 0;
  [[nodiscard]] virtual bool hasOutputDevice() const noexcept = 0;
  [[nodiscard]] virtual std::string lastError() const = 0;
  [[nodiscard]] virtual AudioStats stats() const noexcept = 0;

  [[nodiscard]] virtual SoundHandle play(const PlayOptions &options) = 0;
  virtual bool stop(const SoundHandle &sound, float fadeSeconds = 0.0F) = 0;
  virtual bool setPaused(const SoundHandle &sound, bool paused) = 0;
  virtual bool setLoop(const SoundHandle &sound, bool loop) = 0;
  virtual bool setGain(const SoundHandle &sound, float gain) = 0;
  virtual bool setPitch(const SoundHandle &sound, float pitch) = 0;
  virtual bool setPosition(const SoundHandle &sound, Vec3 position) = 0;
  virtual bool setVelocity(const SoundHandle &sound, Vec3 velocity) = 0;
  virtual bool setAttenuation(const SoundHandle &sound, float minDistance,
                              float maxDistance) = 0;
  virtual bool fadeTo(const SoundHandle &sound, float gain,
                      float seconds) = 0;
  [[nodiscard]] virtual SoundState state(const SoundHandle &sound) const = 0;
  [[nodiscard]] virtual float playbackSeconds(
      const SoundHandle &sound) const = 0;
  virtual bool seekSeconds(const SoundHandle &sound, float seconds) = 0;
  [[nodiscard]] virtual float effectiveGain(
      const SoundHandle &sound) const = 0;

  virtual void setBusGain(Bus bus, float gain) = 0;
  [[nodiscard]] virtual float busGain(Bus bus) const = 0;
  virtual void setListener(const ListenerTransform &listener) = 0;
  virtual void update(float seconds) = 0;
};

using AudioFactory =
    std::function<std::unique_ptr<IAudioEngine>(const AudioEngineConfig &)>;
using AudioLog = std::function<void(std::string_view)>;

[[nodiscard]] std::unique_ptr<IAudioEngine>
createNullAudioEngine(const AudioEngineConfig &config = {});
[[nodiscard]] std::unique_ptr<IAudioEngine>
createMiniaudioEngine(const AudioEngineConfig &config = {});

// Device initialization is not fatal to gameplay. A requested backend error
// is logged and replaced with a bounded null engine using the same contract.
[[nodiscard]] std::unique_ptr<IAudioEngine>
createAudioEngineWithFallback(const AudioEngineConfig &config,
                              const AudioFactory &requested,
                              const AudioLog &log = {});

} // namespace run3::audio
