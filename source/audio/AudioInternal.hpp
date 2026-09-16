#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

namespace run3::audio::detail {

class HandleOwner {
public:
  explicit HandleOwner(std::function<void(std::uint64_t)> release)
      : release_(std::move(release)) {}

  void release(std::uint64_t token) noexcept {
    if (release_) {
      release_(token);
    }
  }
  void detach() noexcept { release_ = {}; }

private:
  std::function<void(std::uint64_t)> release_;
};

inline std::uint64_t makeToken(std::size_t slot, std::uint32_t generation) {
  return (static_cast<std::uint64_t>(generation) << 32U) |
         static_cast<std::uint64_t>(slot + 1U);
}

inline std::size_t tokenSlot(std::uint64_t token) {
  return static_cast<std::size_t>((token & 0xffffffffULL) - 1ULL);
}

inline std::uint32_t tokenGeneration(std::uint64_t token) {
  return static_cast<std::uint32_t>(token >> 32U);
}

} // namespace run3::audio::detail
