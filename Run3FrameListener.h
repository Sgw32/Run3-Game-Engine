#pragma once

#include <run3/input/Input.hpp>

#include <OgreCamera.h>
#include <OgreFrameListener.h>
#include <OgreRenderWindow.h>
#include <OgreWindowEventUtilities.h>

// Compatibility base for code that still includes the old sample-framework
// header. Run3App owns capture and the render loop; this class has no platform
// input objects and only consumes the Run3 event boundary.
class Run3FrameListener : public Ogre::FrameListener,
                          public Ogre::WindowEventListener,
                          public run3::IInputListener {
public:
  Run3FrameListener(Ogre::RenderWindow *window, Ogre::Camera *camera,
                    bool = false, bool = false)
      : window_(window), camera_(camera) {
    if (window_ != nullptr) {
      Ogre::WindowEventUtilities::addWindowEventListener(window_, this);
    }
  }

  ~Run3FrameListener() override {
    if (window_ != nullptr) {
      Ogre::WindowEventUtilities::removeWindowEventListener(window_, this);
    }
  }

  void setInput(run3::IInput *input) noexcept { input_ = input; }
  bool frameStarted(const Ogre::FrameEvent &) override {
    if (input_ != nullptr) {
      for (const run3::InputEvent &event : input_->poll()) {
        onInputEvent(event);
      }
    }
    return !quit_;
  }
  bool frameEnded(const Ogre::FrameEvent &) override { return !quit_; }
  bool onInputEvent(const run3::InputEvent &event) override {
    if (event.type == run3::InputEventType::Quit ||
        (event.type == run3::InputEventType::KeyPressed &&
         event.key == run3::Key::Escape)) {
      quit_ = true;
      return true;
    }
    return false;
  }
  bool windowClosing(Ogre::RenderWindow *) override {
    quit_ = true;
    return true;
  }
  void windowClosed(Ogre::RenderWindow *) override { quit_ = true; }

protected:
  Ogre::RenderWindow *window_{};
  Ogre::Camera *camera_{};
  run3::IInput *input_{};
  bool quit_{};
};
