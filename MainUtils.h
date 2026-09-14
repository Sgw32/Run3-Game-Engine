#include <CEGUI/CEGUI.h>
#include <CEGUI/CEGUISchemeManager.h>
#include <CEGUI/CEGUIWindow.h>
#include <CEGUI/CEGUIWindowManager.h>
#include <run3/input/Input.hpp>
#include <OgreCEGUIRenderer.h>

CEGUI::MouseButton convertButton(run3::MouseButton buttonID) {
  switch (buttonID) {
  case run3::MouseButton::Left:
    return CEGUI::LeftButton;

  case run3::MouseButton::Right:
    return CEGUI::RightButton;

  case run3::MouseButton::Middle:
    return CEGUI::MiddleButton;

  default:
    return CEGUI::LeftButton;
  }
}