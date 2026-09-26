#include <run3/ui/OgreMyGui.hpp>

#include <run3/ui/UiModel.hpp>

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>

#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace run3::ui {
namespace {

MyGUI::KeyCode keyCode(const Key key) noexcept {
  using M = MyGUI::KeyCode;
  switch (key) {
  case Key::Escape: return M::Escape;
  case Key::Return: return M::Return;
  case Key::Backspace: return M::Backspace;
  case Key::Tab: return M::Tab;
  case Key::Space: return M::Space;
  case Key::PageUp: return M::PageUp;
  case Key::PageDown: return M::PageDown;
  case Key::Left: return M::ArrowLeft;
  case Key::Right: return M::ArrowRight;
  case Key::Up: return M::ArrowUp;
  case Key::Down: return M::ArrowDown;
  case Key::LeftShift: return M::LeftShift;
  case Key::RightShift: return M::RightShift;
  case Key::LeftControl: return M::LeftControl;
  case Key::RightControl: return M::RightControl;
  case Key::CapsLock: return M::Capital;
  case Key::PrintScreen: return M::SysRq;
  case Key::F1: return M::F1;
  case Key::F2: return M::F2;
  case Key::F3: return M::F3;
  case Key::F4: return M::F4;
  case Key::F5: return M::F5;
  case Key::F6: return M::F6;
  case Key::F7: return M::F7;
  case Key::F8: return M::F8;
  case Key::F9: return M::F9;
  case Key::F10: return M::F10;
  case Key::F11: return M::F11;
  case Key::F12: return M::F12;
  case Key::A: return M::A;
  case Key::B: return M::B;
  case Key::C: return M::C;
  case Key::D: return M::D;
  case Key::E: return M::E;
  case Key::F: return M::F;
  case Key::G: return M::G;
  case Key::H: return M::H;
  case Key::I: return M::I;
  case Key::J: return M::J;
  case Key::K: return M::K;
  case Key::L: return M::L;
  case Key::M: return M::M;
  case Key::N: return M::N;
  case Key::O: return M::O;
  case Key::P: return M::P;
  case Key::Q: return M::Q;
  case Key::R: return M::R;
  case Key::S: return M::S;
  case Key::T: return M::T;
  case Key::U: return M::U;
  case Key::V: return M::V;
  case Key::W: return M::W;
  case Key::X: return M::X;
  case Key::Y: return M::Y;
  case Key::Z: return M::Z;
  case Key::Num0: return M::Zero;
  case Key::Num1: return M::One;
  case Key::Num2: return M::Two;
  case Key::Num3: return M::Three;
  case Key::Num4: return M::Four;
  case Key::Num5: return M::Five;
  case Key::Num6: return M::Six;
  case Key::Num7: return M::Seven;
  case Key::Num8: return M::Eight;
  case Key::Num9: return M::Nine;
  case Key::Keypad0: return M::Numpad0;
  case Key::Keypad1: return M::Numpad1;
  case Key::Keypad2: return M::Numpad2;
  case Key::Keypad3: return M::Numpad3;
  case Key::Keypad4: return M::Numpad4;
  case Key::Keypad5: return M::Numpad5;
  case Key::Keypad6: return M::Numpad6;
  case Key::Keypad7: return M::Numpad7;
  case Key::Keypad8: return M::Numpad8;
  case Key::Keypad9: return M::Numpad9;
  case Key::Add: return M::Add;
  case Key::Subtract: return M::Subtract;
  case Key::Multiply: return M::Multiply;
  case Key::Apostrophe: return M::Apostrophe;
  case Key::Backslash: return M::Backslash;
  case Key::Comma: return M::Comma;
  case Key::Equals: return M::Equals;
  case Key::Grave: return M::Grave;
  case Key::LeftBracket: return M::LeftBracket;
  case Key::Minus: return M::Minus;
  case Key::Period: return M::Period;
  case Key::RightBracket: return M::RightBracket;
  case Key::Semicolon: return M::Semicolon;
  case Key::Slash: return M::Slash;
  case Key::Unknown: return M::None;
  }
  return M::None;
}

MyGUI::MouseButton mouseButton(const MouseButton button) noexcept {
  switch (button) {
  case MouseButton::Left: return MyGUI::MouseButton::Left;
  case MouseButton::Middle: return MyGUI::MouseButton::Middle;
  case MouseButton::Right: return MyGUI::MouseButton::Right;
  default: return MyGUI::MouseButton::None;
  }
}

std::vector<MyGUI::Char> decodeUtf8(std::string_view text) {
  std::vector<MyGUI::Char> result;
  for (std::size_t index = 0; index < text.size();) {
    const unsigned char first = static_cast<unsigned char>(text[index++]);
    std::uint32_t value = first;
    unsigned continuation = 0;
    if ((first & 0xE0U) == 0xC0U) { value = first & 0x1FU; continuation = 1; }
    else if ((first & 0xF0U) == 0xE0U) { value = first & 0x0FU; continuation = 2; }
    else if ((first & 0xF8U) == 0xF0U) { value = first & 0x07U; continuation = 3; }
    else if ((first & 0x80U) != 0) { value = 0xFFFDU; }
    for (unsigned part = 0; part < continuation; ++part) {
      if (index >= text.size() ||
          (static_cast<unsigned char>(text[index]) & 0xC0U) != 0x80U) {
        value = 0xFFFDU;
        break;
      }
      value = (value << 6U) |
              (static_cast<unsigned char>(text[index++]) & 0x3FU);
    }
    result.push_back(static_cast<MyGUI::Char>(value));
  }
  return result;
}

std::string skin(const WidgetType type) {
  switch (type) {
  case WidgetType::Panel: return "Panel";
  case WidgetType::Text: return "TextBox";
  case WidgetType::Button: return "Button";
  case WidgetType::Edit: return "EditBox";
  case WidgetType::CheckBox: return "CheckBox";
  case WidgetType::List: return "ListBox";
  }
  return "Panel";
}

std::string className(const WidgetType type) {
  switch (type) {
  case WidgetType::Panel: return "Widget";
  case WidgetType::Text: return "TextBox";
  case WidgetType::Button: return "Button";
  case WidgetType::Edit: return "EditBox";
  case WidgetType::CheckBox: return "Button";
  case WidgetType::List: return "ListBox";
  }
  return "Widget";
}

void setCaption(MyGUI::Widget &widget, const WidgetType type,
                const std::string &text) {
  switch (type) {
  case WidgetType::Text: widget.castType<MyGUI::TextBox>()->setCaption(text); break;
  case WidgetType::Button:
  case WidgetType::CheckBox: widget.castType<MyGUI::Button>()->setCaption(text); break;
  case WidgetType::Edit: widget.castType<MyGUI::EditBox>()->setCaption(text); break;
  case WidgetType::Panel:
  case WidgetType::List: break;
  }
}

std::string caption(MyGUI::Widget &widget, const WidgetType type) {
  switch (type) {
  case WidgetType::Text: return widget.castType<MyGUI::TextBox>()->getCaption();
  case WidgetType::Button:
  case WidgetType::CheckBox: return widget.castType<MyGUI::Button>()->getCaption();
  case WidgetType::Edit: return widget.castType<MyGUI::EditBox>()->getCaption();
  case WidgetType::Panel:
  case WidgetType::List: return {};
  }
  return {};
}

class VisibilityGuard final {
public:
  VisibilityGuard(const std::vector<MyGUI::Widget *> &widgets,
                  const Context visibleContext,
                  const std::unordered_map<MyGUI::Widget *, WidgetHandle> &handles)
      : widgets_(widgets) {
    visibility_.reserve(widgets.size());
    for (MyGUI::Widget *widget : widgets_) {
      visibility_.push_back(widget->getVisible());
      const auto found = handles.find(widget);
      widget->setVisible(found != handles.end() &&
                         found->second.context == visibleContext);
    }
  }
  ~VisibilityGuard() {
    for (std::size_t index = 0; index < widgets_.size(); ++index)
      widgets_[index]->setVisible(visibility_[index]);
  }
  VisibilityGuard(const VisibilityGuard &) = delete;
  VisibilityGuard &operator=(const VisibilityGuard &) = delete;

private:
  std::vector<MyGUI::Widget *> widgets_;
  std::vector<bool> visibility_;
};

} // namespace

