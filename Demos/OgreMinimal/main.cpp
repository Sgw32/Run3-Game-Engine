#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreInput.h>
#include <OgreOverlaySystem.h>
#include <OgreShaderGenerator.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

constexpr std::size_t kSmokeTestFrames = 8;

struct Options {
  std::filesystem::path assetDirectory = RUN3_DEMO_DEFAULT_ASSET_DIR;
  std::size_t maximumFrames = 0;
};

Options parseOptions(int argc, char **argv) {
  Options options;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--smoke-test") {
      options.maximumFrames = kSmokeTestFrames;
    } else if (argument == "--asset-dir") {
      if (++index >= argc) {
        throw std::runtime_error("--asset-dir requires a path");
      }
      options.assetDirectory = argv[index];
    } else if (argument == "--help") {
      std::cout << "Run3 Ogre minimal demo\n"
                   "  --smoke-test       render eight frames and exit\n"
                   "  --asset-dir PATH   override the staged asset directory\n"
                   "  --help             show this help\n";
      std::exit(0);
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }

  return options;
}

class MinimalApplication final : public OgreBites::ApplicationContext,
                                 public OgreBites::InputListener {
public:
  explicit MinimalApplication(Options options)
      : OgreBites::ApplicationContext("Run3 Ogre Minimal"),
        options_(std::move(options)) {}

  bool oneTimeConfig() override {
    Ogre::RenderSystem *selectedRenderer = nullptr;
    for (Ogre::RenderSystem *renderer : getRoot()->getAvailableRenderers()) {
      if (renderer->getName().find("Direct3D11") != Ogre::String::npos) {
        selectedRenderer = renderer;
        break;
      }
    }

    if (selectedRenderer == nullptr) {
      throw Ogre::Exception(Ogre::Exception::ERR_RENDERINGAPI_ERROR,
                            "The Direct3D11 render system was not loaded",
                            "MinimalApplication::oneTimeConfig");
    }

    getRoot()->setRenderSystem(selectedRenderer);
    setConfigOptionIfPresent(*selectedRenderer, "Full Screen", "No");
    setConfigOptionIfPresent(*selectedRenderer, "VSync", "Yes");
    selectWindowSize(*selectedRenderer);
    rendererName_ = selectedRenderer->getName();
    return true;
  }

  void locateResources() override {
    const auto absoluteAssets = std::filesystem::absolute(options_.assetDirectory);
    auto &resourceManager = Ogre::ResourceGroupManager::getSingleton();
    resourceManager.addResourceLocation((absoluteAssets / "Main").string(),
                                        "FileSystem", "OgreInternal");
    resourceManager.addResourceLocation(
        (absoluteAssets / "RTShaderLib").string(), "FileSystem",
        "OgreInternal");
    resourceManager.addResourceLocation((absoluteAssets / "models").string(),
                                        "FileSystem", "General");
  }

  void setup() override {
    OgreBites::ApplicationContext::setup();
    addInputListener(this);

    sceneManager_ = getRoot()->createSceneManager();
    sceneManager_->addRenderQueueListener(getOverlaySystem());

    if (auto *shaderGenerator =
            Ogre::RTShader::ShaderGenerator::getSingletonPtr()) {
      shaderGenerator->addSceneManager(sceneManager_);
    }

    sceneManager_->setAmbientLight(Ogre::ColourValue(0.35F, 0.35F, 0.35F));

    auto *light = sceneManager_->createLight("KeyLight");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDiffuseColour(1.0F, 0.95F, 0.85F);
    auto *lightNode = sceneManager_->getRootSceneNode()->createChildSceneNode();
    lightNode->setDirection(Ogre::Vector3(-1.0F, -1.0F, -1.0F),
                            Ogre::Node::TS_WORLD);
    lightNode->attachObject(light);

    auto *entity = createDemoEntity();
    entity->setMaterialName("BaseWhite");
    modelNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
    modelNode_->attachObject(entity);

    auto *camera = sceneManager_->createCamera("MainCamera");
    camera->setNearClipDistance(1.0F);
    camera->setFarClipDistance(2000.0F);
    camera->setAutoAspectRatio(true);

    auto *cameraNode =
        sceneManager_->getRootSceneNode()->createChildSceneNode();
    cameraNode->setPosition(0.0F, 75.0F, 250.0F);
    cameraNode->lookAt(Ogre::Vector3(0.0F, 0.0F, 0.0F),
                       Ogre::Node::TS_WORLD);
    cameraNode->attachObject(camera);

    getRenderWindow()->addViewport(camera);
  }

  bool keyPressed(const OgreBites::KeyboardEvent &event) override {
    if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
      getRoot()->queueEndRendering();
      return true;
    }
    return false;
  }

  bool frameRenderingQueued(const Ogre::FrameEvent &event) override {
    if (!OgreBites::ApplicationContext::frameRenderingQueued(event)) {
      return false;
    }

    if (modelNode_ != nullptr) {
      modelNode_->yaw(Ogre::Degree(30.0F * event.timeSinceLastFrame));
    }

    ++renderedFrames_;
    if (options_.maximumFrames != 0 &&
        renderedFrames_ >= options_.maximumFrames) {
      getRoot()->queueEndRendering();
    }
    return true;
  }

  [[nodiscard]] std::size_t renderedFrames() const noexcept {
    return renderedFrames_;
  }

  [[nodiscard]] const std::string &rendererName() const noexcept {
    return rendererName_;
  }

  [[nodiscard]] const std::string &modelName() const noexcept {
    return modelName_;
  }

