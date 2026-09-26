#pragma once

#include <run3/input/Input.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace run3::ui {

enum class Context { Main, Hud, Computer };
enum class WidgetType { Panel, Text, Button, Edit, CheckBox, List };
enum class UiEvent { Click, Change, Submit };

struct Rect {
  float left{};
  float top{};
  float width{};
  float height{};
};

struct WidgetHandle {
  Context context{Context::Main};
  std::uint64_t id{};
  std::uint32_t generation{};

  [[nodiscard]] bool valid() const noexcept { return id != 0 && generation != 0; }
  [[nodiscard]] std::string token() const;
  [[nodiscard]] static std::optional<WidgetHandle> parse(std::string_view token);
  friend bool operator==(const WidgetHandle &left,
                         const WidgetHandle &right) noexcept {
    return left.context == right.context && left.id == right.id &&
           left.generation == right.generation;
  }
  friend bool operator!=(const WidgetHandle &left,
                         const WidgetHandle &right) noexcept {
    return !(left == right);
  }
};

struct WidgetSpec {
  Context context{Context::Main};
  WidgetHandle parent;
  WidgetType type{WidgetType::Panel};
  std::string name;
  Rect rect;
  std::string text;
};

using UiCallback = std::function<void(std::string)>;

// Capability-limited facade used by ScriptEngine. It deliberately contains no
// MyGUI/Ogre object or ownership type.
class IScriptUiFacade {
public:
  virtual ~IScriptUiFacade() = default;
  virtual WidgetHandle loadLayout(Context context, std::string_view key) = 0;
  virtual WidgetHandle createWidget(const WidgetSpec &spec) = 0;
  virtual std::optional<WidgetHandle> findWidget(Context context,
                                                  std::string_view name) const = 0;
  virtual void destroyWidget(WidgetHandle handle) = 0;
  virtual void setText(WidgetHandle handle, std::string text) = 0;
  virtual void setVisible(WidgetHandle handle, bool visible) = 0;
  virtual void setEnabled(WidgetHandle handle, bool enabled) = 0;
  virtual void setProperty(WidgetHandle handle, std::string_view name,
                           std::string value) = 0;
  virtual void focus(WidgetHandle handle) = 0;
  virtual std::uint64_t setCallback(WidgetHandle handle, UiEvent event,
                                    UiCallback callback) = 0;
  virtual void clearCallback(std::uint64_t callback) noexcept = 0;
  virtual void clearScriptCallbacks() noexcept = 0;
};

enum class MenuActionKind {
  Resume,
  NewGame,
  SelectChapter,
  ApplySettings,
  Quit
};

struct MenuAction {
  MenuActionKind kind{MenuActionKind::Resume};
  std::string chapter;
  std::string resolution;
  double verticalFov{};
};

using MenuActionHandler = std::function<void(const MenuAction &)>;

class IUiSystem : public IScriptUiFacade {
public:
  ~IUiSystem() override = default;

  // Returns true only when UI consumed the event. Run3 input crosses into the
  // backend here and nowhere else.
  virtual bool handleInput(const InputEvent &event) = 0;
  virtual void resize(unsigned width, unsigned height, float dpiScale) = 0;
  virtual void update(float seconds) = 0;

  virtual void showMenu(bool visible) = 0;
  [[nodiscard]] virtual bool menuVisible() const noexcept = 0;
  virtual void setHudVisible(bool visible) = 0;
  virtual void setSubtitle(std::string text, double seconds) = 0;
  virtual void setConsoleVisible(bool visible) = 0;
  virtual void appendConsole(std::string line) = 0;
  virtual void setLoading(bool visible, std::string text) = 0;
  virtual void setInventoryEnabled(bool enabled) = 0;

  // Only one owner can have a computer context. The returned name is the Ogre
  // texture to put on that owner's screen; it is not an owning renderer type.
  [[nodiscard]] virtual std::string activateComputer(std::string ownerKey) = 0;
  virtual void deactivateComputer(std::string_view ownerKey) noexcept = 0;
  [[nodiscard]] virtual bool computerActive() const noexcept = 0;
  [[nodiscard]] virtual bool computerActiveFor(std::string_view ownerKey) const noexcept = 0;
  virtual void renderComputerSurface() = 0;
  virtual void resetMapState() noexcept = 0;
};

[[nodiscard]] std::string_view contextName(Context context) noexcept;
[[nodiscard]] std::optional<Context> parseContext(std::string_view name) noexcept;
[[nodiscard]] std::string_view widgetTypeName(WidgetType type) noexcept;
[[nodiscard]] std::optional<WidgetType> parseWidgetType(std::string_view name) noexcept;
[[nodiscard]] std::optional<UiEvent> parseUiEvent(std::string_view name) noexcept;

} // namespace run3::ui