class MyGuiUiSystem final : public IUiSystem {
public:
  struct Entry {
    WidgetSpec spec;
    MyGUI::Widget *widget{};
  };

  MyGuiUiSystem(Ogre::RenderWindow &window, Ogre::SceneManager &sceneManager,
                const std::filesystem::path &logDirectory,
                MenuActionHandler actionHandler, const float dpiScale)
      : window_(&window), actions_(std::move(actionHandler)), dpiScale_(dpiScale) {
    platform_ = std::make_unique<MyGUI::OgrePlatform>();
    const std::filesystem::path log = logDirectory / "mygui.log";
    platform_->initialise(&window, &sceneManager, "Run3MyGUI", log.string());
    gui_ = std::make_unique<MyGUI::Gui>();
    gui_->setDpiScale(dpiScale_);
    gui_->initialise("MyGUI_Core.xml");
    resize(window.getWidth(), window.getHeight(), dpiScale_);
    buildMainMenu();
    buildHud();
    computerTexture_ = MyGUI::RenderManager::getInstance().createTexture(
        "Run3ComputerSurfaceTexture");
    computerTexture_->createManual(1024, 768, MyGUI::TextureUsage::RenderTarget,
                                   MyGUI::PixelFormat::R8G8B8A8);
    if (computerTexture_->getRenderTarget() == nullptr)
      throw std::runtime_error("MyGUI Ogre backend did not create an RTT surface");
  }

