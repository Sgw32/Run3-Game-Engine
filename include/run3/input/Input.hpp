#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace run3 {

enum class Key {
  Unknown,
  Escape,
  Return,
  Backspace,
  Tab,
  Space,
  PageUp,
  PageDown,
  Left,
  Right,
  Up,
  Down,
  LeftShift,
  RightShift,
  LeftControl,
  RightControl,
  CapsLock,
  PrintScreen,
  A, B, C, D, E, F, G, H, I, J, K, L, M,
  N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
  Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
  Add, Subtract, Multiply,
  Apostrophe, Backslash, Comma, Equals, Grave, LeftBracket, Minus,
  Period, RightBracket, Semicolon, Slash
};

enum class MouseButton { None, Left, Middle, Right, Extra1, Extra2 };

enum class InputEventType {
  KeyPressed,
  KeyReleased,
  TextEntered,
  MouseMoved,
  MousePressed,
  MouseReleased,
  MouseWheel,
  FocusGained,
  FocusLost,
  Resized,
  Quit
};

struct InputEvent {
  InputEventType type{};
  Key key{Key::Unknown};
  MouseButton mouseButton{MouseButton::None};
  std::string text;
  int x{};
  int y{};
  int deltaX{};
  int deltaY{};
  int wheelY{};
  unsigned width{};
  unsigned height{};
  bool repeated{};
  bool shift{};
  bool control{};
  bool alt{};
};

class InputState {
public:
  void beginFrame() noexcept;
  void apply(const InputEvent &event);

  bool keyDown(Key key) const;
  bool mouseButtonDown(MouseButton button) const;
  bool focused() const noexcept { return focused_; }
  bool quitRequested() const noexcept { return quitRequested_; }
  int mouseX() const noexcept { return mouseX_; }
  int mouseY() const noexcept { return mouseY_; }
  int mouseDeltaX() const noexcept { return mouseDeltaX_; }
  int mouseDeltaY() const noexcept { return mouseDeltaY_; }
  int wheelDeltaY() const noexcept { return wheelDeltaY_; }
  unsigned windowWidth() const noexcept { return windowWidth_; }
  unsigned windowHeight() const noexcept { return windowHeight_; }

private:
  std::unordered_set<Key> keys_;
  std::unordered_set<MouseButton> mouseButtons_;
  bool focused_{true};
  bool quitRequested_{};
  int mouseX_{};
  int mouseY_{};
  int mouseDeltaX_{};
  int mouseDeltaY_{};
  int wheelDeltaY_{};
  unsigned windowWidth_{};
  unsigned windowHeight_{};
};

class IInput {
public:
  virtual ~IInput() = default;
  virtual std::vector<InputEvent> poll() = 0;
  virtual const InputState &state() const noexcept = 0;
};

class EventQueueInput final : public IInput {
public:
  void push(InputEvent event);
  std::vector<InputEvent> poll() override;
  const InputState &state() const noexcept override { return state_; }

private:
  std::vector<InputEvent> pending_;
  InputState state_;
};

class NullInput final : public IInput {
public:
  std::vector<InputEvent> poll() override;
  const InputState &state() const noexcept override { return state_; }

private:
  InputState state_;
};

class ReplayInput final : public IInput {
public:
  using Frame = std::vector<InputEvent>;
  explicit ReplayInput(std::vector<Frame> frames);
  std::vector<InputEvent> poll() override;
  const InputState &state() const noexcept override { return state_; }
  bool finished() const noexcept { return nextFrame_ >= frames_.size(); }

private:
  std::vector<Frame> frames_;
  std::size_t nextFrame_{};
  InputState state_;
};

class IInputListener {
public:
  virtual ~IInputListener() = default;
  // Return true when the event was handled and propagation should stop.
  virtual bool onInputEvent(const InputEvent &event) = 0;
};

} // namespace run3
