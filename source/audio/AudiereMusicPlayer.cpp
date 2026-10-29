#include <run3/audio/AudiereMusicPlayer.hpp>

#ifdef _WIN32
// This port links Audiere statically; do not import the obsolete bundled DLL.
#define ADR_DECL
#include <audiere.h>
#endif

#include <algorithm>
#include <cmath>
#include <utility>

namespace run3::audio {

class AudiereMusicPlayer::Impl final {
public:
#ifdef _WIN32
  struct PendingTrack {
    std::filesystem::path file;
    bool loop{};
  };

  // Audiere's DirectSound stream path queues stop callbacks holding the old
  // stream and is not teardown-safe on modern 64-bit Windows. Its WinMM
  // device uses the library's software mixer, remains independent from the
  // game effects backend, and permits the original 3x cutscene pitch.
  Impl() : device(audiere::OpenDevice("winmm")) {
    if (!device) error = "Audiere could not open an output device";
  }

  bool start(const std::filesystem::path &file, const bool loop) {
    if (!device) return false;
    const std::string nativeFile = file.string();
    source = audiere::OpenSampleSource(nativeFile.c_str());
    if (!source) {
      error = "Audiere cannot decode " + nativeFile;
      return false;
    }
    int channels{};
    audiere::SampleFormat format{};
    source->getFormat(channels, sampleRate, format);
    stream = device->openStream(source.get());
    if (!stream) {
      source = nullptr;
      error = "Audiere cannot allocate a stream for " + nativeFile;
      return false;
    }
    stream->setRepeat(loop);
    stream->setVolume(volume);
    stream->setPitchShift(pitch);
    stream->play();
    error.clear();
    return true;
  }

  audiere::AudioDevicePtr device;
  audiere::SampleSourcePtr source;
  audiere::OutputStreamPtr stream;
  std::optional<PendingTrack> pending;
  float transitionRemaining{};
  float transitionDuration{};
  int sampleRate{};
#endif
  float volume{1.0F};
  float pitch{1.0F};
  std::string error;
};

AudiereMusicPlayer::AudiereMusicPlayer() : impl_(std::make_unique<Impl>()) {}
AudiereMusicPlayer::~AudiereMusicPlayer() = default;

bool AudiereMusicPlayer::play(const std::filesystem::path &file,
                              const bool loop,
                              const float transitionSeconds) {
#ifdef _WIN32
  if (file.empty()) {
    stop(transitionSeconds);
    return true;
  }
  if (impl_->stream && transitionSeconds > 0.0F) {
    impl_->pending = Impl::PendingTrack{file, loop};
    impl_->transitionRemaining = transitionSeconds;
    impl_->transitionDuration = transitionSeconds;
    return true;
  }
  clear();
  return impl_->start(file, loop);
#else
  (void)file; (void)loop; (void)transitionSeconds;
  impl_->error = "Audiere music is available only in the Windows build";
  return false;
#endif
}

void AudiereMusicPlayer::stop(const float fadeSeconds) {
#ifdef _WIN32
  impl_->pending.reset();
  if (!impl_->stream) return;
  if (fadeSeconds > 0.0F) {
    impl_->transitionRemaining = fadeSeconds;
    impl_->transitionDuration = fadeSeconds;
  } else {
    // No Audiere callbacks are registered. Releasing removes the mixer stream
    // synchronously; calling stop() would enqueue an obsolete StopEvent on the
    // library's callback thread and races 64-bit teardown.
    impl_->stream = nullptr;
    impl_->source = nullptr;
  }
#else
  (void)fadeSeconds;
#endif
}

void AudiereMusicPlayer::setVolume(const float volume) {
  // Audiere's mixer stores volume as an unbounded fixed-point multiplier.
  // Values above 1.0 therefore amplify and hard-clip rather than representing
  // a conventional normalized gain. MapAudio may apply legacy compensation
  // to quiet authored values, but the dedicated music device must never be
  // driven beyond nominal output.
  impl_->volume = std::clamp(volume, 0.0F, 1.0F);
#ifdef _WIN32
  if (impl_->stream) impl_->stream->setVolume(impl_->volume);
#endif
}

void AudiereMusicPlayer::setPitch(const float pitch) {
  impl_->pitch = std::clamp(pitch, 0.01F, 8.0F);
#ifdef _WIN32
  if (impl_->stream) impl_->stream->setPitchShift(impl_->pitch);
#endif
}

void AudiereMusicPlayer::update(const float seconds) {
#ifdef _WIN32
  if (impl_->device) impl_->device->update();
  if (!impl_->stream || impl_->transitionRemaining <= 0.0F) return;
  impl_->transitionRemaining =
      std::max(0.0F, impl_->transitionRemaining - std::max(0.0F, seconds));
  if (impl_->transitionRemaining > 0.0F) {
    if (impl_->transitionDuration > 0.0F)
      impl_->stream->setVolume(impl_->volume *
          impl_->transitionRemaining / impl_->transitionDuration);
    return;
  }
  impl_->stream = nullptr;
  impl_->source = nullptr;
  if (impl_->pending) {
    const auto next = std::move(*impl_->pending);
    impl_->pending.reset();
    static_cast<void>(impl_->start(next.file, next.loop));
  }
#else
  (void)seconds;
#endif
}

void AudiereMusicPlayer::clear() noexcept {
#ifdef _WIN32
  impl_->pending.reset();
  impl_->transitionRemaining = 0.0F;
  impl_->transitionDuration = 0.0F;
  impl_->stream = nullptr;
  impl_->source = nullptr;
  impl_->sampleRate = 0;
#endif
}

bool AudiereMusicPlayer::active() const noexcept {
#ifdef _WIN32
  return impl_->stream && impl_->stream->isPlaying();
#else
  return false;
#endif
}

std::optional<float> AudiereMusicPlayer::playbackSeconds() const {
#ifdef _WIN32
  if (impl_->pending) return 0.0F;
  if (!impl_->stream || impl_->sampleRate <= 0) return std::nullopt;
  return static_cast<float>(impl_->stream->getPosition()) /
         static_cast<float>(impl_->sampleRate);
#else
  return std::nullopt;
#endif
}

bool AudiereMusicPlayer::seekSeconds(const float seconds) {
#ifdef _WIN32
  if (!impl_->stream || !impl_->stream->isSeekable() ||
      impl_->sampleRate <= 0 || !std::isfinite(seconds) || seconds < 0.0F)
    return false;
  impl_->stream->setPosition(static_cast<int>(
      seconds * static_cast<float>(impl_->sampleRate)));
  return true;
#else
  (void)seconds;
  return false;
#endif
}

const std::string &AudiereMusicPlayer::lastError() const noexcept {
  return impl_->error;
}

} // namespace run3::audio