  ~MyGuiUiSystem() override {
    clearScriptCallbacks();
    entries_.clear();
    reverse_.clear();
    if (computerTexture_ != nullptr) {
      MyGUI::RenderManager::getInstance().destroyTexture(computerTexture_);
      computerTexture_ = nullptr;
    }
    if (gui_) { gui_->shutdown(); gui_.reset(); }
    if (platform_) { platform_->shutdown(); platform_.reset(); }
  }

  bool handleInput(const InputEvent &event) override {
    if (event.type == InputEventType::FocusLost) {
      MyGUI::InputManager::getInstance().resetMouseCaptureWidget();
      MyGUI::InputManager::getInstance().resetKeyFocusWidget();
      return false;
    }
    if (event.type == InputEventType::Resized) {
      resize(event.width, event.height, dpiScale_);
      return false;
    }
    if (!computerActive() && !menuVisible_ && !consoleVisible_) return false;

    const Context context = computerActive() ? Context::Computer : Context::Main;
    const auto roots = rootWidgets();
    VisibilityGuard guard(roots, context, reverse_);
    auto &input = MyGUI::InputManager::getInstance();
    const auto [x, y] = inputPosition(event, context);
    bool handled = false;
    switch (event.type) {
    case InputEventType::MouseMoved:
      mouseX_ = x; mouseY_ = y;
      handled = input.injectMouseMove(x, y, wheel_);
      break;
    case InputEventType::MouseWheel:
      wheel_ += event.wheelY;
      handled = input.injectMouseMove(mouseX_, mouseY_, wheel_);
      break;
    case InputEventType::MousePressed:
      handled = input.injectMousePress(x, y, mouseButton(event.mouseButton));
      break;
    case InputEventType::MouseReleased:
      handled = input.injectMouseRelease(x, y, mouseButton(event.mouseButton));
      break;
    case InputEventType::KeyPressed:
      handled = input.injectKeyPress(keyCode(event.key));
      break;
    case InputEventType::KeyReleased:
      handled = input.injectKeyRelease(keyCode(event.key));
      break;
    case InputEventType::TextEntered:
      for (const MyGUI::Char character : decodeUtf8(event.text))
        handled = input.injectKeyPress(MyGUI::KeyCode::None, character) || handled;
      break;
    default: break;
    }
    return handled || computerActive() || menuVisible_ || consoleVisible_;
  }

