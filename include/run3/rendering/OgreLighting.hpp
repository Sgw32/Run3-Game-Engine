#pragma once
#include <run3/rendering/Lighting.hpp>
#include <OgrePrerequisites.h>
#include <filesystem>
#include <memory>
#include <string_view>

namespace run3::rendering {
// Renderer ownership is confined to this adapter. Gameplay uses Lighting.hpp.
class OgreLighting final {
public:
  OgreLighting(Ogre::SceneManager &, Ogre::Camera &, Ogre::Viewport &,
               LightingSettings settings);
  ~OgreLighting();
  OgreLighting(const OgreLighting &) = delete;
  OgreLighting &operator=(const OgreLighting &) = delete;
  void configureLegacyCompositors(const std::filesystem::path &contentRoot,
                                  const std::filesystem::path &shaderRoot,
                                  const std::filesystem::path &supportAssets,
                                  const std::filesystem::path &programCache,
                                  std::string_view textureQuality);
  void createLab();
  void update(double seconds);
  void setCompositorEnabled(std::string_view name, bool enabled);
  void setCompositorShaderParameter(std::string_view material,
                                    std::string_view parameter,
                                    std::string_view value);
  void clearCompositorEffects() noexcept;
  void writeReport(const std::filesystem::path &, Ogre::RenderWindow &);
  static void configureMaterial(Ogre::Material &, const MaterialDescription &,
                                LightingSettings, bool tangents = false);
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace run3::rendering
