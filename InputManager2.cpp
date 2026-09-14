#include "InputManager2.h"

#include <run3/core/Log.hpp>

#include <algorithm>

namespace buttonGUI {

void InputManager2::initialise(run3::IInput &input) noexcept { input_ = &input; }

void InputManager2::capture() {
  if (input_ == nullptr) {
    run3::logWarning("InputManager2 capture requested before initialisation");
    return;
  }
  for (const run3::InputEvent &event : input_->poll()) {
    for (const auto &[name, listener] : listeners_) {
      (void)name;
      if (listener != nullptr && listener->onInputEvent(event)) {
        break;
      }
    }
  }
}

void InputManager2::addListener(run3::IInputListener *listener,
                                const std::string &instanceName) {
  if (listener != nullptr) {
    listeners_.emplace(instanceName, listener);
  }
}

void InputManager2::removeListener(const std::string &instanceName) {
  listeners_.erase(instanceName);
}

void InputManager2::removeListener(run3::IInputListener *listener) {
  const auto found = std::find_if(
      listeners_.begin(), listeners_.end(),
      [listener](const auto &entry) { return entry.second == listener; });
  if (found != listeners_.end()) {
    listeners_.erase(found);
  }
}

void InputManager2::removeAllListeners() noexcept { listeners_.clear(); }

const run3::InputState *InputManager2::state() const noexcept {
  return input_ == nullptr ? nullptr : &input_->state();
}

void InputManager2::setWindowExtents(unsigned width, unsigned height) {
  width_ = width;
  height_ = height;
}

InputManager2 *InputManager2::getSingletonPtr() {
  static InputManager2 instance;
  return &instance;
}

} // namespace buttonGUI