private:
  static void setConfigOptionIfPresent(Ogre::RenderSystem &renderer,
                                       const Ogre::String &name,
                                       const Ogre::String &value) {
    const auto &options = renderer.getConfigOptions();
    const auto option = options.find(name);
    if (option == options.end()) {
      return;
    }

    const auto &values = option->second.possibleValues;
    if (values.empty() ||
        std::find(values.begin(), values.end(), value) != values.end()) {
      renderer.setConfigOption(name, value);
    }
  }

  static void selectWindowSize(Ogre::RenderSystem &renderer) {
    const auto &options = renderer.getConfigOptions();
    const auto videoMode = options.find("Video Mode");
    if (videoMode == options.end()) {
      return;
    }

    const Ogre::String desired = "1280 x 720 @ 32-bit colour";
    const auto &values = videoMode->second.possibleValues;
    const auto selected = std::find(values.begin(), values.end(), desired);
    if (selected != values.end()) {
      renderer.setConfigOption("Video Mode", *selected);
    }
  }

  Ogre::Entity *createDemoEntity() {
#ifdef RUN3_DEMO_HAS_TLW_MODEL
    try {
      // The legacy mesh names this material, but its original material script
      // is not needed for this geometry-only smoke test.
      Ogre::MaterialManager::getSingleton().createOrRetrieve(
          "1 - Default",
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
      auto *entity = sceneManager_->createEntity("cube.mesh");
      modelName_ = "converted The Long Way media/models/cube.mesh";
      return entity;
    } catch (const Ogre::Exception &exception) {
      std::cerr << "Could not load the staged The Long Way mesh; using Ogre's "
                   "generated cube instead: "
                << exception.getDescription() << '\n';
    }
#endif
    modelName_ = "Ogre generated cube";
    return sceneManager_->createEntity(Ogre::SceneManager::PT_CUBE);
  }

  Options options_;
  Ogre::SceneManager *sceneManager_ = nullptr;
  Ogre::SceneNode *modelNode_ = nullptr;
  std::size_t renderedFrames_ = 0;
  std::string rendererName_;
  std::string modelName_;
};

} // namespace

int main(int argc, char **argv) {
  try {
    MinimalApplication application(parseOptions(argc, argv));
    application.initApp();
    application.getRoot()->startRendering();

    std::cout << "RUN3_OGRE_MINIMAL_OK"
              << " frames=" << application.renderedFrames()
              << " renderer=\"" << application.rendererName() << '"'
              << " model=\"" << application.modelName() << "\"\n";

    application.closeApp();
    return 0;
  } catch (const Ogre::Exception &exception) {
    std::cerr << "Ogre error: " << exception.getFullDescription() << '\n';
  } catch (const std::exception &exception) {
    std::cerr << "Error: " << exception.what() << '\n';
  }

  return 1;
}
