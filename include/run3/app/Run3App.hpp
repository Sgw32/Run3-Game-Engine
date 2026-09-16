#pragma once

#include <run3/app/AppPaths.hpp>
#include <run3/app/Configuration.hpp>
#include <run3/app/EngineClock.hpp>
#include <run3/audio/Audio.hpp>
#include <run3/audio/MapAudio.hpp>
#include <run3/input/Input.hpp>
#include <run3/input/OgreBitesInputAdapter.hpp>
#include <run3/gameplay/PlayerController.hpp>
#include <run3/gameplay/StaticMap.hpp>

#include <OgreApplicationContext.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace Ogre {
class Camera;
class RenderWindow;
class SceneManager;
class SceneNode;
} // namespace Ogre

namespace run3 {

struct Run3AppOptions {
  AppPaths paths;
  std::string renderer;
  std::string audioBackend{"auto"};
  std::uint64_t frameLimit{};
  bool explicitContentRoot{};
  bool validateContent{};
  std::filesystem::path manifestPath;
  std::filesystem::path reportPath;
  std::filesystem::path renderFixture;
  std::string mapName;
  std::string mapQuality{"low"};
  std::string resourceProfile{"resources_low_low.cfg"};
  double playerHeightCm{180.0};
  double renderHz{};
  bool fullscreen{};
  bool startNoclip{};
  bool physicsDebug{};
};

Run3AppOptions loadRun3AppOptions(int argc, char **argv,
                                 bool validationByDefault = false);
void printRun3AppUsage();

class Run3App final : public OgreBites::ApplicationContext {
public:
  explicit Run3App(Run3AppOptions options);
  int run();

  void createRoot() override;
  bool oneTimeConfig() override;
  void locateResources() override;
  void setup() override;
  bool frameStarted(const Ogre::FrameEvent &) override { return true; }
  void windowResized(Ogre::RenderWindow *window) override;
  bool windowClosing(Ogre::RenderWindow *window) override;
  void windowClosed(Ogre::RenderWindow *window) override;
  void windowFocusChange(Ogre::RenderWindow *window) override;

private:
  void updateAspectRatio();
  void setGameplayMouseCapture(bool enabled);
  void handleInput(const std::vector<InputEvent> &events);
  void requestQuit();

  Run3AppOptions options_;
  EventQueueInput input_;
  OgreBitesInputAdapter inputAdapter_;
  EngineClock clock_;
  Ogre::SceneManager *sceneManager_{};
  Ogre::SceneNode *cubeNode_{};
  Ogre::SceneNode *cameraNode_{};
  Ogre::Camera *camera_{};
  std::unique_ptr<physics::PhysicsWorld> physicsWorld_;
  std::unique_ptr<gameplay::StaticMap> staticMap_;
  std::unique_ptr<gameplay::PlayerController> player_;
  std::unique_ptr<audio::IAudioEngine> audioEngine_;
  std::unique_ptr<audio::MapAudioRuntime> mapAudio_;
  double yawRadians_{};
  double pitchRadians_{};
  bool physicsDebug_{};
  bool gameplayMouseCapture_{};
  std::uint64_t renderedFrames_{};
  std::uint64_t simulatedSteps_{};
  bool quitRequested_{};
  bool validationFailed_{};
};

} // namespace run3