  void resize(const unsigned width, const unsigned height, const float dpiScale) override {
    if (width == 0 || height == 0) return;
    dpiScale_ = dpiScale;
    layout_ = calculateLayout(width, height, dpiScale_);
    MyGUI::LayerManager::getInstance().resizeView(
        MyGUI::IntSize(static_cast<int>(width), static_cast<int>(height)));
    for (auto &[id, entry] : entries_) {
      static_cast<void>(id);
      applyCoordinates(entry);
    }
  }

  void update(const float seconds) override {
    if (subtitleSeconds_ > 0.0) {
      subtitleSeconds_ -= seconds;
      if (subtitleSeconds_ <= 0.0) setSubtitle({}, 0.0);
    }
  }

  void showMenu(const bool visible) override {
    menuVisible_ = visible;
    if (auto root = findWidget(Context::Main, "menu.root"))
      widget(*root).setVisible(visible);
    if (!visible) MyGUI::InputManager::getInstance().resetKeyFocusWidget();
  }
  bool menuVisible() const noexcept override { return menuVisible_; }

  void setHudVisible(const bool visible) override {
    hudVisible_ = visible;
    for (auto &[id, entry] : entries_) {
      static_cast<void>(id);
      if (entry.spec.context == Context::Hud) entry.widget->setVisible(visible);
    }
    if (!consoleVisible_) setWidgetVisible("hud.console", false);
    if (!loadingVisible_) setWidgetVisible("hud.loading", false);
    if (!inventoryEnabled_) setWidgetVisible("hud.inventory", false);
  }

  void setSubtitle(std::string text, const double seconds) override {
    subtitleSeconds_ = seconds;
    if (auto handle = findWidget(Context::Hud, "hud.subtitle")) {
      setCaption(widget(*handle), WidgetType::Text, text);
      widget(*handle).setVisible(hudVisible_ && !text.empty());
    }
  }
  void setConsoleVisible(const bool visible) override {
    consoleVisible_ = visible;
    setWidgetVisible("hud.console", visible && hudVisible_);
  }
  void appendConsole(std::string line) override {
    if (!consoleText_.empty()) consoleText_ += '\n';
    consoleText_ += std::move(line);
    constexpr std::size_t maximum = 8192;
    if (consoleText_.size() > maximum)
      consoleText_.erase(0, consoleText_.size() - maximum);
    if (auto handle = findWidget(Context::Hud, "hud.console"))
      setCaption(widget(*handle), WidgetType::Text, consoleText_);
  }
  void setLoading(const bool visible, std::string text) override {
    loadingVisible_ = visible;
    if (auto handle = findWidget(Context::Hud, "hud.loading")) {
      setCaption(widget(*handle), WidgetType::Text, text);
      widget(*handle).setVisible(visible && hudVisible_);
    }
  }
  void setInventoryEnabled(const bool enabled) override {
    inventoryEnabled_ = enabled;
    setWidgetVisible("hud.inventory", enabled && hudVisible_);
  }

  WidgetHandle loadLayout(const Context context, const std::string_view key) override {
    const bool approved =
        (context == Context::Main && (key == "main" || key == "default")) ||
        (context == Context::Hud && (key == "hud" || key == "default")) ||
        (context == Context::Computer && (key == "computer" || key == "default"));
    if (!approved)
      throw std::invalid_argument("unapproved scoped UI layout key '" +
                                  std::string(key) + "'");
    const std::string rootName = context == Context::Main ? "menu.root"
                               : context == Context::Hud ? "hud.root"
                                                         : "computer.root";
    const auto root = registry_.find(context, rootName);
    if (!root) throw std::logic_error("requested UI context is not active");
    return *root;
  }

