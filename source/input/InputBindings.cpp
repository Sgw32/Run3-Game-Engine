#include <run3/input/InputBindings.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace run3 {
namespace {

constexpr std::size_t index(const InputAction action) noexcept {
  return static_cast<std::size_t>(action);
}

std::string normalized(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (const unsigned char character : value) {
    if (character != ' ' && character != '_' && character != '-')
      result.push_back(static_cast<char>(std::tolower(character)));
  }
  return result;
}

} // namespace

const std::vector<InputActionDefinition> &inputActionDefinitions() noexcept {
  static const std::vector<InputActionDefinition> definitions{
      {InputAction::MoveForward, "move-forward", "Move forward", Key::W},
      {InputAction::MoveBackward, "move-backward", "Move backward", Key::S},
      {InputAction::MoveLeft, "move-left", "Move left", Key::A},
      {InputAction::MoveRight, "move-right", "Move right", Key::D},
      {InputAction::Run, "run", "Run", Key::LeftShift},
      {InputAction::Jump, "jump", "Jump / up", Key::Space},
      {InputAction::Crouch, "crouch", "Crouch / down", Key::LeftControl},
      {InputAction::Use, "use", "Use", Key::E},
      {InputAction::Inventory, "inventory", "Inventory", Key::I},
      {InputAction::Flashlight, "flashlight", "Flashlight", Key::F},
  };
  return definitions;
}

const InputActionDefinition &inputActionDefinition(const InputAction action) {
  const auto &definitions = inputActionDefinitions();
  const auto found = std::find_if(definitions.begin(), definitions.end(),
                                  [action](const auto &definition) {
                                    return definition.action == action;
                                  });
  if (found == definitions.end())
    throw std::invalid_argument("unknown input action");
  return *found;
}

