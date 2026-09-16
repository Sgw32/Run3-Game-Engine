#include <run3/audio/Audio.hpp>

#include "AudioInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace run3::audio {
namespace {

constexpr std::size_t busIndex(const Bus bus) {
  return static_cast<std::size_t>(bus);
}

float distance(const Vec3 a, const Vec3 b) {
  const float x = a.x - b.x;
  const float y = a.y - b.y;
  const float z = a.z - b.z;
  return std::sqrt(x * x + y * y + z * z);
}

class NullAudioEngine final : public IAudioEngine {
public:
  explicit NullAudioEngine(const AudioEngineConfig &config)
      : slots_(std::max<std::size_t>(1, config.maxVoices)),
        owner_(std::make_shared<detail::HandleOwner>(
            [this](const std::uint64_t token) { release(token); })) {}

  ~NullAudioEngine() override { owner_->detach(); }

  std::string_view backendName() const noexcept override { return "null"; }
  bool hasOutputDevice() const noexcept override { return false; }
  std::string lastError() const override { return lastError_; }
  AudioStats stats() const noexcept override {
    return {static_cast<std::size_t>(std::count_if(
                slots_.begin(), slots_.end(),
                [](const Slot &slot) { return slot.active; })),
            slots_.size()};
  }

  SoundHandle play(const PlayOptions &options) override {
    if (options.file.empty()) {
      lastError_ = "audio path is empty";
      return {};
    }
    if (options.gain < 0.0F || options.pitch <= 0.0F ||
        options.minDistance < 0.0F ||
        options.maxDistance < options.minDistance) {
      lastError_ = "invalid gain, pitch, or attenuation distances";
      return {};
    }
    const auto found = std::find_if(slots_.begin(), slots_.end(),
                                    [](const Slot &slot) {
                                      return !slot.active;
                                    });
    if (found == slots_.end()) {
      lastError_ = "audio voice pool is full";
      return {};
    }
    Slot &slot = *found;
    slot.active = true;
    slot.state = SoundState::playing;
    slot.options = options;
    slot.gain = options.fadeInSeconds > 0.0F ? 0.0F : options.gain;
    slot.fadeStart = slot.gain;
    slot.fadeTarget = options.gain;
    slot.fadeRemaining = options.fadeInSeconds;
    slot.fadeDuration = options.fadeInSeconds;
    slot.pendingStop = false;
    slot.playbackSeconds = 0.0F;
    if (++slot.generation == 0) {
      ++slot.generation;
    }
    lastError_.clear();
    const auto index = static_cast<std::size_t>(found - slots_.begin());
    return {owner_, detail::makeToken(index, slot.generation)};
  }

  bool stop(const SoundHandle &sound, const float fadeSeconds) override {
    Slot *slot = find(sound);
    if (slot == nullptr || fadeSeconds < 0.0F) {
      return false;
    }
    if (fadeSeconds == 0.0F) {
      slot->state = SoundState::stopped;
      slot->gain = 0.0F;
      slot->pendingStop = false;
    } else {
      beginFade(*slot, 0.0F, fadeSeconds);
      slot->pendingStop = true;
    }
    return true;
  }

  bool setPaused(const SoundHandle &sound, const bool paused) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->state = paused ? SoundState::paused : SoundState::playing;
    return true;
  }

