#pragma once

#include <run3/ui/Ui.hpp>

#include <memory>
#include <string>

namespace Ogre {
class RenderWindow;
class SceneManager;
}

namespace run3::ui {

// Runtime boundary for the original Ogre-overlay buttonGUI implementation.
// This deliberately does not expose or depend on MyGUI.
class IButtonGuiSystem : public IButtonGuiFacade {
public:
  ~IButtonGuiSystem() override = default;

  virtual bool handleInput(const InputEvent &event) = 0;
  virtual void update() = 0;
  virtual void resize() = 0;
  virtual void beginComputer(bool renderToTexture) = 0;
  virtual void endComputer() noexcept = 0;
  virtual void beginInventory() = 0;
  virtual void endInventory() noexcept = 0;
  virtual void setDisplayMaterial(std::string material) = 0;
  // RTT computers may still run original scripts which construct full-screen
  // buttonGUI controls. The owner uses this to route input to that legacy
  // compatibility layer only after such controls actually exist.
  [[nodiscard]] virtual bool hasContent() const noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IButtonGuiSystem>
createOgreButtonGuiSystem(Ogre::RenderWindow &window,
                          Ogre::SceneManager &sceneManager,
                          std::string cameraName);

} // namespace run3::ui
