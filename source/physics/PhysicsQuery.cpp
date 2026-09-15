#include <run3/physics/PhysicsQuery.hpp>

#include <algorithm>

namespace run3::physics {

std::optional<RaycastHit>
IPhysicsQuery::raycastClosest(const RaycastQuery &query) const {
  auto hits = raycastAll(query);
  if (hits.empty()) {
    return std::nullopt;
  }
  return *std::min_element(hits.begin(), hits.end(),
                           [](const RaycastHit &left, const RaycastHit &right) {
                             return left.fraction < right.fraction;
                           });
}

WorldPhysicsQuery::WorldPhysicsQuery(const PhysicsWorld &world) noexcept
    : world_(&world) {}

std::vector<RaycastHit>
WorldPhysicsQuery::raycastAll(const RaycastQuery &query) const {
  return world_->raycastAll(query);
}

} // namespace run3::physics