  bool setLoop(const SoundHandle &sound, const bool loop) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.loop = loop;
    return true;
  }

  bool setGain(const SoundHandle &sound, const float gain) override {
    Slot *slot = find(sound);
    if (slot == nullptr || gain < 0.0F) {
      return false;
    }
    slot->gain = gain;
    slot->options.gain = gain;
    slot->fadeRemaining = 0.0F;
    return true;
  }

  bool setPitch(const SoundHandle &sound, const float pitch) override {
    Slot *slot = find(sound);
    if (slot == nullptr || pitch <= 0.0F) {
      return false;
    }
    slot->options.pitch = pitch;
    return true;
  }

  bool setPosition(const SoundHandle &sound, const Vec3 position) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.position = position;
    return true;
  }

  bool setVelocity(const SoundHandle &sound, const Vec3 velocity) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.velocity = velocity;
    return true;
  }

  bool setAttenuation(const SoundHandle &sound, const float minDistance,
                      const float maxDistance) override {
    Slot *slot = find(sound);
    if (slot == nullptr || minDistance < 0.0F || maxDistance < minDistance) {
      return false;
    }
    slot->options.minDistance = minDistance;
    slot->options.maxDistance = maxDistance;
    return true;
  }

  bool fadeTo(const SoundHandle &sound, const float gain,
              const float seconds) override {
    Slot *slot = find(sound);
    if (slot == nullptr || gain < 0.0F || seconds < 0.0F) {
      return false;
    }
    if (seconds == 0.0F) {
      slot->gain = gain;
      slot->fadeRemaining = 0.0F;
    } else {
      beginFade(*slot, gain, seconds);
    }
    return true;
  }

  SoundState state(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    return slot == nullptr ? SoundState::invalid : slot->state;
  }

  float playbackSeconds(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    return slot == nullptr ? 0.0F : slot->playbackSeconds;
  }

  bool seekSeconds(const SoundHandle &sound, const float seconds) override {
    Slot *slot = find(sound);
    if (slot == nullptr || seconds < 0.0F) {
      return false;
    }
    slot->playbackSeconds = seconds;
    return true;
  }

  float effectiveGain(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    if (slot == nullptr || slot->state == SoundState::stopped) {
      return 0.0F;
    }
    float result = slot->gain * gains_[busIndex(slot->options.bus)];
    if (slot->options.bus != Bus::master) {
      result *= gains_[busIndex(Bus::master)];
    }
    if (!slot->options.spatial) {
      return result;
    }
    const float separation = distance(slot->options.position, listener_.position);
    if (separation >= slot->options.maxDistance) {
      return 0.0F;
    }
    if (separation <= slot->options.minDistance) {
      return result;
    }
    const float range = slot->options.maxDistance - slot->options.minDistance;
    return result * (1.0F - (separation - slot->options.minDistance) / range);
  }

  void setBusGain(const Bus bus, const float gain) override {
    if (gain < 0.0F) {
      throw std::invalid_argument("audio bus gain cannot be negative");
    }
    gains_[busIndex(bus)] = gain;
  }

  float busGain(const Bus bus) const override { return gains_[busIndex(bus)]; }

  void setListener(const ListenerTransform &listener) override {
    listener_ = listener;
  }

  void update(const float seconds) override {
    if (seconds < 0.0F || !std::isfinite(seconds)) {
      throw std::invalid_argument("audio update duration must be finite and non-negative");
    }
    for (Slot &slot : slots_) {
      if (!slot.active) {
        continue;
      }
      if (slot.state == SoundState::playing) {
        slot.playbackSeconds += seconds * slot.options.pitch;
      }
      if (slot.fadeRemaining <= 0.0F) {
        continue;
      }
      slot.fadeRemaining = std::max(0.0F, slot.fadeRemaining - seconds);
      const float progress = slot.fadeDuration <= 0.0F
                                 ? 1.0F
                                 : 1.0F - slot.fadeRemaining / slot.fadeDuration;
      slot.gain = slot.fadeStart +
                  (slot.fadeTarget - slot.fadeStart) * progress;
      if (slot.fadeRemaining == 0.0F && slot.pendingStop) {
        slot.state = SoundState::stopped;
        slot.pendingStop = false;
      }
    }
  }

private:
  struct Slot {
    bool active{};
    std::uint32_t generation{};
    SoundState state{SoundState::invalid};
    PlayOptions options;
    float gain{1.0F};
    float playbackSeconds{};
    float fadeStart{};
    float fadeTarget{};
    float fadeDuration{};
    float fadeRemaining{};
    bool pendingStop{};
  };

  Slot *find(const SoundHandle &sound) {
    return const_cast<Slot *>(std::as_const(*this).find(sound));
  }
  const Slot *find(const SoundHandle &sound) const {
    if (!sound.valid() || sound.token() == 0) {
      return nullptr;
    }
    const std::size_t index = detail::tokenSlot(sound.token());
    if (index >= slots_.size()) {
      return nullptr;
    }
    const Slot &slot = slots_[index];
    return slot.active &&
                   slot.generation == detail::tokenGeneration(sound.token())
               ? &slot
               : nullptr;
  }
  static void beginFade(Slot &slot, const float target, const float seconds) {
    slot.fadeStart = slot.gain;
    slot.fadeTarget = target;
    slot.fadeDuration = seconds;
    slot.fadeRemaining = seconds;
  }
  void release(const std::uint64_t token) noexcept {
    if (token == 0) {
      return;
    }
    const std::size_t index = detail::tokenSlot(token);
    if (index < slots_.size() && slots_[index].active &&
        slots_[index].generation == detail::tokenGeneration(token)) {
      slots_[index].active = false;
      slots_[index].state = SoundState::invalid;
    }
  }

  std::vector<Slot> slots_;
  std::shared_ptr<detail::HandleOwner> owner_;
  std::array<float, 4> gains_{1.0F, 1.0F, 1.0F, 1.0F};
  ListenerTransform listener_;
  std::string lastError_;
};

} // namespace

std::unique_ptr<IAudioEngine>
createNullAudioEngine(const AudioEngineConfig &config) {
  return std::make_unique<NullAudioEngine>(config);
}

} // namespace run3::audio
