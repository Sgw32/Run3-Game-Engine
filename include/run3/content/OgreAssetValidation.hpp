#pragma once

#include <run3/content/AssetValidation.hpp>

#include <filesystem>

namespace Ogre {
class SceneManager;
}

namespace run3 {

void validateOgreContent(AssetReport &report, Ogre::SceneManager &sceneManager,
                         const std::filesystem::path &renderFixture);

} // namespace run3
