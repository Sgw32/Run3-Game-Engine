#include <run3/audio/Audio.hpp>

#include "AudioInternal.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
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

std::string resultMessage(const std::string_view operation,
                          const ma_result result) {
  return std::string(operation) + ": " + ma_result_description(result);
}

class MiniaudioEngine final : public IAudioEngine {
public:
  explicit MiniaudioEngine(const AudioEngineConfig &config)
      : noOutputDevice_(config.noOutputDevice),
        slots_(std::max<std::size_t>(1, config.maxVoices)),
        owner_(std::make_shared<detail::HandleOwner>(
            [this](const std::uint64_t token) { release(token); })) {
    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.noDevice = noOutputDevice_ ? MA_TRUE : MA_FALSE;
    if (noOutputDevice_) {
      engineConfig.channels = 2;
      engineConfig.sampleRate = 48000;
    }
    const ma_result result = ma_engine_init(&engineConfig, &engine_);
    if (result != MA_SUCCESS) {
      owner_->detach();
      throw std::runtime_error(resultMessage("ma_engine_init", result));
    }
    engineInitialized_ = true;

    for (ma_sound_group &group : groups_) {
      const ma_result groupResult =
          ma_sound_group_init(&engine_, 0, nullptr, &group);
      if (groupResult != MA_SUCCESS) {
        shutdown();
        throw std::runtime_error(
            resultMessage("ma_sound_group_init", groupResult));
      }
      ++initializedGroups_;
    }
  }

  ~MiniaudioEngine() override {
    owner_->detach();
    shutdown();
  }

