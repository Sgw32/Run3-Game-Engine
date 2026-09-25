#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>

#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreInput.h>
#include <OgreShaderGenerator.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct Options {
  std::string renderer =
#ifdef _WIN32
      "d3d11";
#else
      "gl3plus";
#endif
  std::size_t maximumFrames = 0;
};

Options parseOptions(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--renderer") {
      if (++index >= argc) {
        throw std::runtime_error("--renderer requires d3d11 or gl3plus");
      }
      options.renderer = argv[index];
    } else if (argument == "--frames") {
      if (++index >= argc) {
        throw std::runtime_error("--frames requires a positive integer");
      }
      options.maximumFrames = std::stoul(argv[index]);
      if (options.maximumFrames == 0) {
        throw std::runtime_error("--frames must be greater than zero");
      }
    } else if (argument == "--smoke-test") {
      options.maximumFrames = 12;
    } else if (argument == "--help") {
      std::cout << "Run3 Ogre + MyGUI demo\n"
                   "  --renderer d3d11|gl3plus  choose Ogre renderer\n"
                   "  --frames N                render N frames and exit\n"
                   "  --smoke-test              render 12 frames and exit\n"
                   "  --help                    show this help\n";
      std::exit(0);
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }

  if (options.renderer != "d3d11" && options.renderer != "gl3plus") {
    throw std::runtime_error("--renderer must be d3d11 or gl3plus");
  }
  return options;
}

