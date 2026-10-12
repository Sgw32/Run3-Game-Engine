#pragma once

#include <run3/physics/Physics.hpp>
#include <run3/rendering/Lighting.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Ogre {
class SceneManager;
}

namespace run3::gameplay {

enum class BlockSide { North, South, East, West };

struct BlockPort {
  BlockSide side{BlockSide::North};
  // Authoring coordinates: X, horizontal depth, vertical height.
  physics::Vec3 positionCm;
  double widthCm{400.0};
  double heightCm{320.0};
};

struct BlockLight {
  bool enabled{};
  physics::Vec3 positionCm;
  std::array<float, 3> colour{1.0F, 0.95F, 0.8F};
  float rangeCm{1800.0F};
};

struct BlockDefinition {
  std::string id;
  std::string mesh;
  std::string style;
  // Width, depth and height in authoring centimetres.
  physics::Vec3 dimensionsCm;
  std::vector<BlockPort> ports;
  BlockLight light;
  bool termination{};
};

struct BlockManifest {
  unsigned unitsPerMeter{100};
  unsigned gridCm{500};
  double biomeScaleCm{24000.0};
  std::vector<BlockDefinition> blocks;
};

[[nodiscard]] BlockManifest
loadBlockManifest(const std::filesystem::path &path);
[[nodiscard]] BlockSide opposite(BlockSide side) noexcept;
// Computes an Ogre-space root for `attached` by aligning its socket with an
// already placed socket. This is the common placement rule for differently
// sized blocks and for the house's 500 cm floor-height transition.
[[nodiscard]] physics::Vec3 alignedBlockOrigin(
    physics::Vec3 placedBlockOrigin, const BlockPort &placed,
    const BlockPort &attached);

struct BlockCell {
  std::int64_t x{};
  std::int64_t z{};

  friend bool operator==(const BlockCell &left, const BlockCell &right) {
    return left.x == right.x && left.z == right.z;
  }
};

struct BlockPlacement {
  BlockCell anchor;
  const BlockDefinition *definition{};
  unsigned widthCells{1};
  unsigned depthCells{1};
  // Logical Ogre-space root in centimetres, before floating-origin rebasing.
  physics::Vec3 rootCm;
};

// Pure deterministic selection boundary. It uses low-frequency seeded biome
// regions and a second coordinate hash for variation inside each biome.
class SeededBlockGenerator final {
public:
  SeededBlockGenerator(const BlockManifest &manifest, std::uint64_t seed,
                       std::string originBlock = {});

  [[nodiscard]] const BlockDefinition &blockFor(BlockCell cell) const;
  [[nodiscard]] BlockPlacement placementForCell(BlockCell cell) const;
  [[nodiscard]] std::string_view biomeFor(BlockCell cell) const;
  [[nodiscard]] std::uint64_t seed() const noexcept;
  [[nodiscard]] double cellSizeCm() const noexcept;

private:
  const BlockManifest *manifest_{};
  std::uint64_t seed_{};
  std::size_t originIndex_{};
  std::vector<std::string> styles_;
  std::vector<std::size_t> smallIndices_;
  std::vector<std::size_t> largeIndices_;
  std::vector<std::size_t> houseIndices_;
};

struct SemiInfiniteWorldOptions {
  std::filesystem::path manifest;
  std::uint64_t seed{};
  std::string originBlock;
  double horizontalLoadRadiusCm{20000.0};
  double verticalLoadRadiusCm{3000.0};
  double rebaseThresholdCm{50000.0};
  // A 100,000,000 cm side is exactly 1,000 km. Logical coordinates are int64,
  // so this can be raised without changing renderer or physics precision.
  std::int64_t logicalExtentCm{100000000};
  unsigned lightBudget{3};
  bool terminators{true};
  rendering::LightingSettings lighting;
};

struct SemiInfiniteWorldStats {
  std::size_t activeBlocks{};
  std::size_t activeTerminators{};
  std::size_t activeLights{};
  std::size_t pooledBlocks{};
  std::size_t pooledTerminators{};
  std::size_t generatedCells{};
  std::uint64_t completedStreamPlans{};
  std::uint64_t rebases{};
  std::int64_t originXcm{};
  std::int64_t originYcm{};
  std::int64_t originZcm{};
};

class SemiInfiniteWorld final {
public:
  SemiInfiniteWorld(Ogre::SceneManager &sceneManager,
                    physics::PhysicsWorld &physics,
                    SemiInfiniteWorldOptions options);
  ~SemiInfiniteWorld();
  SemiInfiniteWorld(SemiInfiniteWorld &&) noexcept;
  SemiInfiniteWorld &operator=(SemiInfiniteWorld &&) noexcept;
  SemiInfiniteWorld(const SemiInfiniteWorld &) = delete;
  SemiInfiniteWorld &operator=(const SemiInfiniteWorld &) = delete;

  // Returns a grid-aligned local translation that should be subtracted from
  // every physics body and non-streamed scene root before applyOriginShift().
  [[nodiscard]] std::optional<physics::Vec3>
  requestedOriginShift(physics::Vec3 localPlayerPosition) const;
  void applyOriginShift(physics::Vec3 offset);
  void update(physics::Vec3 localPlayerPosition);
  void setDebugDraw(bool enabled);
  void unload() noexcept;

  [[nodiscard]] const SemiInfiniteWorldStats &stats() const noexcept;
  [[nodiscard]] const BlockManifest &manifest() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
