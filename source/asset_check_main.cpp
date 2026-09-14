#include <run3/app/Run3App.hpp>
#include <run3/core/Log.hpp>

#include <OgreException.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <utility>

int main(int argc, char **argv) {
  try {
    run3::Run3AppOptions options = run3::loadRun3AppOptions(argc, argv, true);
    if (options.renderer.empty()) {
      return EXIT_SUCCESS;
    }
    run3::Run3App application(std::move(options));
    return application.run();
  } catch (const Ogre::Exception &error) {
    run3::logError("Ogre asset validation error: " + error.getFullDescription());
  } catch (const std::exception &error) {
    run3::logError(std::string("run3_asset_check error: ") + error.what());
  }
  return EXIT_FAILURE;
}
