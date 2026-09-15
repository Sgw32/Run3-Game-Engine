#pragma once

#include <run3/physics/Physics.hpp>

#include <optional>
#include <vector>

namespace run3::physics {

class IPhysicsQuery {
public:
  virtual ~IPhysicsQuery() = default;

  [[nodiscard]] virtual std::vector<RaycastHit>
  raycastAll(const RaycastQuery &query) const = 0;
  [[nodiscard]] virtual std::optional<RaycastHit>
  raycastClosest(const RaycastQuery &query) const;
};

class WorldPhysicsQuery final : public IPhysicsQuery {
public:
  explicit WorldPhysicsQuery(const PhysicsWorld &world) noexcept;
  [[nodiscard]] std::vector<RaycastHit>
  raycastAll(const RaycastQuery &query) const override;

private:
  const PhysicsWorld *world_{};
};

} // namespace run3::physics
