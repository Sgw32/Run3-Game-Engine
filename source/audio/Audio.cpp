#include <run3/audio/Audio.hpp>

#include "AudioInternal.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace run3::audio {

Vec3 fromGameCoordinates(const Vec3 value) noexcept { return value; }

Vec3 fromGameVelocity(const Vec3 value) noexcept {
  constexpr float gameUnitsToMetres = 0.01F;
  return {value.x * gameUnitsToMetres, value.y * gameUnitsToMetres,
          value.z * gameUnitsToMetres};
}

SoundHandle::SoundHandle(std::shared_ptr<detail::HandleOwner> owner,
                         const std::uint64_t token) noexcept
    : owner_(std::move(owner)), token_(token) {}

SoundHandle::~SoundHandle() { reset(); }

SoundHandle::SoundHandle(SoundHandle &&other) noexcept
    : owner_(std::move(other.owner_)), token_(std::exchange(other.token_, 0)) {}

SoundHandle &SoundHandle::operator=(SoundHandle &&other) noexcept {
  if (this != &other) {
    reset();
    owner_ = std::move(other.owner_);
    token_ = std::exchange(other.token_, 0);
  }
  return *this;
}

bool SoundHandle::valid() const noexcept {
  return token_ != 0 && owner_ != nullptr;
}

void SoundHandle::reset() noexcept {
  if (token_ != 0 && owner_ != nullptr) {
    owner_->release(token_);
  }
  token_ = 0;
  owner_.reset();
}

std::unique_ptr<IAudioEngine>
createAudioEngineWithFallback(const AudioEngineConfig &config,
                              const AudioFactory &requested,
                              const AudioLog &log) {
  try {
    if (!requested) {
      throw std::runtime_error("audio backend factory is empty");
    }
    std::unique_ptr<IAudioEngine> result = requested(config);
    if (!result) {
      throw std::runtime_error("audio backend factory returned null");
    }
    return result;
  } catch (const std::exception &error) {
    if (log) {
      log(std::string("Audio device initialization failed; using null backend: ") +
          error.what());
    }
    return createNullAudioEngine(config);
  }
}

} // namespace run3::audio