class DemoApplication final : public OgreBites::ApplicationContext,
                              public OgreBites::InputListener {
public:
  explicit DemoApplication(Options options)
      : OgreBites::ApplicationContext("Run3 MyGUI Ogre demo"),
        options_(std::move(options)) {}

  bool oneTimeConfig() override {
    const std::string needle =
        options_.renderer == "d3d11" ? "Direct3D11" : "OpenGL 3+";
    Ogre::RenderSystem *selected = nullptr;
    for (Ogre::RenderSystem *renderer : getRoot()->getAvailableRenderers()) {
      if (renderer->getName().find(needle) != Ogre::String::npos) {
        selected = renderer;
        break;
      }
    }
    if (selected == nullptr) {
      throw std::runtime_error("requested Ogre renderer was not loaded: " +
                               options_.renderer);
    }

    getRoot()->setRenderSystem(selected);
    setConfigOption(*selected, "Full Screen", "No");
    setConfigOption(*selected, "VSync", "Yes");
    selectWindowSize(*selected);
    rendererName_ = selected->getName();
    return true;
  }

  void locateResources() override {
    const std::filesystem::path media = RUN3_MYGUI_DEMO_MEDIA_DIR;
    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    resources.addResourceLocation((media / "OgreMain").string(),
                                  "FileSystem", "OgreInternal");
    resources.addResourceLocation((media / "RTShaderLib").string(),
                                  "FileSystem", "OgreInternal");
    resources.addResourceLocation((media / "MyGUI_Media").string(),
                                  "FileSystem", "Run3MyGUI");
  }

  void setup() override {
    OgreBites::ApplicationContext::setup();
    addInputListener(this);

    sceneManager_ = getRoot()->createSceneManager();
    sceneManager_->setAmbientLight(Ogre::ColourValue(0.22F, 0.25F, 0.32F));
    if (auto *shaderGenerator =
            Ogre::RTShader::ShaderGenerator::getSingletonPtr()) {
      shaderGenerator->addSceneManager(sceneManager_);
    }

    auto *light = sceneManager_->createLight("DemoKeyLight");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDiffuseColour(1.0F, 0.86F, 0.66F);
    auto *lightNode =
        sceneManager_->getRootSceneNode()->createChildSceneNode();
    lightNode->setDirection(Ogre::Vector3(-1.0F, -0.8F, -0.6F),
                            Ogre::Node::TS_WORLD);
    lightNode->attachObject(light);

    Ogre::Entity *cube =
        sceneManager_->createEntity(Ogre::SceneManager::PT_CUBE);
    cube->setMaterialName("BaseWhite");
    cubeNode_ = sceneManager_->getRootSceneNode()->createChildSceneNode();
    cubeNode_->setScale(0.85F, 0.85F, 0.85F);
    cubeNode_->attachObject(cube);

    auto *camera = sceneManager_->createCamera("DemoCamera");
    camera->setNearClipDistance(1.0F);
    camera->setFarClipDistance(2000.0F);
    camera->setAutoAspectRatio(true);
    auto *cameraNode =
        sceneManager_->getRootSceneNode()->createChildSceneNode();
    cameraNode->setPosition(0.0F, 55.0F, 260.0F);
    cameraNode->lookAt(Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD);
    cameraNode->attachObject(camera);
    // EGL/WSLg may advertise no useful Video Mode choices during Ogre
    // configuration and otherwise falls back to 320x200.
    getRenderWindow()->resize(960, 540);
    getRenderWindow()->addViewport(camera);

    platform_ = std::make_unique<MyGUI::OgrePlatform>();
    platform_->initialise(getRenderWindow(), sceneManager_, "Run3MyGUI",
                          "MyGUIOgreDemo.log");
    gui_ = std::make_unique<MyGUI::Gui>();
    gui_->initialise("MyGUI_Core.xml");

    window_ = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>(
        "WindowCS", MyGUI::IntCoord(24, 24, 390, 180),
        MyGUI::Align::Default, "Main");
    window_->setCaption("Run3 + Ogre 14.5.2 + MyGUI");
    auto *description = window_->createWidget<MyGUI::TextBox>(
        "TextBox", MyGUI::IntCoord(20, 42, 345, 52),
        MyGUI::Align::HStretch | MyGUI::Align::Top);
    description->setCaption(
        "MyGUI is drawn in the foreground.\nThe Ogre cube rotates behind it.");
    toggleButton_ = window_->createWidget<MyGUI::Button>(
        "Button", MyGUI::IntCoord(20, 105, 170, 34),
        MyGUI::Align::Left | MyGUI::Align::Bottom);
    toggleButton_->setCaption("Pause cube");
    toggleButton_->eventMouseButtonClick +=
        MyGUI::newDelegate(this, &DemoApplication::toggleRotation);
  }

  void shutdown() override {
    if (gui_) {
      gui_->shutdown();
      gui_.reset();
      window_ = nullptr;
      toggleButton_ = nullptr;
    }
    if (platform_) {
      platform_->shutdown();
      platform_.reset();
    }
    OgreBites::ApplicationContext::shutdown();
  }

  bool keyPressed(const OgreBites::KeyboardEvent &event) override {
    if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
      getRoot()->queueEndRendering();
      return true;
    }
    return false;
  }

  bool mouseMoved(const OgreBites::MouseMotionEvent &event) override {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    return MyGUI::InputManager::getInstance().injectMouseMove(event.x, event.y,
                                                               mouseWheel_);
  }

  bool mouseWheelRolled(const OgreBites::MouseWheelEvent &event) override {
    mouseWheel_ += event.y;
    return MyGUI::InputManager::getInstance().injectMouseMove(
        lastMouseX_, lastMouseY_, mouseWheel_);
  }

  bool mousePressed(const OgreBites::MouseButtonEvent &event) override {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    return MyGUI::InputManager::getInstance().injectMousePress(
        event.x, event.y, translateButton(event.button));
  }

  bool mouseReleased(const OgreBites::MouseButtonEvent &event) override {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    return MyGUI::InputManager::getInstance().injectMouseRelease(
        event.x, event.y, translateButton(event.button));
  }

  bool frameRenderingQueued(const Ogre::FrameEvent &event) override {
    if (!OgreBites::ApplicationContext::frameRenderingQueued(event)) {
      return false;
    }
    if (rotationEnabled_ && cubeNode_ != nullptr) {
      cubeNode_->yaw(Ogre::Degree(35.0F * event.timeSinceLastFrame));
      cubeNode_->pitch(Ogre::Degree(18.0F * event.timeSinceLastFrame));
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

private:
  static void setConfigOption(Ogre::RenderSystem &renderer,
                              const Ogre::String &name,
                              const Ogre::String &value) {
    const auto found = renderer.getConfigOptions().find(name);
    if (found == renderer.getConfigOptions().end()) {
      return;
    }
    const auto &values = found->second.possibleValues;
    if (values.empty() ||
        std::find(values.begin(), values.end(), value) != values.end()) {
      renderer.setConfigOption(name, value);
    }
  }

  static void selectWindowSize(Ogre::RenderSystem &renderer) {
    const auto found = renderer.getConfigOptions().find("Video Mode");
    if (found == renderer.getConfigOptions().end()) {
      return;
    }
    for (const Ogre::String &value : found->second.possibleValues) {
      if (value.find("1280 x 720") != Ogre::String::npos) {
        renderer.setConfigOption("Video Mode", value);
        return;
      }
    }
  }

  static MyGUI::MouseButton translateButton(unsigned char button) {
    switch (button) {
    case OgreBites::BUTTON_LEFT:
      return MyGUI::MouseButton::Left;
    case OgreBites::BUTTON_MIDDLE:
      return MyGUI::MouseButton::Middle;
    case OgreBites::BUTTON_RIGHT:
      return MyGUI::MouseButton::Right;
    default:
      return MyGUI::MouseButton::None;
    }
  }

  void toggleRotation(MyGUI::Widget *) {
    rotationEnabled_ = !rotationEnabled_;
    toggleButton_->setCaption(rotationEnabled_ ? "Pause cube" : "Resume cube");
  }

  Options options_;
  Ogre::SceneManager *sceneManager_ = nullptr;
  Ogre::SceneNode *cubeNode_ = nullptr;
  std::unique_ptr<MyGUI::OgrePlatform> platform_;
  std::unique_ptr<MyGUI::Gui> gui_;
  MyGUI::Window *window_ = nullptr;
  MyGUI::Button *toggleButton_ = nullptr;
  bool rotationEnabled_ = true;
  int lastMouseX_ = 0;
  int lastMouseY_ = 0;
  int mouseWheel_ = 0;
  std::size_t renderedFrames_ = 0;
  std::string rendererName_;
};

} // namespace

int main(int argc, char **argv) {
  try {
    DemoApplication application(parseOptions(argc, argv));
    application.initApp();
    application.getRoot()->startRendering();
    std::cout << "RUN3_MYGUI_OGRE_DEMO_OK frames="
              << application.renderedFrames() << " renderer=\""
              << application.rendererName() << "\"\n";
    application.closeApp();
    return 0;
  } catch (const Ogre::Exception &exception) {
    std::cerr << "Ogre error: " << exception.getFullDescription() << '\n';
  } catch (const std::exception &exception) {
    std::cerr << "Error: " << exception.what() << '\n';
  }
  return 1;
}