  WidgetHandle createWidget(const WidgetSpec &spec) override {
    if (spec.context == Context::Computer && !computerActive())
      throw std::logic_error("computer UI context is not active");
    const WidgetHandle handle = registry_.add(spec);
    MyGUI::Widget *parent = spec.parent.valid() ? &widget(spec.parent) : nullptr;
    MyGUI::Widget *created = nullptr;
    const MyGUI::IntCoord coordinate = coordinateFor(spec);
    if (parent != nullptr) {
      created = parent->createWidgetT(className(spec.type), skin(spec.type),
                                      coordinate, MyGUI::Align::Default,
                                      spec.name);
    } else {
      created = MyGUI::Gui::getInstance().createWidgetT(
          className(spec.type), skin(spec.type), coordinate,
          MyGUI::Align::Default, "Main", spec.name);
    }
    setCaption(*created, spec.type, spec.text);
    entries_.emplace(handle.id, Entry{spec, created});
    reverse_.emplace(created, handle);
    bindNativeEvents(handle, *created);
    return handle;
  }

  std::optional<WidgetHandle> findWidget(const Context context,
                                          const std::string_view name) const override {
    return registry_.find(context, name);
  }

  void destroyWidget(const WidgetHandle handle) override {
    MyGUI::Widget *target = &widget(handle);
    const std::vector<WidgetHandle> removed = registry_.erase(handle);
    for (const WidgetHandle item : removed) {
      const auto found = entries_.find(item.id);
      if (found != entries_.end()) {
        reverse_.erase(found->second.widget);
        entries_.erase(found);
      }
    }
    MyGUI::Gui::getInstance().destroyWidget(target);
  }
  void setText(const WidgetHandle handle, std::string text) override {
    const WidgetType type = registry_.require(handle).type;
    setCaption(widget(handle), type, text);
  }
  void setVisible(const WidgetHandle handle, const bool visible) override {
    widget(handle).setVisible(visible);
  }
  void setEnabled(const WidgetHandle handle, const bool enabled) override {
    widget(handle).setEnabled(enabled);
  }
  void setProperty(const WidgetHandle handle, const std::string_view name,
                   std::string value) override {
    static constexpr std::array<std::string_view, 7> approved{
        "Alpha", "TextAlign", "FontHeight", "NeedMouse", "NeedKey",
        "StateSelected", "Position"};
    if (std::find(approved.begin(), approved.end(), name) == approved.end())
      throw std::invalid_argument("unsafe MyGUI property '" + std::string(name) + "'");
    if (name == "Position") {
      std::istringstream input(value);
      int x{};
      int y{};
      if (!(input >> x >> y))
        throw std::invalid_argument("Position expects 'x y'");
      widget(handle).setPosition(x, y);
    } else {
      widget(handle).setProperty(std::string(name), std::move(value));
    }
  }
  void focus(const WidgetHandle handle) override {
    MyGUI::InputManager::getInstance().setKeyFocusWidget(&widget(handle));
  }
  std::uint64_t setCallback(const WidgetHandle handle, const UiEvent event,
                            UiCallback callback) override {
    return registry_.bind(handle, event, std::move(callback));
  }
  void clearCallback(const std::uint64_t callback) noexcept override {
    registry_.unbind(callback);
  }
  void clearScriptCallbacks() noexcept override { registry_.clearCallbacks(); }

  std::string activateComputer(std::string ownerKey) override {
    if (ownerKey.empty()) throw std::invalid_argument("computer owner key is empty");
    if (computerActive()) {
      if (computerOwner_ == ownerKey) return computerTexture_->getName();
      deactivateComputer(computerOwner_);
    }
    computerOwner_ = std::move(ownerKey);
    registry_.beginContext(Context::Computer, computerOwner_);
    WidgetSpec background{Context::Computer, {}, WidgetType::Panel,
                          "computer.root", {0, 0, 1024, 768}, {}};
    const WidgetHandle root = createWidget(background);
    createWidget({Context::Computer, root, WidgetType::Text,
                  "computer.title", {36, 28, 952, 54},
                  "RUN3 COMPUTER / " + computerOwner_});
    createWidget({Context::Computer, root, WidgetType::Text,
                  "computer.status", {36, 100, 952, 600},
                  "Computer connected. Lua UI context is ready."});
    setVisible(root, false);
    return computerTexture_->getName();
  }

