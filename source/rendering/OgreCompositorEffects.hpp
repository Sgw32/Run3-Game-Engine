#pragma once

#include <OgreCompositorInstance.h>
#include <OgrePrerequisites.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace run3::rendering {

class OgreCompositorEffects final {
public:
  explicit OgreCompositorEffects(Ogre::Viewport &viewport);
  ~OgreCompositorEffects();
  OgreCompositorEffects(const OgreCompositorEffects &) = delete;
  OgreCompositorEffects &operator=(const OgreCompositorEffects &) = delete;

  void configure(const std::filesystem::path &contentRoot,
                 const std::filesystem::path &shaderRoot,
                 const std::filesystem::path &supportAssets,
                 const std::filesystem::path &programCache,
                 std::string_view textureQuality);
  void setEnabled(std::string_view name, bool enabled);
  void setShaderParameter(std::string_view program,
                          std::string_view parameter,
                          std::string_view value);
  void update(double seconds) noexcept;
  void clear() noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace run3::rendering
