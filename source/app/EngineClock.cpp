#include <run3/app/EngineClock.hpp>

#include <algorithm>
#include <stdexcept>

namespace run3 {

EngineClock::EngineClock(ClockMode mode, Duration fixedStep,
                         std::size_t maxCatchUpSteps, Duration maxFrameTime)
    : mode_(mode), fixedStep_(fixedStep), maxCatchUpSteps_(maxCatchUpSteps),
      maxFrameTime_(maxFrameTime) {
  if (fixedStep_ <= Duration::zero() || maxCatchUpSteps_ == 0 ||
      maxFrameTime_ <= Duration::zero()) {
    throw std::invalid_argument("EngineClock durations and catch-up limit must be positive");
  }
}

ClockFrame EngineClock::advance(Duration elapsed) {
  elapsed = std::clamp(elapsed, Duration::zero(), maxFrameTime_);
  if (mode_ == ClockMode::Variable) {
    return ClockFrame{elapsed, elapsed, elapsed > Duration::zero() ? 1U : 0U,
                      0.0};
  }

  accumulator_ += elapsed;
  // Render schedules such as 1/144 s accumulate a few ulps below an exact
  // 1/60 boundary. Treat only a picosecond-scale remainder as the boundary so
  // render rate cannot lose a simulation step over an otherwise exact span.
  const Duration boundaryEpsilon{1e-12};
  const auto available = static_cast<std::size_t>(
      (accumulator_ + boundaryEpsilon) / fixedStep_);
  const std::size_t steps = std::min(available, maxCatchUpSteps_);
  accumulator_ -= fixedStep_ * static_cast<double>(steps);
  if (accumulator_ < Duration::zero() &&
      accumulator_ > -boundaryEpsilon) {
    accumulator_ = Duration::zero();
  }
  if (available > maxCatchUpSteps_) {
    accumulator_ = Duration::zero();
  }
  return ClockFrame{elapsed, fixedStep_, steps,
                    accumulator_.count() / fixedStep_.count()};
}

ClockFrame EngineClock::tick(TimePoint now) {
  if (!started_) {
    reset(now);
    return advance(Duration::zero());
  }
  const Duration elapsed = now - previous_;
  previous_ = now;
  return advance(elapsed);
}

void EngineClock::reset(TimePoint now) noexcept {
  previous_ = now;
  accumulator_ = Duration::zero();
  started_ = true;
}

} // namespace run3