  void deactivateComputer(const std::string_view ownerKey) noexcept override {
    if (!computerActive() || ownerKey != computerOwner_) return;
    try {
      if (const auto root = registry_.find(Context::Computer, "computer.root"))
        destroyWidget(*root);
    } catch (...) {}
    registry_.clearContext(Context::Computer);
    computerOwner_.clear();
    MyGUI::InputManager::getInstance().resetMouseCaptureWidget();
    MyGUI::InputManager::getInstance().resetKeyFocusWidget();
  }
  bool computerActive() const noexcept override { return !computerOwner_.empty(); }
  bool computerActiveFor(const std::string_view ownerKey) const noexcept override {
    return computerOwner_ == ownerKey;
  }

  void renderComputerSurface() override {
    if (!computerActive()) return;
    MyGUI::IRenderTarget *target = computerTexture_->getRenderTarget();
    const auto roots = rootWidgets();
    VisibilityGuard guard(roots, Context::Computer, reverse_);
    target->begin();
    try {
      MyGUI::LayerManager::getInstance().renderToTarget(target, true);
      target->end();
    } catch (...) {
      target->end();
      throw;
    }
  }

  void resetMapState() noexcept override {
    if (computerActive()) deactivateComputer(computerOwner_);
    clearScriptCallbacks();
    setSubtitle({}, 0.0);
    setInventoryEnabled(false);
    setLoading(false, {});
  }

private:
  MyGUI::Widget &widget(const WidgetHandle handle) const {
    static_cast<void>(registry_.require(handle));
    const auto found = entries_.find(handle.id);
    if (found == entries_.end() || found->second.widget == nullptr)
      throw std::logic_error("UI backend lost a live widget");
    return *found->second.widget;
  }

  std::vector<MyGUI::Widget *> rootWidgets() const {
    std::vector<MyGUI::Widget *> result;
    for (const auto &[id, entry] : entries_) {
      static_cast<void>(id);
      if (!entry.spec.parent.valid()) result.push_back(entry.widget);
    }
    return result;
  }

  MyGUI::IntCoord coordinateFor(const WidgetSpec &spec) const {
    const bool child = spec.parent.valid();
    const float offsetX = child || spec.context == Context::Computer ? 0.0F
                                                                     : layout_.offsetX;
    const float offsetY = child || spec.context == Context::Computer ? 0.0F
                                                                     : layout_.offsetY;
    const float scale = spec.context == Context::Computer ? 1.0F : layout_.scale;
    return {static_cast<int>(std::lround(offsetX + spec.rect.left * scale)),
            static_cast<int>(std::lround(offsetY + spec.rect.top * scale)),
            static_cast<int>(std::lround(spec.rect.width * scale)),
            static_cast<int>(std::lround(spec.rect.height * scale))};
  }
  void applyCoordinates(Entry &entry) {
    entry.widget->setCoord(coordinateFor(entry.spec));
  }

  std::pair<int, int> inputPosition(const InputEvent &event,
                                    const Context context) const {
    if (context != Context::Computer || window_->getWidth() == 0 ||
        window_->getHeight() == 0) return {event.x, event.y};
    return {event.x * 1024 / static_cast<int>(window_->getWidth()),
            event.y * 768 / static_cast<int>(window_->getHeight())};
  }

  void bindNativeEvents(const WidgetHandle handle, MyGUI::Widget &created) {
    if (created.isType<MyGUI::Button>()) {
      created.castType<MyGUI::Button>()->eventMouseButtonClick +=
          MyGUI::newDelegate(this, &MyGuiUiSystem::onClick);
    }
    if (created.isType<MyGUI::EditBox>()) {
      auto *edit = created.castType<MyGUI::EditBox>();
      edit->eventEditTextChange += MyGUI::newDelegate(this, &MyGuiUiSystem::onEditChange);
      edit->eventEditSelectAccept += MyGUI::newDelegate(this, &MyGuiUiSystem::onEditSubmit);
    }
    static_cast<void>(handle);
  }

