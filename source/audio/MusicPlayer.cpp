#include <run3/audio/MusicPlayer.hpp>

#include <algorithm>
#include <utility>

namespace run3::audio {

bool MusicPlayer::play(const std::filesystem::path &file, const bool loop,
                       const float transitionSeconds) {
  if (file.empty()) {
    stop(transitionSeconds);
    return true;
  }
  if (current_.valid() && transitionSeconds > 0.0F) {
    static_cast<void>(engine_.stop(current_, transitionSeconds));
    pending_ = PendingTrack{file, loop, transitionSeconds, transitionSeconds};
    return true;
  }
  clear();
  return start(file, loop, transitionSeconds);
}

void MusicPlayer::toggle(const std::filesystem::path &file, const bool loop,
                         const float transitionSeconds) {
  if (current_.valid()) {
    stop(transitionSeconds);
  } else {
    static_cast<void>(play(file, loop, transitionSeconds));
  }
}

void MusicPlayer::stop(const float fadeSeconds) {
  pending_.reset();
  if (!current_.valid()) {
    return;
  }
  static_cast<void>(engine_.stop(current_, fadeSeconds));
  if (fadeSeconds <= 0.0F) {
    current_.reset();
  }
}

void MusicPlayer::setVolume(const float volume) {
  volume_ = std::max(0.0F, volume);
  if (current_.valid()) {
    static_cast<void>(engine_.setGain(current_, volume_));
  }
}

void MusicPlayer::setPitch(const float pitch) {
  pitch_ = std::max(0.01F, pitch);
  if (current_.valid()) {
    static_cast<void>(engine_.setPitch(current_, pitch_));
  }
}

void MusicPlayer::update(const float seconds) {
  if (current_.valid() && !pending_ &&
      engine_.state(current_) == SoundState::stopped) {
    current_.reset();
  }
  if (!pending_) {
    return;
  }
  pending_->delay = std::max(0.0F, pending_->delay - seconds);
  if (pending_->delay > 0.0F) {
    return;
  }
  PendingTrack next = std::move(*pending_);
  pending_.reset();
  current_.reset();
  static_cast<void>(start(next.file, next.loop, next.fadeInSeconds));
}

void MusicPlayer::clear() noexcept {
  pending_.reset();
  current_.reset();
}

bool MusicPlayer::start(const std::filesystem::path &file, const bool loop,
                        const float fadeInSeconds) {
  PlayOptions options;
  options.file = file;
  options.bus = Bus::music;
  options.loop = loop;
  options.streaming = true;
  options.gain = volume_;
  options.pitch = pitch_;
  options.fadeInSeconds = std::max(0.0F, fadeInSeconds);
  current_ = engine_.play(options);
  return current_.valid();
}

} // namespace run3::audio
