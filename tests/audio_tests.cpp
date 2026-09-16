#include <run3/audio/Audio.hpp>
#include <run3/audio/MusicPlayer.hpp>
#include <run3/audio/SoundRuntime.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
using namespace run3::audio;

class TemporaryAudioFiles {
public:
  TemporaryAudioFiles() {
    const auto stamp = std::chrono::high_resolution_clock::now()
                           .time_since_epoch()
                           .count();
    root_ = fs::temp_directory_path() /
            ("run3-step7-audio-" + std::to_string(stamp));
    fs::create_directories(root_);
  }
  ~TemporaryAudioFiles() {
    std::error_code error;
    fs::remove_all(root_, error);
  }

  fs::path makeWave(std::string name) const {
    const fs::path path = root_ / std::move(name);
    constexpr std::uint32_t sampleRate = 8000;
    constexpr std::uint16_t channels = 1;
    constexpr std::uint16_t bits = 16;
    constexpr std::uint32_t frames = 800;
    constexpr std::uint32_t dataSize = frames * channels * (bits / 8);
    std::ofstream output(path, std::ios::binary);
    const auto write16 = [&output](const std::uint16_t value) {
      const std::array<char, 2> bytes{
          static_cast<char>(value & 0xffU),
          static_cast<char>((value >> 8U) & 0xffU)};
      output.write(bytes.data(), bytes.size());
    };
    const auto write32 = [&output](const std::uint32_t value) {
      const std::array<char, 4> bytes{
          static_cast<char>(value & 0xffU),
          static_cast<char>((value >> 8U) & 0xffU),
          static_cast<char>((value >> 16U) & 0xffU),
          static_cast<char>((value >> 24U) & 0xffU)};
      output.write(bytes.data(), bytes.size());
    };
    output.write("RIFF", 4);
    write32(36U + dataSize);
    output.write("WAVEfmt ", 8);
    write32(16);
    write16(1);
    write16(channels);
    write32(sampleRate);
    write32(sampleRate * channels * (bits / 8));
    write16(channels * (bits / 8));
    write16(bits);
    output.write("data", 4);
    write32(dataSize);
    for (std::uint32_t frame = 0; frame < frames; ++frame) {
      write16(frame % 16U < 8U ? 1200U : static_cast<std::uint16_t>(-1200));
    }
    return path;
  }

  fs::path makeCorrupt(std::string name) const {
    const fs::path path = root_ / std::move(name);
    std::ofstream(path, std::ios::binary) << "not an audio stream";
    return path;
  }

private:
  fs::path root_;
};

fs::path firstWithExtension(const fs::path &root, const std::string &extension) {
  std::error_code error;
  if (!fs::is_directory(root, error)) {
    return {};
  }
  for (fs::recursive_directory_iterator iterator(
           root, fs::directory_options::skip_permission_denied, error),
       end;
       !error && iterator != end; iterator.increment(error)) {
    if (iterator->is_regular_file(error) &&
        iterator->path().extension() == extension) {
      return iterator->path();
    }
  }
  return {};
}

