#include <run3/ui/OgreButtonGui.hpp>

#include "buttonGUI.h"

#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace run3::ui {
namespace {

short pixel(const float value) {
  const float bounded = std::clamp(
      value, static_cast<float>(std::numeric_limits<short>::min()),
      static_cast<float>(std::numeric_limits<short>::max()));
  return static_cast<short>(std::lround(bounded));
}

unsigned short extent(const float value) {
  const float bounded = std::clamp(
      value, 0.0F, static_cast<float>(std::numeric_limits<unsigned short>::max()));
  return static_cast<unsigned short>(std::lround(bounded));
}

class OgreButtonGuiSystem final : public IButtonGuiSystem {
public:
  OgreButtonGuiSystem(Ogre::RenderWindow &window,
                      Ogre::SceneManager &sceneManager, std::string cameraName)
      : window_(&window), sceneManager_(&sceneManager),
        cameraName_(std::move(cameraName)) {}

  ~OgreButtonGuiSystem() override { shutdown(); }

  bool handleInput(const InputEvent &event) override {
    if (!manager_) return false;
    switch (event.type) {
    case InputEventType::MouseMoved:
      mouseX_ = event.x;
      mouseY_ = event.y;
      return manager_->injectMouseMove(event.x, event.y);
    case InputEventType::MouseWheel:
      if (event.x != mouseX_ || event.y != mouseY_)
        static_cast<void>(manager_->injectMouseMove(event.x, event.y));
      mouseX_ = event.x;
      mouseY_ = event.y;
      return manager_->injectMouseWheel(pixel(static_cast<float>(event.wheelY)));
    case InputEventType::MousePressed: {
      static_cast<void>(manager_->injectMouseMove(event.x, event.y));
      auto button = event.mouseButton;
      return manager_->injectMouseDown(button);
    }
    case InputEventType::MouseReleased: {
      static_cast<void>(manager_->injectMouseMove(event.x, event.y));
      auto button = event.mouseButton;
      return manager_->injectMouseUp(button);
    }
    case InputEventType::KeyPressed:
      return manager_->injectKeyPressed(event);
    case InputEventType::KeyReleased:
      return manager_->injectKeyReleased(event);
    default:
      return false;
    }
  }

  void update() override {
    if (!manager_) return;
    manager_->update();
    std::vector<UiCallback> pending;
    for (buttonGUI::buttonEvent *event = manager_->getEvent(); event != nullptr;
         event = manager_->getEvent()) {
      if (event->action != buttonGUI::ONCLICK ||
          event->mouseButton != MouseButton::Left || event->actionButton == nullptr)
        continue;
      const auto callback = callbacks_.find(*event->actionButton->getName());
      if (callback != callbacks_.end() && callback->second)
        pending.push_back(callback->second);
    }
    for (auto &callback : pending) callback({});
  }

  void resize() override {
    if (manager_) manager_->resetScreenResolution();
    if (backgroundPanel_) {
      backgroundPanel_->setDimensions(static_cast<Ogre::Real>(window_->getWidth()),
                                      static_cast<Ogre::Real>(window_->getHeight()));
    }
  }

  void beginComputer(const bool renderToTexture) override {
    shutdown();
    mode_ = Mode::Computer;
    renderToTexture_ = renderToTexture;
    createManager();

    // The typed MyGUI surface owns the texture for RTT computers. Legacy Lua
    // can nevertheless create buttonGUI controls; those remain a full-screen
    // compatibility layer and therefore need a live manager in both modes.
    if (renderToTexture_) return;

    Ogre::OverlayManager &overlays = Ogre::OverlayManager::getSingleton();
    backgroundOverlay_ = overlays.create("Run3ComputerButtonGuiBackground");
    backgroundOverlay_->setZOrder(1);
    backgroundPanel_ = static_cast<Ogre::PanelOverlayElement *>(
        overlays.createOverlayElement("Panel", "Run3ComputerButtonGuiBackgroundPanel"));
    backgroundPanel_->setMetricsMode(Ogre::GMM_PIXELS);
    backgroundPanel_->setPosition(0, 0);
    backgroundPanel_->setDimensions(static_cast<Ogre::Real>(window_->getWidth()),
                                    static_cast<Ogre::Real>(window_->getHeight()));
    backgroundOverlay_->add2D(backgroundPanel_);
    backgroundOverlay_->show();
  }

  void endComputer() noexcept override {
    if (mode_ != Mode::Computer) return;
    shutdown();
  }

  void beginInventory() override {
    shutdown();
    mode_ = Mode::Inventory;
    createManager();
  }

  void endInventory() noexcept override {
    if (mode_ != Mode::Inventory) return;
    shutdown();
  }

