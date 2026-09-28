#pragma once
#include <run3/rendering/Lighting.hpp>
#include <OgrePrerequisites.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
namespace run3::rendering {
class LightingPost final {
public:
  LightingPost(Ogre::SceneManager &, Ogre::Camera &, Ogre::Viewport &, LightingSettings);
  ~LightingPost();
  static void configureDeferred(Ogre::Material &, Surface, bool lit = true,
                                bool reflection = false);
  nlohmann::json report() const;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}
