#pragma once

#include <run3/physics/Physics.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Ogre {
class SceneManager;
}

namespace run3::gameplay {

struct AxisAlignedVolume {
  physics::Vec3 minimum;
  physics::Vec3 maximum;
  std::string name;

  [[nodiscard]] bool contains(physics::Vec3 point) const noexcept;
};

struct StaticMapOptions {
  std::filesystem::path contentRoot;
  std::string mapName;
  std::string quality{"low"};
  std::string resourceProfile{"resources_low_low.cfg"};
};

struct StaticMapStats {
  std::size_t visualSections{};
  std::size_t collisionSections{};
  std::size_t triangles{};
  std::size_t skippedSections{};
};

class StaticMap final {
public:
  StaticMap(Ogre::SceneManager &sceneManager, physics::PhysicsWorld &world);
  ~StaticMap();
  StaticMap(StaticMap &&) noexcept;
  StaticMap &operator=(StaticMap &&) noexcept;
  StaticMap(const StaticMap &) = delete;
  StaticMap &operator=(const StaticMap &) = delete;

  StaticMapStats load(const StaticMapOptions &options);
  void unload() noexcept;
  void setDebugDraw(bool enabled);
  void syncDynamicTransforms();

  [[nodiscard]] physics::Vec3 spawnPosition() const noexcept;
  [[nodiscard]] const std::vector<AxisAlignedVolume> &ladderVolumes() const;
  [[nodiscard]] const StaticMapStats &stats() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
