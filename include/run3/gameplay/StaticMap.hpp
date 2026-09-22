#pragma once

#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>
#include <run3/gameplay/EntityRegistry.hpp>
#include <run3/physics/Physics.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Ogre {
class SceneManager;
class Entity;
}

namespace run3::gameplay {

struct AxisAlignedVolume {
  physics::Vec3 minimum;
  physics::Vec3 maximum;
  std::string name;

  [[nodiscard]] bool contains(physics::Vec3 point) const noexcept;
};

struct StaticMapOptions {
  const AppPaths *paths{};
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

struct NamedObjectBounds {
  physics::Vec3 centre;
  physics::Vec3 halfExtents;
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
  [[nodiscard]] const content::MapDefinition &definition() const;
  [[nodiscard]] EntityRegistry &registry();
  [[nodiscard]] const EntityRegistry &registry() const;
  [[nodiscard]] bool setNamedObjectVisible(std::string_view name,
                                           std::optional<bool> visible);
  [[nodiscard]] std::optional<NamedObjectBounds>
  namedObjectBounds(std::string_view name) const;
  [[nodiscard]] Ogre::Entity *namedObject(std::string_view name) const;
  [[nodiscard]] bool setNamedObjectPhysicsEnabled(std::string_view name,
                                                  bool enabled);
  void applyCompatibleMaterials(Ogre::Entity &entity,
                                std::string_view overrideMaterial = {});

private:
  class Impl;
  std::unique_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
