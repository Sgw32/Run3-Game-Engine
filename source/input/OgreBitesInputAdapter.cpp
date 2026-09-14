#include <run3/input/OgreBitesInputAdapter.hpp>

#include <cctype>

namespace run3 {
namespace {

constexpr int sdlScancodeMask = 1 << 30;
constexpr int sdlCapsLock = sdlScancodeMask | 57;
constexpr int sdlPrintScreen = sdlScancodeMask | 70;
constexpr int sdlF1 = sdlScancodeMask | 58;
constexpr int sdlLeftControl = sdlScancodeMask | 224;
constexpr int sdlRightControl = sdlScancodeMask | 228;
constexpr int sdlRightShift = sdlScancodeMask | 229;

Key letterKey(int key) noexcept {
  if (key >= 'a' && key <= 'z') {
    return static_cast<Key>(static_cast<int>(Key::A) + key - 'a');
  }
  if (key >= 'A' && key <= 'Z') {
    return static_cast<Key>(static_cast<int>(Key::A) + key - 'A');
  }
  return Key::Unknown;
}

Key digitKey(int key) noexcept {
  if (key >= '0' && key <= '9') {
    return static_cast<Key>(static_cast<int>(Key::Num0) + key - '0');
  }
  return Key::Unknown;
}

} // namespace

Key translateOgreBitesKey(OgreBites::Keycode key) noexcept {
  if (const Key translated = letterKey(key); translated != Key::Unknown) {
    return translated;
  }
  if (const Key translated = digitKey(key); translated != Key::Unknown) {
    return translated;
  }
  switch (key) {
  case OgreBites::SDLK_ESCAPE: return Key::Escape;
  case OgreBites::SDLK_RETURN: return Key::Return;
  case '\b': return Key::Backspace;
  case '\t': return Key::Tab;
  case OgreBites::SDLK_SPACE: return Key::Space;
  case OgreBites::SDLK_PAGEUP: return Key::PageUp;
  case OgreBites::SDLK_PAGEDOWN: return Key::PageDown;
  case OgreBites::SDLK_LEFT: return Key::Left;
  case OgreBites::SDLK_RIGHT: return Key::Right;
  case OgreBites::SDLK_UP: return Key::Up;
  case OgreBites::SDLK_DOWN: return Key::Down;
  case OgreBites::SDLK_LSHIFT: return Key::LeftShift;
  case sdlRightShift: return Key::RightShift;
  case sdlLeftControl: return Key::LeftControl;
  case sdlRightControl: return Key::RightControl;
  case sdlCapsLock: return Key::CapsLock;
  case sdlPrintScreen: return Key::PrintScreen;
  case sdlF1 + 0: return Key::F1;
  case sdlF1 + 1: return Key::F2;
  case sdlF1 + 2: return Key::F3;
  case sdlF1 + 3: return Key::F4;
  case sdlF1 + 4: return Key::F5;
  case sdlF1 + 5: return Key::F6;
  case sdlF1 + 6: return Key::F7;
  case sdlF1 + 7: return Key::F8;
  case sdlF1 + 8: return Key::F9;
  case sdlF1 + 9: return Key::F10;
  case sdlF1 + 10: return Key::F11;
  case sdlF1 + 11: return Key::F12;
  case OgreBites::SDLK_KP_PLUS: return Key::Add;
  case OgreBites::SDLK_KP_MINUS: return Key::Subtract;
  case OgreBites::SDLK_KP_MULTIPLY: return Key::Multiply;
  case OgreBites::SDLK_KP_0: return Key::Keypad0;
  case OgreBites::SDLK_KP_1: return Key::Keypad1;
  case OgreBites::SDLK_KP_2: return Key::Keypad2;
  case OgreBites::SDLK_KP_3: return Key::Keypad3;
  case OgreBites::SDLK_KP_4: return Key::Keypad4;
  case OgreBites::SDLK_KP_5: return Key::Keypad5;
  case OgreBites::SDLK_KP_6: return Key::Keypad6;
  case OgreBites::SDLK_KP_7: return Key::Keypad7;
  case OgreBites::SDLK_KP_8: return Key::Keypad8;
  case OgreBites::SDLK_KP_9: return Key::Keypad9;
  case '/': return Key::Slash;
  case '.': return Key::Period;
  case ',': return Key::Comma;
  case '-': return Key::Minus;
  case '=': return Key::Equals;
  case '\\': return Key::Backslash;
  case '\'': return Key::Apostrophe;
  case ';': return Key::Semicolon;
  case '[': return Key::LeftBracket;
  case ']': return Key::RightBracket;
  case '`': return Key::Grave;
  default: return Key::Unknown;
  }
}

MouseButton translateOgreBitesMouseButton(unsigned char button) noexcept {
  switch (button) {
  case OgreBites::BUTTON_LEFT: return MouseButton::Left;
  case OgreBites::BUTTON_MIDDLE: return MouseButton::Middle;
  case OgreBites::BUTTON_RIGHT: return MouseButton::Right;
  case 4: return MouseButton::Extra1;
  case 5: return MouseButton::Extra2;
  default: return MouseButton::None;
  }
}

InputEvent OgreBitesInputAdapter::keyboardEvent(
    InputEventType type, const OgreBites::KeyboardEvent &event) const {
  return InputEvent{type,
                    translateOgreBitesKey(event.keysym.sym),
                    MouseButton::None,
                    {},
                    0, 0, 0, 0, 0, 0, 0,
                    event.repeat != 0,
                    (event.keysym.mod & OgreBites::KMOD_SHIFT) != 0,
                    (event.keysym.mod & OgreBites::KMOD_CTRL) != 0,
                    (event.keysym.mod & OgreBites::KMOD_ALT) != 0};
}

bool OgreBitesInputAdapter::keyPressed(const OgreBites::KeyboardEvent &event) {
  input_.push(keyboardEvent(InputEventType::KeyPressed, event));
  return true;
}

bool OgreBitesInputAdapter::keyReleased(const OgreBites::KeyboardEvent &event) {
  input_.push(keyboardEvent(InputEventType::KeyReleased, event));
  return true;
}

bool OgreBitesInputAdapter::mouseMoved(const OgreBites::MouseMotionEvent &event) {
  InputEvent translated;
  translated.type = InputEventType::MouseMoved;
  translated.x = event.x;
  translated.y = event.y;
  translated.deltaX = event.xrel;
  translated.deltaY = event.yrel;
  input_.push(std::move(translated));
  return true;
}

bool OgreBitesInputAdapter::mouseWheelRolled(
    const OgreBites::MouseWheelEvent &event) {
  InputEvent translated;
  translated.type = InputEventType::MouseWheel;
  translated.wheelY = event.y;
  input_.push(std::move(translated));
  return true;
}

bool OgreBitesInputAdapter::mousePressed(
    const OgreBites::MouseButtonEvent &event) {
  InputEvent translated;
  translated.type = InputEventType::MousePressed;
  translated.mouseButton = translateOgreBitesMouseButton(event.button);
  translated.x = event.x;
  translated.y = event.y;
  input_.push(std::move(translated));
  return true;
}

bool OgreBitesInputAdapter::mouseReleased(
    const OgreBites::MouseButtonEvent &event) {
  InputEvent translated;
  translated.type = InputEventType::MouseReleased;
  translated.mouseButton = translateOgreBitesMouseButton(event.button);
  translated.x = event.x;
  translated.y = event.y;
  input_.push(std::move(translated));
  return true;
}

bool OgreBitesInputAdapter::textInput(const OgreBites::TextInputEvent &event) {
  InputEvent translated;
  translated.type = InputEventType::TextEntered;
  if (event.chars != nullptr) {
    translated.text = event.chars;
  }
  input_.push(std::move(translated));
  return true;
}

} // namespace run3
