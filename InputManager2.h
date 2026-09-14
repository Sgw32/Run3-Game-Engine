#pragma once

#include <run3/input/Input.hpp>

#include <map>
#include <string>

namespace buttonGUI {

// Transitional dispatcher retained for legacy call sites. Platform input is
// captured by Run3App; this class only fans out backend-neutral events.
class InputManager2 {
public:
  void initialise(run3::IInput &input) noexcept;
  void capture();

  void addListener(run3::IInputListener *listener,
                   const std::string &instanceName);
  void addKeyListener(run3::IInputListener *listener,
                      const std::string &instanceName) {
    addListener(listener, instanceName);
  }
  void addMouseListener(run3::IInputListener *listener,
                        const std::string &instanceName) {
    addListener(listener, instanceName);
  }
  void removeListener(const std::string &instanceName);
  void removeListener(run3::IInputListener *listener);
  void removeAllListeners() noexcept;

  const run3::InputState *state() const noexcept;
  void setWindowExtents(unsigned width, unsigned height);

  static InputManager2 *getSingletonPtr();

private:
  InputManager2() = default;
  std::map<std::string, run3::IInputListener *> listeners_;
  run3::IInput *input_{};
  unsigned width_{};
  unsigned height_{};
};

} // namespace buttonGUI
