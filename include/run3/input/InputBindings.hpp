#pragma once

#include <run3/input/Input.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace run3 {

// Gameplay actions are intentionally independent of platform key codes. Adding
// an action to this catalog automatically makes it available to config and the
// controls page; gameplay only asks whether the action is down.
enum class InputAction {
  MoveForward,
  MoveBackward,
  MoveLeft,
  MoveRight,
  Run,
  Jump,
  Crouch,
  Use,
  Inventory,
  Flashlight,
  Count
};

struct InputActionDefinition {
  InputAction action{};
  std::string_view id;
  std::string_view label;
  Key defaultKey{Key::Unknown};
};

[[nodiscard]] const std::vector<InputActionDefinition> &
inputActionDefinitions() noexcept;
[[nodiscard]] const InputActionDefinition &
inputActionDefinition(InputAction action);

[[nodiscard]] std::string_view keyName(Key key) noexcept;
[[nodiscard]] std::string_view keyDisplayName(Key key) noexcept;
[[nodiscard]] std::optional<Key> parseKey(std::string_view name) noexcept;
[[nodiscard]] bool isBindableKey(Key key) noexcept;

class InputBindings final {
public:
  InputBindings();

  [[nodiscard]] Key key(InputAction action) const;
  [[nodiscard]] bool down(const InputState &state, InputAction action) const;

  // Keys are unique. Rebinding to an occupied key swaps the two actions so no
  // action silently becomes inaccessible.
  void rebind(InputAction action, Key key);

private:
  std::vector<Key> keys_;
};

} // namespace run3
