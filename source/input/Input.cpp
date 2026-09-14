#include <run3/input/Input.hpp>

#include <utility>

namespace run3 {

void InputState::beginFrame() noexcept {
  mouseDeltaX_ = 0;
  mouseDeltaY_ = 0;
  wheelDeltaY_ = 0;
}

void InputState::apply(const InputEvent &event) {
  switch (event.type) {
  case InputEventType::KeyPressed:
    if (event.key != Key::Unknown) {
      keys_.insert(event.key);
    }
    break;
  case InputEventType::KeyReleased:
    keys_.erase(event.key);
    break;
  case InputEventType::MouseMoved:
    mouseX_ = event.x;
    mouseY_ = event.y;
    mouseDeltaX_ += event.deltaX;
    mouseDeltaY_ += event.deltaY;
    break;
  case InputEventType::MousePressed:
    if (event.mouseButton != MouseButton::None) {
      mouseButtons_.insert(event.mouseButton);
    }
    break;
  case InputEventType::MouseReleased:
    mouseButtons_.erase(event.mouseButton);
    break;
  case InputEventType::MouseWheel:
    wheelDeltaY_ += event.wheelY;
    break;
  case InputEventType::FocusGained:
    focused_ = true;
    break;
  case InputEventType::FocusLost:
    focused_ = false;
    keys_.clear();
    mouseButtons_.clear();
    break;
  case InputEventType::Resized:
    windowWidth_ = event.width;
    windowHeight_ = event.height;
    break;
  case InputEventType::Quit:
    quitRequested_ = true;
    break;
  case InputEventType::TextEntered:
    break;
  }
}

bool InputState::keyDown(Key key) const { return keys_.count(key) != 0; }

bool InputState::mouseButtonDown(MouseButton button) const {
  return mouseButtons_.count(button) != 0;
}

void EventQueueInput::push(InputEvent event) {
  pending_.push_back(std::move(event));
}

std::vector<InputEvent> EventQueueInput::poll() {
  state_.beginFrame();
  for (const InputEvent &event : pending_) {
    state_.apply(event);
  }
  std::vector<InputEvent> result;
  result.swap(pending_);
  return result;
}

std::vector<InputEvent> NullInput::poll() {
  state_.beginFrame();
  return {};
}

ReplayInput::ReplayInput(std::vector<Frame> frames)
    : frames_(std::move(frames)) {}

std::vector<InputEvent> ReplayInput::poll() {
  state_.beginFrame();
  if (finished()) {
    return {};
  }
  Frame events = frames_[nextFrame_++];
  for (const InputEvent &event : events) {
    state_.apply(event);
  }
  return events;
}

} // namespace run3