  void onClick(MyGUI::Widget *sender) {
    const auto found = reverse_.find(sender);
    if (found == reverse_.end()) return;
    const std::string &name = registry_.require(found->second).name;
    if (name == "menu.resume") {
      showMenu(false);
      emitAction(MenuAction{MenuActionKind::Resume, {}, {}, 0.0});
    }
    else if (name == "menu.new")
      emitAction(MenuAction{MenuActionKind::NewGame, {}, {}, 0.0});
    else if (name == "menu.chapter.open") showPage("chapter");
    else if (name == "menu.options.open") showPage("options");
    else if (name == "menu.back.chapter" || name == "menu.back.options") showPage("main");
    else if (name == "menu.chapter.start") {
      MenuAction action{MenuActionKind::SelectChapter, {}, {}, 0.0};
      if (const auto field = findWidget(Context::Main, "menu.chapter.name"))
        action.chapter = caption(widget(*field), WidgetType::Edit);
      emitAction(action);
    } else if (name == "menu.options.apply") {
      MenuAction action{MenuActionKind::ApplySettings, {}, {}, 0.0};
      if (const auto resolution = findWidget(Context::Main, "menu.options.resolution"))
        action.resolution = caption(widget(*resolution), WidgetType::Edit);
      if (const auto fov = findWidget(Context::Main, "menu.options.fov")) {
        try { action.verticalFov = std::stod(caption(widget(*fov), WidgetType::Edit)); }
        catch (...) { action.verticalFov = 0.0; }
      }
      emitAction(action);
    } else if (name == "menu.quit")
      emitAction(MenuAction{MenuActionKind::Quit, {}, {}, 0.0});
    registry_.emit(found->second, UiEvent::Click,
                   caption(*sender, registry_.require(found->second).type));
  }
  void onEditChange(MyGUI::EditBox *sender) {
    const auto found = reverse_.find(sender);
    if (found != reverse_.end())
      registry_.emit(found->second, UiEvent::Change, sender->getCaption());
  }
  void onEditSubmit(MyGUI::EditBox *sender) {
    const auto found = reverse_.find(sender);
    if (found != reverse_.end())
      registry_.emit(found->second, UiEvent::Submit, sender->getCaption());
  }
  void emitAction(const MenuAction &action) { if (actions_) actions_(action); }

  void setWidgetVisible(const std::string_view name, const bool visible) {
    if (const auto handle = findWidget(Context::Hud, name))
      widget(*handle).setVisible(visible);
  }
  void showPage(const std::string_view page) {
    const std::array<std::string_view, 3> names{"main", "chapter", "options"};
    for (const auto name : names) {
      if (const auto handle = findWidget(Context::Main,
                                         "menu.page." + std::string(name)))
        widget(*handle).setVisible(name == page);
    }
  }

