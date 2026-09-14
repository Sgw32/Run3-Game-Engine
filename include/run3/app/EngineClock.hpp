#pragma once

#include <chrono>
#include <cstddef>

namespace run3 {

enum class ClockMode { Variable, Fixed };

struct ClockFrame {
  std::chrono::duration<double> elapsed{};
  std::chrono::duration<double> simulationStep{};
  std::size_t simulationSteps{};
  double interpolationAlpha{};
};

class EngineClock {
public:
  using Duration = std::chrono::duration<double>;
  using TimePoint = std::chrono::steady_clock::time_point;

  explicit EngineClock(ClockMode mode = ClockMode::Variable,
                       Duration fixedStep = Duration{1.0 / 60.0},
                       std::size_t maxCatchUpSteps = 5,
                       Duration maxFrameTime = Duration{0.25});

  ClockFrame advance(Duration elapsed);
  ClockFrame tick(TimePoint now = std::chrono::steady_clock::now());
  void reset(TimePoint now = std::chrono::steady_clock::now()) noexcept;

private:
  ClockMode mode_;
  Duration fixedStep_;
  std::size_t maxCatchUpSteps_;
  Duration maxFrameTime_;
  Duration accumulator_{};
  TimePoint previous_{};
  bool started_{};
};

} // namespace run3
