#include <run3/ui/OgreButtonGui.hpp>

#include "buttonGUI.h"

#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgrePanelOverlayElement.h>
#include <OgreFontManager.h>
#include <OgreMaterialManager.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>
#include <OgreTextAreaOverlayElement.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <sstream>
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
      const Ogre::Real width = renderToTexture_ ? 1024.0F
                                                : window_->getWidth();
      const Ogre::Real height = renderToTexture_ ? 768.0F
                                                 : window_->getHeight();
      backgroundPanel_->setDimensions(width, height);
    }
  }

  void beginComputer(const bool renderToTexture) override {
    shutdown();
    mode_ = Mode::Computer;
    renderToTexture_ = renderToTexture;
    createManager();

    Ogre::OverlayManager &overlays = Ogre::OverlayManager::getSingleton();
    backgroundOverlay_ = overlays.create("Run3ComputerButtonGuiBackground");
    backgroundOverlay_->setZOrder(1);
    backgroundPanel_ = static_cast<Ogre::PanelOverlayElement *>(
        overlays.createOverlayElement("Panel", "Run3ComputerButtonGuiBackgroundPanel"));
    backgroundPanel_->setMetricsMode(Ogre::GMM_PIXELS);
    backgroundPanel_->setPosition(0, 0);
    backgroundPanel_->setDimensions(renderToTexture_ ? 1024.0F : window_->getWidth(),
                                    renderToTexture_ ? 768.0F : window_->getHeight());
    backgroundPanel_->setMaterialName(
        "Run3/BlackMat",
        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
    displayText_ = static_cast<Ogre::TextAreaOverlayElement *>(
        overlays.createOverlayElement("TextArea", "Run3ComputerDisplayText"));
    displayText_->setMetricsMode(Ogre::GMM_PIXELS);
    displayText_->setPosition(18, 18);
    displayText_->setDimensions(988, 732);
    Ogre::FontPtr displayFont = Ogre::FontManager::getSingleton().getByName(
        "Run3ComputerDisplayFont",
        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
    if (!displayFont) {
      displayFont = Ogre::FontManager::getSingleton().create(
          "Run3ComputerDisplayFont",
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
      displayFont->setType(Ogre::FT_TRUETYPE);
      displayFont->setSource("console.ttf");
      displayFont->setTrueTypeSize(24.0F);
      displayFont->setTrueTypeResolution(96);
      displayFont->load();
    }
    displayText_->setFontName(
        displayFont->getName(), displayFont->getGroup());
    displayText_->setCharHeight(19);
    displayText_->setColour(Ogre::ColourValue(1.0F, 0.72F, 0.42F, 1.0F));
    backgroundPanel_->addChild(displayText_);
    backgroundOverlay_->add2D(backgroundPanel_);
    if (renderToTexture_) backgroundOverlay_->hide();
    else backgroundOverlay_->show();
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
      try {
        if (!Ogre::MaterialManager::getSingleton().getByName(
                material,
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
          return;
        backgroundPanel_->setTransparent(false);
        backgroundPanel_->setMaterialName(
            material,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
      } catch (...) {
        // Legacy computer scripts treat unavailable display assets as a
        // recoverable no-op; the rest of the interface must remain usable.
      }
    }
  }

  void updateComputerDisplay(
      const std::string_view command,
      const std::vector<std::string> &arguments) override {
    if (mode_ != Mode::Computer || displayText_ == nullptr) return;
    if (command == "dMaterialSet" && !arguments.empty()) {
      setDisplayMaterial(arguments.front());
    } else if (command == "dClear") {
      computerLines_.clear();
    } else if (command == "dCompPrint") {
      computerLines_.push_back(computerPrefix_ +
          (arguments.empty() ? std::string{} : arguments.front()));
    } else if (command == "dPrefix") {
      computerPrefix_ = arguments.empty() ? std::string{} : arguments.front();
    } else if (command == "dSetMaxLines" && !arguments.empty()) {
      try {
        computerMaxLines_ = std::clamp<std::size_t>(
            static_cast<std::size_t>(std::stoul(arguments.front())), 1, 256);
      } catch (...) {}
    } else if (command == "dSetFontSize" && !arguments.empty()) {
      try {
        float size = std::stof(arguments.front());
        if (size <= 1.0F) size *= 768.0F;
        displayText_->setCharHeight(std::clamp(size, 1.0F, 96.0F));
      } catch (...) {}
    } else if (command == "dSetFont" && !arguments.empty()) {
      try {
        Ogre::FontPtr font = Ogre::FontManager::getSingleton().getByName(
            arguments.front(),
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        if (font) displayText_->setFontName(font->getName(), font->getGroup());
      } catch (...) {}
    } else if (command == "dSetDimensions" && arguments.size() >= 4) {
      try {
        const float left = std::stof(arguments[0]);
        const float top = std::stof(arguments[1]);
        const float right = std::stof(arguments[2]);
        const float bottom = std::stof(arguments[3]);
        displayText_->setPosition(left * 1024.0F, top * 768.0F);
        displayText_->setDimensions(std::max(1.0F, (right - left) * 1024.0F),
                                    std::max(1.0F, (bottom - top) * 768.0F));
      } catch (...) {}
    }
    while (computerLines_.size() > computerMaxLines_)
      computerLines_.erase(computerLines_.begin());
    std::ostringstream caption;
    for (std::size_t index = 0; index < computerLines_.size(); ++index) {
      if (index != 0) caption << '\n';
      caption << computerLines_[index];
    }
    displayText_->setCaption(caption.str());
  }

  void setRenderPassVisible(const bool visible) override {
    if (!renderToTexture_) return;
    if (backgroundOverlay_ != nullptr) {
      if (visible) backgroundOverlay_->show();
      else backgroundOverlay_->hide();
    }
    if (manager_ != nullptr) manager_->setOverlayVisibility(visible);
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
    if (renderToTexture_) manager_->setOverlayVisibility(false);
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
    if (renderToTexture_) manager_->setOverlayVisibility(false);
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
    if (renderToTexture_) manager_->setOverlayVisibility(false);
  }

  void shutdown() noexcept {
    callbacks_.clear();
    manager_.reset();
    try {
      Ogre::OverlayManager &overlays = Ogre::OverlayManager::getSingleton();
      if (backgroundOverlay_) {
        if (backgroundPanel_ && displayText_)
          backgroundPanel_->removeChild(displayText_->getName());
        backgroundOverlay_->remove2D(backgroundPanel_);
        overlays.destroy(backgroundOverlay_->getName());
      }
      if (displayText_) overlays.destroyOverlayElement(displayText_);
      if (backgroundPanel_)
        overlays.destroyOverlayElement(backgroundPanel_);
    } catch (...) {}
    backgroundOverlay_ = nullptr;
    backgroundPanel_ = nullptr;
    displayText_ = nullptr;
    computerLines_.clear();
    computerPrefix_.clear();
    computerMaxLines_ = 32;
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
  Ogre::TextAreaOverlayElement *displayText_{};
  std::unordered_map<std::string, UiCallback> callbacks_;
  int layoutMode_{};
  int mouseX_{};
  int mouseY_{};
  bool cursorVisible_{true};
  bool renderToTexture_{};
  bool hasContent_{};
  std::vector<std::string> computerLines_;
  std::string computerPrefix_;
  std::size_t computerMaxLines_{32};
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
