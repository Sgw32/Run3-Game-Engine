#pragma once

#include <run3/ui/Ui.hpp>

#include <filesystem>
#include <memory>

namespace Ogre {
class RenderWindow;
class SceneManager;
}

namespace run3::ui {

// This is the one platform boundary allowed to mention Ogre. MyGUI remains
// private to its implementation translation unit.
[[nodiscard]] std::unique_ptr<IUiSystem> createMyGuiUiSystem(
    Ogre::RenderWindow &window, Ogre::SceneManager &sceneManager,
    const std::filesystem::path &userLogDirectory,
    MenuActionHandler actionHandler, float dpiScale = 1.0F);

} // namespace run3::ui
