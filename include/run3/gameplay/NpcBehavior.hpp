#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace run3::gameplay {

// Stable behavior values replace the original process-global RangeRandom
// calls.  NPCs remain visually distinct, while saves, replays and threaded
// updates cannot change their animation or look schedules.
inline std::uint64_t npcBehaviorHash(std::string_view text,
                                     std::uint64_t hash =
                                         1469598103934665603ULL) noexcept {
  for (const unsigned char byte : text) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

inline std::uint64_t stableNpcBehaviorSeed(std::string_view name,
                                           std::string_view source,
                                           std::uint64_t authoredOrder) noexcept {
  std::uint64_t result = npcBehaviorHash(name);
  result = npcBehaviorHash(source, result);
  for (unsigned shift = 0; shift != 64; shift += 8) {
    result ^= (authoredOrder >> shift) & 0xffU;
    result *= 1099511628211ULL;
  }
  return result;
}

inline std::uint64_t mixNpcBehavior(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

inline double npcBehaviorUnit(std::uint64_t seed, std::uint64_t salt) noexcept {
  // Use the high 53 bits so the conversion is exactly representable as a
  // double in [0, 1).
  return static_cast<double>(mixNpcBehavior(seed ^ salt) >> 11U) *
         (1.0 / 9007199254740992.0);
}

inline double npcAnimationTimeMultiplier(std::uint64_t seed) noexcept {
  // This is the legacy npc_neutral uniqueMult range: approximately 0.9-1.1.
  return 0.9 + 0.2 * npcBehaviorUnit(seed, 0x414e494d4154494fULL);
}

enum class NpcLookPhase { PlayerFirst, Forward, PlayerSecond, Elsewhere };
enum class NpcLookTarget { Player, Forward, Elsewhere };

struct NpcLookState {
  NpcLookPhase phase{NpcLookPhase::Forward};
  double elapsedSeconds{};
  std::uint64_t cycle{};
  bool engaged{};
};

struct NpcLookDecision {
  NpcLookTarget target{NpcLookTarget::Forward};
  double elsewhereYawDegrees{};
};

inline double npcLookPhaseDuration(std::uint64_t seed,
                                   NpcLookPhase phase,
                                   std::uint64_t cycle) noexcept {
  const auto phaseIndex = static_cast<std::uint64_t>(phase);
  const double variation = npcBehaviorUnit(
      seed, 0x4c4f4f4b54494d45ULL ^ (cycle * 17U + phaseIndex));
  switch (phase) {
  case NpcLookPhase::PlayerFirst:
  case NpcLookPhase::PlayerSecond:
  case NpcLookPhase::Elsewhere:
    return 4.5 + variation; // About five seconds.
  case NpcLookPhase::Forward:
    return 9.0 + 2.0 * variation; // About ten seconds.
  }
  return 5.0;
}

inline NpcLookPhase nextNpcLookPhase(NpcLookPhase phase) noexcept {
  switch (phase) {
  case NpcLookPhase::PlayerFirst: return NpcLookPhase::Forward;
  case NpcLookPhase::Forward: return NpcLookPhase::PlayerSecond;
  case NpcLookPhase::PlayerSecond: return NpcLookPhase::Elsewhere;
  case NpcLookPhase::Elsewhere: return NpcLookPhase::PlayerFirst;
  }
  return NpcLookPhase::PlayerFirst;
}

inline NpcLookDecision updateNpcLookState(NpcLookState &state,
                                          bool playerNear,
                                          double seconds,
                                          std::uint64_t seed) noexcept {
  if (!playerNear) {
    state.phase = NpcLookPhase::Forward;
    state.elapsedSeconds = 0.0;
    state.engaged = false;
    return {};
  }
  if (!state.engaged) {
    state.engaged = true;
    state.phase = NpcLookPhase::PlayerFirst;
    state.elapsedSeconds = 0.0;
    // A later encounter receives a different, but still deterministic,
    // timing schedule and elsewhere direction.
    ++state.cycle;
  }

  state.elapsedSeconds += std::max(0.0, seconds);
  // The loop also makes a delayed/throttled frame advance the explicit state
  // machine correctly instead of skipping or changing its random sequence.
  for (unsigned transitions = 0; transitions != 16; ++transitions) {
    const double duration =
        npcLookPhaseDuration(seed, state.phase, state.cycle);
    if (state.elapsedSeconds < duration) break;
    state.elapsedSeconds -= duration;
    if (state.phase == NpcLookPhase::Elsewhere) ++state.cycle;
    state.phase = nextNpcLookPhase(state.phase);
  }

  if (state.phase == NpcLookPhase::PlayerFirst ||
      state.phase == NpcLookPhase::PlayerSecond)
    return {NpcLookTarget::Player, 0.0};
  if (state.phase == NpcLookPhase::Forward)
    return {NpcLookTarget::Forward, 0.0};

  const double side = npcBehaviorUnit(
                          seed, 0x4c4f4f4b53494445ULL ^ state.cycle) < 0.5
                          ? -1.0
                          : 1.0;
  const double angle = 20.0 + 20.0 * npcBehaviorUnit(
      seed, 0x4c4f4f4b414e474cULL ^ state.cycle);
  return {NpcLookTarget::Elsewhere, side * angle};
}

} // namespace run3::gameplay
