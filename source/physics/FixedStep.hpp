#pragma once

#include <run3/physics/Physics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace run3::physics::detail {

class FixedStepAccumulator final {
public:
  explicit FixedStepAccumulator(std::uint32_t maxCatchUpSteps)
      : maxCatchUpSteps_(maxCatchUpSteps) {
    if (maxCatchUpSteps_ == 0) {
      throw std::invalid_argument("maxCatchUpSteps must be greater than zero");
    }
  }

  template <class Step>
  [[nodiscard]] StepResult advance(double frameSeconds, Step &&step) {
    if (!std::isfinite(frameSeconds) || frameSeconds < 0.0) {
      throw std::invalid_argument(
          "physics frame time must be finite and non-negative");
    }
    accumulator_ += frameSeconds;
    constexpr double fixed = PhysicsWorld::fixedStepSeconds;
    constexpr double epsilon = fixed * 1e-9;
    const auto available = static_cast<std::uint64_t>(
        std::floor((accumulator_ + epsilon) / fixed));
    const auto steps = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(available, maxCatchUpSteps_));
    for (std::uint32_t index = 0; index < steps; ++index) {
      step(fixed);
    }
    accumulator_ -= static_cast<double>(steps) * fixed;

    const std::uint64_t droppedSteps = available - steps;
    const double droppedSeconds = static_cast<double>(droppedSteps) * fixed;
    accumulator_ -= droppedSeconds;
    if (accumulator_ < 0.0 && accumulator_ > -epsilon) {
      accumulator_ = 0.0;
    }
    if (accumulator_ >= fixed) {
      accumulator_ = std::fmod(accumulator_, fixed);
    }

    return StepResult{steps, static_cast<double>(steps) * fixed,
                      droppedSeconds, accumulator_ / fixed};
  }

  [[nodiscard]] double interpolationAlpha() const noexcept {
    return accumulator_ / PhysicsWorld::fixedStepSeconds;
  }

private:
  std::uint32_t maxCatchUpSteps_;
  double accumulator_{};
};

} // namespace run3::physics::detail
