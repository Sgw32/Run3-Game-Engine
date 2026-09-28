#pragma once
#include <run3/rendering/Lighting.hpp>
#include <OgrePrerequisites.h>
#include <filesystem>
#include <memory>

namespace run3::rendering {
// Renderer ownership is confined to this adapter. Gameplay uses Lighting.hpp.
class OgreLighting final {
public:
  OgreLighting(Ogre::SceneManager &, Ogre::Camera &, Ogre::Viewport &,
               LightingSettings settings);
  ~OgreLighting();
  OgreLighting(const OgreLighting &) = delete;
  OgreLighting &operator=(const OgreLighting &) = delete;
  void createLab();
  void update(double seconds);
  void writeReport(const std::filesystem::path &, Ogre::RenderWindow &);
  static void configureMaterial(Ogre::Material &, const MaterialDescription &,
                                LightingSettings, bool tangents = false);
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace run3::rendering
