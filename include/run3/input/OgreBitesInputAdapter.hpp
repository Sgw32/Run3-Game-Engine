#pragma once

#include <run3/input/Input.hpp>

#include <OgreInput.h>

namespace run3 {

Key translateOgreBitesKey(OgreBites::Keycode key) noexcept;
MouseButton translateOgreBitesMouseButton(unsigned char button) noexcept;

class OgreBitesInputAdapter final : public OgreBites::InputListener {
public:
  explicit OgreBitesInputAdapter(EventQueueInput &input) : input_(input) {}

  bool keyPressed(const OgreBites::KeyboardEvent &event) override;
  bool keyReleased(const OgreBites::KeyboardEvent &event) override;
  bool mouseMoved(const OgreBites::MouseMotionEvent &event) override;
  bool mouseWheelRolled(const OgreBites::MouseWheelEvent &event) override;
  bool mousePressed(const OgreBites::MouseButtonEvent &event) override;
  bool mouseReleased(const OgreBites::MouseButtonEvent &event) override;
  bool textInput(const OgreBites::TextInputEvent &event) override;

private:
  InputEvent keyboardEvent(InputEventType type,
                           const OgreBites::KeyboardEvent &event) const;
  EventQueueInput &input_;
};

} // namespace run3
