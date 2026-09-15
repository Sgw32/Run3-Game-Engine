#pragma once

#include <run3/physics/PhysicsQuery.hpp>

#include <cstdint>
#include <vector>

namespace run3::air3 {

using NodeId = std::uint64_t;

struct PathNode {
  NodeId id{};
  physics::Vec3 position;
};

class AirPathFind final {
public:
  explicit AirPathFind(const physics::IPhysicsQuery &physicsQuery) noexcept;
  void setNodes(std::vector<PathNode> nodes);
  [[nodiscard]] std::vector<PathNode> search(const PathNode &start,
                                             const PathNode &destination) const;

private:
  [[nodiscard]] bool lineBlocked(physics::Vec3 from,
                                 physics::Vec3 to) const;

  const physics::IPhysicsQuery *physicsQuery_{};
  std::vector<PathNode> nodes_;
};

} // namespace run3::air3
