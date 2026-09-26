#pragma once

#include <memory>
#include <string>

namespace Ogre {
class SceneManager;
}

namespace run3::rendering {

struct SkySettings {
  bool enabled{};
  std::string material;
  float distance{50000.0F};
};

struct WaterSettings {
  bool enabled{};
  float height{};
  float width{100000.0F};
  float depth{100000.0F};
};

// Portable replacement boundary for retired SkyX/Hydrax ownership. The first
// implementation intentionally provides only an Ogre sky box and simple
// transparent water plane; richer effects remain optional backends.
class IEnvironment {
public:
  virtual ~IEnvironment() = default;
  virtual void setSky(const SkySettings &settings) = 0;
  virtual void setWater(const WaterSettings &settings) = 0;
  virtual void clear() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IEnvironment>
createPortableOgreEnvironment(Ogre::SceneManager &sceneManager);

} // namespace run3::rendering