  void buildMainMenu() {
    const WidgetHandle root = createWidget(
        {Context::Main, {}, WidgetType::Panel, "menu.root",
         {430, 70, 420, 580}, {}});
    const WidgetHandle title = createWidget(
        {Context::Main, root, WidgetType::Text, "menu.title",
         {35, 24, 350, 55}, "THE LONG WAY / RUN3"});
    static_cast<void>(title);
    const WidgetHandle main = createWidget(
        {Context::Main, root, WidgetType::Panel, "menu.page.main",
         {25, 90, 370, 450}, {}});
    const std::array<std::pair<std::string_view, std::string_view>, 5> buttons{{
        {"menu.resume", "Continue"}, {"menu.new", "New game"},
        {"menu.chapter.open", "Chapters"}, {"menu.options.open", "Options"},
        {"menu.quit", "Quit"}}};
    for (std::size_t index = 0; index < buttons.size(); ++index)
      createWidget({Context::Main, main, WidgetType::Button,
                    std::string(buttons[index].first),
                    {35, 20.0F + static_cast<float>(index) * 68.0F, 300, 48},
                    std::string(buttons[index].second)});

    const WidgetHandle chapter = createWidget(
        {Context::Main, root, WidgetType::Panel, "menu.page.chapter",
         {25, 90, 370, 450}, {}});
    createWidget({Context::Main, chapter, WidgetType::Text, "menu.chapter.label",
                  {30, 30, 310, 40}, "Chapter map name"});
    createWidget({Context::Main, chapter, WidgetType::Edit, "menu.chapter.name",
                  {30, 86, 310, 44}, "tlwcao"});
    createWidget({Context::Main, chapter, WidgetType::Button, "menu.chapter.start",
                  {30, 160, 310, 48}, "Start chapter"});
    createWidget({Context::Main, chapter, WidgetType::Button, "menu.back.chapter",
                  {30, 350, 310, 48}, "Back"});

    const WidgetHandle options = createWidget(
        {Context::Main, root, WidgetType::Panel, "menu.page.options",
         {25, 90, 370, 450}, {}});
    createWidget({Context::Main, options, WidgetType::Text, "menu.options.res.label",
                  {30, 20, 310, 35}, "Resolution (WIDTHxHEIGHT)"});
    createWidget({Context::Main, options, WidgetType::Edit,
                  "menu.options.resolution", {30, 58, 310, 42}, "1280x720"});
    createWidget({Context::Main, options, WidgetType::Text, "menu.options.fov.label",
                  {30, 125, 310, 35}, "Vertical field of view"});
    createWidget({Context::Main, options, WidgetType::Edit,
                  "menu.options.fov", {30, 163, 310, 42}, "75"});
    createWidget({Context::Main, options, WidgetType::Button,
                  "menu.options.apply", {30, 245, 310, 48}, "Apply"});
    createWidget({Context::Main, options, WidgetType::Button,
                  "menu.back.options", {30, 350, 310, 48}, "Back"});
    showPage("main");
    showMenu(false);
  }

  void buildHud() {
    const WidgetHandle root = createWidget(
        {Context::Hud, {}, WidgetType::Panel, "hud.root", {0, 0, 1280, 720}, {}});
    createWidget({Context::Hud, root, WidgetType::Text, "hud.crosshair",
                  {627, 340, 26, 40}, "+"});
    createWidget({Context::Hud, root, WidgetType::Text, "hud.subtitle",
                  {180, 625, 920, 60}, {}});
    createWidget({Context::Hud, root, WidgetType::Text, "hud.console",
                  {20, 20, 800, 320}, {}});
    createWidget({Context::Hud, root, WidgetType::Text, "hud.loading",
                  {440, 330, 400, 60}, "Loading..."});
    createWidget({Context::Hud, root, WidgetType::Text, "hud.inventory",
                  {940, 70, 300, 560}, "Inventory"});
    setSubtitle({}, 0.0);
    setConsoleVisible(false);
    setLoading(false, {});
    setInventoryEnabled(false);
  }

  Ogre::RenderWindow *window_{};
  MenuActionHandler actions_;
  UiRegistry registry_;
  std::unique_ptr<MyGUI::OgrePlatform> platform_;
  std::unique_ptr<MyGUI::Gui> gui_;
  MyGUI::ITexture *computerTexture_{};
  std::unordered_map<std::uint64_t, Entry> entries_;
  std::unordered_map<MyGUI::Widget *, WidgetHandle> reverse_;
  ScaledLayout layout_;
  float dpiScale_{1.0F};
  std::string computerOwner_;
  std::string consoleText_;
  double subtitleSeconds_{};
  int mouseX_{};
  int mouseY_{};
  int wheel_{};
  bool menuVisible_{};
  bool hudVisible_{true};
  bool consoleVisible_{};
  bool loadingVisible_{};
  bool inventoryEnabled_{};
};

std::unique_ptr<IUiSystem> createMyGuiUiSystem(
    Ogre::RenderWindow &window, Ogre::SceneManager &sceneManager,
    const std::filesystem::path &userLogDirectory,
    MenuActionHandler actionHandler, const float dpiScale) {
  return std::make_unique<MyGuiUiSystem>(window, sceneManager, userLogDirectory,
                                        std::move(actionHandler), dpiScale);
}

} // namespace run3::ui
