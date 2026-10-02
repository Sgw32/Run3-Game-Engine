#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace run3::gameplay {

inline constexpr double facialAnimationLeadInSeconds = 0.2;

struct FacialPhoneme {
  double second{};
  std::string letter;
  bool patch{};
};

struct FacialAnimationDefinition {
  std::filesystem::path source;
  std::filesystem::path sound;
  std::string subtitle;
  bool patched{};
  std::vector<FacialPhoneme> phonemes;

  [[nodiscard]] double durationSeconds() const noexcept;
};

struct FacialPoseInfluence {
  int poseIndex{-1};
  float weight{};
};

struct FacialPoseSample {
  std::vector<FacialPoseInfluence> influences;
  bool active{};
};

// Loads the legacy third-party lipsync format. Node times are seconds on the
// voice cursor; the 0.2 second pre-roll belongs to playback, not to the file.
[[nodiscard]] FacialAnimationDefinition
loadFacialAnimationDefinition(const std::filesystem::path &path);

// Legacy Run3 meshes use these fixed numeric pose slots. -1 is an authored
// closed/neutral mouth and deliberately contributes no pose.
[[nodiscard]] int facialPoseIndex(std::string_view letter) noexcept;

// soundSecond is relative to the voice: negative values cover the legacy
// pre-roll. Between authored phonemes this returns a linear two-pose blend.
[[nodiscard]] FacialPoseSample
sampleFacialAnimation(const FacialAnimationDefinition &definition,
                      double soundSecond);

} // namespace run3::gameplay