  void setDisplayMaterial(std::string material) override {
    if (backgroundPanel_ && !material.empty()) {
      backgroundPanel_->setTransparent(false);
      backgroundPanel_->setMaterialName(
          material, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
    }
  }

  bool hasContent() const noexcept override { return hasContent_; }

  void activate(const int layoutMode) override { layoutMode_ = layoutMode; }
  void deactivate() noexcept override { layoutMode_ = 0; }

  void clear() noexcept override {
    callbacks_.clear();
    if (manager_) manager_->deleteAllButtons();
    hasContent_ = false;
  }

  std::string createButton(std::string name, std::string material, Rect rect,
                           const bool dummy, UiCallback callback) override {
    if (!manager_)
      throw std::logic_error("buttonGUI button created outside an active overlay");
    if (buttonGUI::button *existing = manager_->getButton(name)) {
      existing->show(false);
      if (callback) callbacks_[name] = std::move(callback);
      return name;
    }

    transform(rect);
    buttonGUI::buttonPosition position(pixel(rect.left), pixel(rect.top));
    buttonGUI::button *created = manager_->createButton(
        name, material, position, extent(rect.width), extent(rect.height), 0,
        true, !dummy, Ogre::String{});
    hasContent_ = true;
    if (callback && !dummy) callbacks_[name] = std::move(callback);
    return *created->getName();
  }

  std::string createMeshButton(std::string name, std::string mesh, Rect rect,
                               const float zoom, std::string rotation,
                               UiCallback callback) override {
    if (!manager_)
      throw std::logic_error("buttonGUI mesh button created outside an active overlay");
    if (buttonGUI::button *existing = manager_->getButton(name)) {
      existing->show(false);
      if (callback) callbacks_[name] = std::move(callback);
      return name;
    }

    transform(rect);
    buttonGUI::buttonPosition position(pixel(rect.left), pixel(rect.top));
    buttonGUI::button *created = manager_->createButton(
        name, "BLANK", position, extent(rect.width), extent(rect.height), 0,
        true, true, Ogre::String{});
    hasContent_ = true;
    created->setMovable(true);
    buttonGUI::buttonMesh *createdMesh = created->addButtonMesh(
        name + "_mesh", mesh, 0, 0, pixel(rect.width), pixel(rect.height));
    createdMesh->setZoom(zoom);
    if (!rotation.empty())
      createdMesh->setRotation(Ogre::StringConverter::parseQuaternion(rotation));
    if (callback) callbacks_[name] = std::move(callback);
    return *created->getName();
  }

  void setPosition(const std::string_view name, const float x,
                   const float y) override {
    if (!manager_) return;
    buttonGUI::button *target = manager_->getButton(std::string(name));
    if (!target) return;
    Rect rect{x, y, 0, 0};
    transformPosition(rect);
    target->setPosition(buttonGUI::buttonPosition(pixel(rect.left), pixel(rect.top)));
  }

  std::pair<float, float>
  position(const std::string_view name) const noexcept override {
    if (!manager_) return {};
    buttonGUI::button *target = manager_->getButton(std::string(name));
    if (!target) return {};
    short x{};
    short y{};
    if (!target->getPosition(x, y)) return {};
    return {static_cast<float>(x), static_cast<float>(y)};
  }

  void setCursorVisible(const bool visible) override {
    cursorVisible_ = visible;
    if (!manager_) return;
    if (visible) manager_->showCursor();
    else manager_->hideCursor();
  }

  std::pair<float, float> cursorPosition() const noexcept override {
    if (!manager_) return {};
    short x{};
    short y{};
    manager_->getCursorButton()->getPosition(x, y);
    return {static_cast<float>(x), static_cast<float>(y)};
  }

private:
  enum class Mode { None, Computer, Inventory };

  void createManager() {
    buttonGUI::textScheme text("neuropol", 22, 1.0F, 1.0F, 1.0F, 1.0F);
    manager_ = std::make_unique<buttonGUI::buttonManager>(
        "darkInput", text, sceneManager_, cameraName_, window_);
    manager_->setCursor("cursorMat1", 32, 32, 3, 3, cursorVisible_);
  }

  void shutdown() noexcept {
    callbacks_.clear();
    manager_.reset();
    try {
      Ogre::OverlayManager &overlays = Ogre::OverlayManager::getSingleton();
      if (backgroundOverlay_) {
        backgroundOverlay_->remove2D(backgroundPanel_);
        overlays.destroy(backgroundOverlay_->getName());
      }
      if (backgroundPanel_)
        overlays.destroyOverlayElement(backgroundPanel_);
    } catch (...) {}
    backgroundOverlay_ = nullptr;
    backgroundPanel_ = nullptr;
    renderToTexture_ = false;
    hasContent_ = false;
    layoutMode_ = 0;
    mode_ = Mode::None;
  }
  void transformPosition(Rect &rect) const {
    const float width = static_cast<float>(window_->getWidth());
    const float height = static_cast<float>(window_->getHeight());
    if (layoutMode_ == 1) {
      rect.left += width * 0.5F - 320.0F;
      rect.top += height * 0.5F - 240.0F;
    } else if (layoutMode_ == 2) {
      rect.left *= width / 640.0F;
      rect.top *= height / 480.0F;
    } else if (layoutMode_ == 3) {
      rect.left *= 512.0F / 640.0F;
      rect.top *= 512.0F / 480.0F;
    }
  }

  void transform(Rect &rect) const {
    transformPosition(rect);
    if (layoutMode_ == 2) {
      rect.width *= static_cast<float>(window_->getWidth()) / 640.0F;
      rect.height *= static_cast<float>(window_->getHeight()) / 480.0F;
    } else if (layoutMode_ == 3) {
      rect.width *= 512.0F / 640.0F;
      rect.height *= 512.0F / 480.0F;
    }
  }

  Ogre::RenderWindow *window_{};
  Ogre::SceneManager *sceneManager_{};
  std::string cameraName_;
  std::unique_ptr<buttonGUI::buttonManager> manager_;
  Ogre::Overlay *backgroundOverlay_{};
  Ogre::PanelOverlayElement *backgroundPanel_{};
  std::unordered_map<std::string, UiCallback> callbacks_;
  int layoutMode_{};
  int mouseX_{};
  int mouseY_{};
  bool cursorVisible_{true};
  bool renderToTexture_{};
  bool hasContent_{};
  Mode mode_{Mode::None};
};

} // namespace

std::unique_ptr<IButtonGuiSystem>
createOgreButtonGuiSystem(Ogre::RenderWindow &window,
                          Ogre::SceneManager &sceneManager,
                          std::string cameraName) {
  return std::make_unique<OgreButtonGuiSystem>(window, sceneManager,
                                                std::move(cameraName));
}

} // namespace run3::ui