std::vector<fs::path> allWithExtension(const fs::path &root,
                                       const std::string &extension) {
  std::vector<fs::path> result;
  std::error_code error;
  if (!fs::is_directory(root, error)) {
    return result;
  }
  for (fs::recursive_directory_iterator iterator(
           root, fs::directory_options::skip_permission_denied, error),
       end;
       !error && iterator != end; iterator.increment(error)) {
    if (iterator->is_regular_file(error) &&
        iterator->path().extension() == extension) {
      result.push_back(iterator->path());
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace

TEST_CASE("game and Ogre audio coordinates retain the legacy convention") {
  const Vec3 input{12.0F, -3.0F, 44.0F};
  const Vec3 converted = fromGameCoordinates(input);
  CHECK(converted.x == input.x);
  CHECK(converted.y == input.y);
  CHECK(converted.z == input.z);
}

TEST_CASE("null backend has bounded generation-safe RAII voices") {
  auto engine = createNullAudioEngine({1, false});
  PlayOptions options;
  options.file = "first.wav";
  SoundHandle first = engine->play(options);
  REQUIRE(first.valid());
  const std::uint64_t firstToken = first.token();
  CHECK(engine->stats().activeVoices == 1);
  CHECK_FALSE(engine->play(options).valid());
  CHECK(engine->lastError().find("pool is full") != std::string::npos);

  first.reset();
  CHECK(engine->stats().activeVoices == 0);
  SoundHandle replacement = engine->play(options);
  REQUIRE(replacement.valid());
  CHECK(replacement.token() != firstToken);
  CHECK(engine->state(first) == SoundState::invalid);
}

TEST_CASE("null backend supports state, fades, pitch, seek, buses, and 3D attenuation") {
  auto engine = createNullAudioEngine({4, false});
  PlayOptions options;
  options.file = "spatial.wav";
  options.spatial = true;
  options.loop = true;
  options.position = {55.0F, 0.0F, 0.0F};
  options.minDistance = 10.0F;
  options.maxDistance = 100.0F;
  SoundHandle sound = engine->play(options);
  REQUIRE(sound.valid());
  engine->setListener({});
  engine->setBusGain(Bus::master, 0.5F);
  engine->setBusGain(Bus::effects, 0.5F);
  CHECK(engine->effectiveGain(sound) == Catch::Approx(0.125F));

  CHECK(engine->setPaused(sound, true));
  CHECK(engine->state(sound) == SoundState::paused);
  CHECK(engine->setPaused(sound, false));
  CHECK(engine->setLoop(sound, false));
  CHECK(engine->setPitch(sound, 2.0F));
  engine->update(0.5F);
  CHECK(engine->playbackSeconds(sound) == Catch::Approx(1.0F));
  CHECK(engine->seekSeconds(sound, 3.0F));
  CHECK(engine->playbackSeconds(sound) == Catch::Approx(3.0F));
  CHECK(engine->fadeTo(sound, 0.25F, 1.0F));
  engine->update(1.0F);
  CHECK(engine->effectiveGain(sound) == Catch::Approx(0.03125F));
  CHECK(engine->stop(sound, 0.5F));
  engine->update(0.5F);
  CHECK(engine->state(sound) == SoundState::stopped);
}

TEST_CASE("map-scoped sound runtime releases every voice on clear") {
  auto engine = createNullAudioEngine({3, false});
  SoundRuntime runtime(*engine);
  CHECK(runtime.emit("ui.wav", 1.0F));
  CHECK(runtime.emit3D("world.wav", 5.0F, {1.0F, 2.0F, 3.0F}, 2.0F,
                       50.0F, true, Bus::voice));
  CHECK(runtime.activeCount() == 2);
  CHECK(engine->stats().activeVoices == 2);
  runtime.update(1.1F);
  CHECK(runtime.activeCount() == 1);
  runtime.clear();
  CHECK(engine->stats().activeVoices == 0);
}

TEST_CASE("music player streams, loops, transitions, fades, and changes pitch") {
  auto engine = createNullAudioEngine({2, false});
  MusicPlayer player(*engine);
  REQUIRE(player.play("track-a.flac", true));
  const auto firstToken = player.current().token();
  player.setVolume(0.6F);
  player.setPitch(0.8F);
  REQUIRE(player.play("track-b.flac", false, 1.0F));
  engine->update(0.5F);
  player.update(0.5F);
  CHECK(player.current().token() == firstToken);
  engine->update(0.5F);
  player.update(0.5F);
  REQUIRE(player.active());
  CHECK(player.current().token() != firstToken);
  CHECK(engine->effectiveGain(player.current()) == Catch::Approx(0.0F));
  engine->update(0.5F);
  CHECK(engine->effectiveGain(player.current()) == Catch::Approx(0.3F));
  player.stop(0.25F);
  engine->update(0.25F);
  player.update(0.25F);
  CHECK_FALSE(player.active());
}

TEST_CASE("device initialization failure falls back to null audio") {
  std::string logged;
  const AudioFactory failing = [](const AudioEngineConfig &)
      -> std::unique_ptr<IAudioEngine> {
    throw std::runtime_error("injected device failure");
  };
  auto engine = createAudioEngineWithFallback(
      {4, false}, failing,
      [&logged](const std::string_view message) { logged = message; });
  REQUIRE(engine);
  CHECK(engine->backendName() == "null");
  CHECK_FALSE(engine->hasOutputDevice());
  CHECK(logged.find("injected device failure") != std::string::npos);
}

TEST_CASE("miniaudio reports missing and corrupt data and decodes WAV") {
  TemporaryAudioFiles files;
  auto engine = createMiniaudioEngine({4, true});
  CHECK(engine->backendName() == "miniaudio-0.11.25");
  CHECK_FALSE(engine->hasOutputDevice());

  PlayOptions options;
  options.file = files.makeWave("valid.wav");
  SoundHandle wave = engine->play(options);
  REQUIRE(wave.valid());
  CHECK(engine->state(wave) != SoundState::invalid);
  wave.reset();

  options.file = options.file.parent_path() / "missing.wav";
  CHECK_FALSE(engine->play(options).valid());
  CHECK_FALSE(engine->lastError().empty());
  options.file = files.makeCorrupt("corrupt.wav");
  CHECK_FALSE(engine->play(options).valid());
  CHECK_FALSE(engine->lastError().empty());
}

TEST_CASE("local MP3 and converted FLAC decode when authorized content is attached") {
  const fs::path source = RUN3_TEST_SOURCE_DIR;
  const fs::path sounds = source / "Games" / "The Long Way" / "TheLongWay" /
                          "run3" / "sounds";
  const fs::path mp3 = firstWithExtension(sounds, ".mp3");
  const std::vector<fs::path> flacs = allWithExtension(
      source / "converted-content" / "audio-flac", ".flac");
  if (mp3.empty() || flacs.empty()) {
    SUCCEED("optional full content or converted tracker output is absent");
    return;
  }
  auto engine = createMiniaudioEngine({2, true});
  PlayOptions options;
  options.streaming = true;
  options.bus = Bus::music;
  options.file = mp3;
  SoundHandle mp3Sound = engine->play(options);
  REQUIRE(mp3Sound.valid());
  mp3Sound.reset();
  for (const fs::path &flac : flacs) {
    options.file = flac;
    SoundHandle converted = engine->play(options);
    REQUIRE(converted.valid());
  }
}
