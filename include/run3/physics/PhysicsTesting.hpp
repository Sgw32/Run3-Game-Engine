#pragma once

#include <run3/physics/Physics.hpp>

namespace run3::physics {

// This deliberately separate header is test support. Runtime code must use
// UnitConversion::standard() through the normal factories.
struct testing_PhysicsTestAccess final {
  [[nodiscard]] static UnitConversion unitConversion(double metresPerGameUnit);
  [[nodiscard]] static PhysicsWorld
  createBulletWorld(const PhysicsConfig &config, UnitConversion units);
  [[nodiscard]] static PhysicsWorld
  createNullWorld(const PhysicsConfig &config, UnitConversion units);
};

namespace testing {
using PhysicsTestAccess = testing_PhysicsTestAccess;
}

} // namespace run3::physics