std::string_view keyName(const Key key) noexcept {
  if (key >= Key::A && key <= Key::Z) {
    static constexpr std::string_view letters[]{
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
    return letters[static_cast<std::size_t>(key) - static_cast<std::size_t>(Key::A)];
  }
  if (key >= Key::Num0 && key <= Key::Num9) {
    static constexpr std::string_view digits[]{"0", "1", "2", "3", "4",
                                                "5", "6", "7", "8", "9"};
    return digits[static_cast<std::size_t>(key) - static_cast<std::size_t>(Key::Num0)];
  }
  switch (key) {
  case Key::Unknown: return "Unknown";
  case Key::Escape: return "Escape";
  case Key::Return: return "Return";
  case Key::Backspace: return "Backspace";
  case Key::Tab: return "Tab";
  case Key::Space: return "Space";
  case Key::PageUp: return "PageUp";
  case Key::PageDown: return "PageDown";
  case Key::Left: return "Left";
  case Key::Right: return "Right";
  case Key::Up: return "Up";
  case Key::Down: return "Down";
  case Key::LeftShift: return "LeftShift";
  case Key::RightShift: return "RightShift";
  case Key::LeftControl: return "LeftControl";
  case Key::RightControl: return "RightControl";
  case Key::CapsLock: return "CapsLock";
  case Key::PrintScreen: return "PrintScreen";
  case Key::F1: return "F1"; case Key::F2: return "F2";
  case Key::F3: return "F3"; case Key::F4: return "F4";
  case Key::F5: return "F5"; case Key::F6: return "F6";
  case Key::F7: return "F7"; case Key::F8: return "F8";
  case Key::F9: return "F9"; case Key::F10: return "F10";
  case Key::F11: return "F11"; case Key::F12: return "F12";
  case Key::Keypad0: return "Keypad0"; case Key::Keypad1: return "Keypad1";
  case Key::Keypad2: return "Keypad2"; case Key::Keypad3: return "Keypad3";
  case Key::Keypad4: return "Keypad4"; case Key::Keypad5: return "Keypad5";
  case Key::Keypad6: return "Keypad6"; case Key::Keypad7: return "Keypad7";
  case Key::Keypad8: return "Keypad8"; case Key::Keypad9: return "Keypad9";
  case Key::Add: return "Add"; case Key::Subtract: return "Subtract";
  case Key::Multiply: return "Multiply";
  case Key::Apostrophe: return "Apostrophe";
  case Key::Backslash: return "Backslash";
  case Key::Comma: return "Comma";
  case Key::Equals: return "Equals";
  case Key::Grave: return "Grave";
  case Key::LeftBracket: return "LeftBracket";
  case Key::Minus: return "Minus";
  case Key::Period: return "Period";
  case Key::RightBracket: return "RightBracket";
  case Key::Semicolon: return "Semicolon";
  case Key::Slash: return "Slash";
  case Key::A: case Key::B: case Key::C: case Key::D: case Key::E:
  case Key::F: case Key::G: case Key::H: case Key::I: case Key::J:
  case Key::K: case Key::L: case Key::M: case Key::N: case Key::O:
  case Key::P: case Key::Q: case Key::R: case Key::S: case Key::T:
  case Key::U: case Key::V: case Key::W: case Key::X: case Key::Y:
  case Key::Z: case Key::Num0: case Key::Num1: case Key::Num2:
  case Key::Num3: case Key::Num4: case Key::Num5: case Key::Num6:
  case Key::Num7: case Key::Num8: case Key::Num9: break;
  }
  return "Unknown";
}

std::string_view keyDisplayName(const Key key) noexcept {
  switch (key) {
  case Key::LeftShift: return "Left Shift";
  case Key::RightShift: return "Right Shift";
  case Key::LeftControl: return "Left Ctrl";
  case Key::RightControl: return "Right Ctrl";
  case Key::CapsLock: return "Caps Lock";
  case Key::PrintScreen: return "Print Screen";
  case Key::PageUp: return "Page Up";
  case Key::PageDown: return "Page Down";
  case Key::LeftBracket: return "[";
  case Key::RightBracket: return "]";
  case Key::Apostrophe: return "'";
  case Key::Backslash: return "\\";
  case Key::Comma: return ",";
  case Key::Equals: return "=";
  case Key::Grave: return "`";
  case Key::Minus: return "-";
  case Key::Period: return ".";
  case Key::Semicolon: return ";";
  case Key::Slash: return "/";
  default: return keyName(key);
  }
}

std::optional<Key> parseKey(const std::string_view name) noexcept {
  const std::string wanted = normalized(name);
  // All values in the backend-neutral enum are contiguous.
  for (int value = static_cast<int>(Key::Escape);
       value <= static_cast<int>(Key::Slash); ++value) {
    const Key candidate = static_cast<Key>(value);
    if (normalized(keyName(candidate)) == wanted ||
        normalized(keyDisplayName(candidate)) == wanted)
      return candidate;
  }
  if (wanted == "ctrl") return Key::LeftControl;
  if (wanted == "shift") return Key::LeftShift;
  if (wanted == "enter") return Key::Return;
  return std::nullopt;
}

bool isBindableKey(const Key key) noexcept {
  return key != Key::Unknown && key != Key::Escape && key != Key::PrintScreen;
}

InputBindings::InputBindings()
    : keys_(static_cast<std::size_t>(InputAction::Count), Key::Unknown) {
  for (const auto &definition : inputActionDefinitions())
    keys_[index(definition.action)] = definition.defaultKey;
}

Key InputBindings::key(const InputAction action) const {
  if (action == InputAction::Count)
    throw std::invalid_argument("unknown input action");
  return keys_.at(index(action));
}

bool InputBindings::down(const InputState &state, const InputAction action) const {
  return state.keyDown(key(action));
}

void InputBindings::rebind(const InputAction action, const Key newKey) {
  if (action == InputAction::Count || !isBindableKey(newKey))
    throw std::invalid_argument("key cannot be assigned to a gameplay action");
  const std::size_t target = index(action);
  const Key previous = keys_.at(target);
  const auto occupied = std::find(keys_.begin(), keys_.end(), newKey);
  if (occupied != keys_.end() && occupied != keys_.begin() + target)
    *occupied = previous;
  keys_[target] = newKey;
}

} // namespace run3