  std::string_view backendName() const noexcept override {
    return "miniaudio-0.11.25";
  }
  bool hasOutputDevice() const noexcept override { return !noOutputDevice_; }
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
    const std::string path = options.file.u8string();
    // miniaudio 0.11.25's resource-manager buffer path reads a freed node when
    // initialization fails for a missing or corrupt file. Validate through
    // the decoder first so malformed input never reaches that unsafe error
    // branch. This is a header-only parse; streaming data is not decoded here.
    std::error_code pathError;
    if (!std::filesystem::is_regular_file(options.file, pathError)) {
      lastError_ = "Cannot open " + path + ": file does not exist";
      return {};
    }
    ma_decoder decoder{};
    const ma_decoder_config decoderConfig = ma_decoder_config_init_default();
    const ma_result decoderResult =
        ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder);
    if (decoderResult != MA_SUCCESS) {
      lastError_ = resultMessage("Cannot decode " + path, decoderResult);
      return {};
    }
    ma_decoder_uninit(&decoder);

    ma_uint32 flags = options.streaming ? MA_SOUND_FLAG_STREAM : 0;
    if (!options.spatial) {
      flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    }
    const ma_result result = ma_sound_init_from_file(
        &engine_, path.c_str(), flags, groupFor(options.bus), nullptr,
        &slot.sound);
    if (result != MA_SUCCESS) {
      lastError_ = resultMessage("Cannot open " + path, result);
      return {};
    }

    slot.initialized = true;
    slot.active = true;
    slot.paused = false;
    slot.pendingStop = false;
    slot.stopCountdown = 0.0F;
    slot.options = options;
    slot.gain = options.gain;
    if (++slot.generation == 0) {
      ++slot.generation;
    }

    ma_sound_set_looping(&slot.sound, options.loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&slot.sound, options.gain);
    ma_sound_set_pitch(&slot.sound, options.pitch);
    const Vec3 position = fromGameCoordinates(options.position);
    const Vec3 velocity = fromGameVelocity(options.velocity);
    ma_sound_set_position(&slot.sound, position.x, position.y, position.z);
    ma_sound_set_velocity(&slot.sound, velocity.x, velocity.y, velocity.z);
    ma_sound_set_min_distance(&slot.sound, options.minDistance);
    ma_sound_set_max_distance(&slot.sound, options.maxDistance);
    ma_sound_set_rolloff(&slot.sound, 1.0F);
    if (options.fadeInSeconds > 0.0F) {
      ma_sound_set_fade_in_milliseconds(
          &slot.sound, 0.0F, options.gain,
          static_cast<ma_uint64>(options.fadeInSeconds * 1000.0F));
    }
    const ma_result startResult = ma_sound_start(&slot.sound);
    if (startResult != MA_SUCCESS) {
      lastError_ = resultMessage("ma_sound_start", startResult);
      release(detail::makeToken(
          static_cast<std::size_t>(found - slots_.begin()), slot.generation));
      return {};
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
    if (fadeSeconds > 0.0F) {
      ma_sound_set_fade_in_milliseconds(
          &slot->sound, -1.0F, 0.0F,
          static_cast<ma_uint64>(fadeSeconds * 1000.0F));
      slot->pendingStop = true;
      slot->stopCountdown = fadeSeconds;
      return true;
    }
    slot->pendingStop = false;
    slot->stopCountdown = 0.0F;
    slot->paused = false;
    return ma_sound_stop(&slot->sound) == MA_SUCCESS;
  }

  bool setPaused(const SoundHandle &sound, const bool paused) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    const ma_result result =
        paused ? ma_sound_stop(&slot->sound) : ma_sound_start(&slot->sound);
    if (result == MA_SUCCESS) {
      slot->paused = paused;
      return true;
    }
    lastError_ = resultMessage(paused ? "ma_sound_stop" : "ma_sound_start",
                               result);
    return false;
  }

  bool setLoop(const SoundHandle &sound, const bool loop) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.loop = loop;
    ma_sound_set_looping(&slot->sound, loop ? MA_TRUE : MA_FALSE);
    return true;
  }

  bool setGain(const SoundHandle &sound, const float gain) override {
    Slot *slot = find(sound);
    if (slot == nullptr || gain < 0.0F) {
      return false;
    }
    slot->gain = gain;
    slot->options.gain = gain;
    ma_sound_set_volume(&slot->sound, gain);
    return true;
  }

  bool setPitch(const SoundHandle &sound, const float pitch) override {
    Slot *slot = find(sound);
    if (slot == nullptr || pitch <= 0.0F) {
      return false;
    }
    slot->options.pitch = pitch;
    ma_sound_set_pitch(&slot->sound, pitch);
    return true;
  }

  bool setPosition(const SoundHandle &sound, const Vec3 position) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.position = position;
    const Vec3 converted = fromGameCoordinates(position);
    ma_sound_set_position(&slot->sound, converted.x, converted.y, converted.z);
    return true;
  }

  bool setVelocity(const SoundHandle &sound, const Vec3 velocity) override {
    Slot *slot = find(sound);
    if (slot == nullptr) {
      return false;
    }
    slot->options.velocity = velocity;
    const Vec3 converted = fromGameVelocity(velocity);
    ma_sound_set_velocity(&slot->sound, converted.x, converted.y, converted.z);
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
    ma_sound_set_min_distance(&slot->sound, minDistance);
    ma_sound_set_max_distance(&slot->sound, maxDistance);
    return true;
  }

  bool fadeTo(const SoundHandle &sound, const float gain,
              const float seconds) override {
    Slot *slot = find(sound);
    if (slot == nullptr || gain < 0.0F || seconds < 0.0F) {
      return false;
    }
    slot->gain = gain;
    slot->options.gain = gain;
    ma_sound_set_fade_in_milliseconds(
        &slot->sound, -1.0F, gain,
        static_cast<ma_uint64>(seconds * 1000.0F));
    return true;
  }

  SoundState state(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    if (slot == nullptr) {
      return SoundState::invalid;
    }
    if (slot->paused) {
      return SoundState::paused;
    }
    return ma_sound_is_playing(&slot->sound) == MA_TRUE
               ? SoundState::playing
               : SoundState::stopped;
  }

  float playbackSeconds(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    if (slot == nullptr) {
      return 0.0F;
    }
    float cursor{};
    return ma_sound_get_cursor_in_seconds(&slot->sound, &cursor) == MA_SUCCESS
               ? cursor
               : 0.0F;
  }

  bool seekSeconds(const SoundHandle &sound, const float seconds) override {
    Slot *slot = find(sound);
    if (slot == nullptr || seconds < 0.0F) {
      return false;
    }
    const ma_uint64 frame = static_cast<ma_uint64>(
        seconds * static_cast<float>(ma_engine_get_sample_rate(&engine_)));
    return ma_sound_seek_to_pcm_frame(&slot->sound, frame) == MA_SUCCESS;
  }

  float effectiveGain(const SoundHandle &sound) const override {
    const Slot *slot = find(sound);
    if (slot == nullptr || state(sound) == SoundState::stopped) {
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
    if (bus == Bus::master) {
      ma_engine_set_volume(&engine_, gain);
    } else {
      ma_sound_group_set_volume(groupFor(bus), gain);
    }
  }

  float busGain(const Bus bus) const override { return gains_[busIndex(bus)]; }

  void setListener(const ListenerTransform &listener) override {
    listener_ = listener;
    const Vec3 position = fromGameCoordinates(listener.position);
    const Vec3 velocity = fromGameVelocity(listener.velocity);
    const Vec3 forward = fromGameCoordinates(listener.forward);
    const Vec3 up = fromGameCoordinates(listener.up);
    ma_engine_listener_set_position(&engine_, 0, position.x, position.y,
                                    position.z);
    ma_engine_listener_set_velocity(&engine_, 0, velocity.x, velocity.y,
                                    velocity.z);
    ma_engine_listener_set_direction(&engine_, 0, forward.x, forward.y,
                                     forward.z);
    ma_engine_listener_set_world_up(&engine_, 0, up.x, up.y, up.z);
  }

  void update(const float seconds) override {
    if (seconds < 0.0F || !std::isfinite(seconds)) {
      throw std::invalid_argument("audio update duration must be finite and non-negative");
    }
    for (Slot &slot : slots_) {
      if (!slot.active || !slot.pendingStop) {
        continue;
      }
      slot.stopCountdown = std::max(0.0F, slot.stopCountdown - seconds);
      if (slot.stopCountdown == 0.0F) {
        static_cast<void>(ma_sound_stop(&slot.sound));
        slot.pendingStop = false;
      }
    }
    if (noOutputDevice_) {
      ma_uint64 remaining = static_cast<ma_uint64>(
          seconds * static_cast<float>(ma_engine_get_sample_rate(&engine_)));
      const ma_uint32 channels = ma_engine_get_channels(&engine_);
      std::vector<float> scratch(static_cast<std::size_t>(4096U * channels));
      while (remaining > 0) {
        const ma_uint64 requested = std::min<ma_uint64>(remaining, 4096U);
        ma_uint64 read{};
        const ma_result result = ma_engine_read_pcm_frames(
            &engine_, scratch.data(), requested, &read);
        if ((result != MA_SUCCESS && result != MA_AT_END) || read == 0) {
          break;
        }
        remaining -= read;
      }
    }
  }

private:
  struct Slot {
    ma_sound sound{};
    bool initialized{};
    bool active{};
    bool paused{};
    bool pendingStop{};
    float stopCountdown{};
    float gain{1.0F};
    std::uint32_t generation{};
    PlayOptions options;
  };

  ma_sound_group *groupFor(const Bus bus) {
    if (bus == Bus::master) {
      return nullptr;
    }
    return &groups_[busIndex(bus) - 1U];
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
  Slot *find(const SoundHandle &sound) {
    return const_cast<Slot *>(std::as_const(*this).find(sound));
  }
  void release(const std::uint64_t token) noexcept {
    if (token == 0) {
      return;
    }
    const std::size_t index = detail::tokenSlot(token);
    if (index >= slots_.size()) {
      return;
    }
    Slot &slot = slots_[index];
    if (!slot.active ||
        slot.generation != detail::tokenGeneration(token)) {
      return;
    }
    if (slot.initialized) {
      ma_sound_uninit(&slot.sound);
    }
    slot = {};
    slot.generation = detail::tokenGeneration(token);
  }
  void shutdown() noexcept {
    for (Slot &slot : slots_) {
      if (slot.initialized) {
        ma_sound_uninit(&slot.sound);
        slot.initialized = false;
      }
      slot.active = false;
    }
    while (initializedGroups_ > 0) {
      --initializedGroups_;
      ma_sound_group_uninit(&groups_[initializedGroups_]);
    }
    if (engineInitialized_) {
      ma_engine_uninit(&engine_);
      engineInitialized_ = false;
    }
  }

  bool noOutputDevice_{};
  ma_engine engine_{};
  bool engineInitialized_{};
  std::array<ma_sound_group, 3> groups_{};
  std::size_t initializedGroups_{};
  std::vector<Slot> slots_;
  std::shared_ptr<detail::HandleOwner> owner_;
  std::array<float, 4> gains_{1.0F, 1.0F, 1.0F, 1.0F};
  ListenerTransform listener_;
  std::string lastError_;
};

} // namespace

std::unique_ptr<IAudioEngine>
createMiniaudioEngine(const AudioEngineConfig &config) {
  return std::make_unique<MiniaudioEngine>(config);
}

} // namespace run3::audio
