#include <OgreApplicationContext.h>
#include <OgreCamera.h>
#include <OgreColourValue.h>
#include <OgreEntity.h>
#include <OgreException.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreMath.h>
#include <OgreOverlaySystem.h>
#include <OgreRenderSystem.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreStringConverter.h>
#include <OgreViewport.h>
#include <OgreVector.h>

#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
#include <OgreRTShaderSystem.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace {

namespace fs = std::filesystem;

constexpr std::string_view kOgreVersion = "14.5.2";
static_assert(OGRE_VERSION_MAJOR == 14 && OGRE_VERSION_MINOR == 5 &&
                  OGRE_VERSION_PATCH == 2,
              "run3_shell must be built with pinned Ogre 14.5.2");

struct Options {
#ifdef _WIN32
    std::string renderer{"d3d11"};
#else
    std::string renderer{"gl3plus"};
#endif
    std::uint64_t frames{};
    fs::path userDir;
    std::optional<fs::path> contentRoot;
};

[[nodiscard]] fs::path executablePath(const char* argv0) {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length != 0 && length < buffer.size()) {
        return fs::path(std::wstring_view(buffer.data(), length));
    }
#else
    std::error_code error;
    const fs::path resolved = fs::read_symlink("/proc/self/exe", error);
    if (!error) {
        return resolved;
    }
#endif

    std::error_code fallbackError;
    const fs::path fallback = fs::absolute(argv0, fallbackError);
    if (fallbackError) {
        throw std::runtime_error("Cannot resolve the run3_shell executable path");
    }
    return fallback;
}

[[nodiscard]] fs::path defaultUserDir() {
#ifdef _WIN32
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        return fs::path(localAppData) / "Run3" / "shell";
    }
#else
    if (const char* stateHome = std::getenv("XDG_STATE_HOME")) {
        return fs::path(stateHome) / "run3" / "shell";
    }
    if (const char* home = std::getenv("HOME")) {
        return fs::path(home) / ".local" / "state" / "run3" / "shell";
    }
#endif
    return fs::temp_directory_path() / "run3" / "shell";
}

[[nodiscard]] fs::path resolveCliPath(const fs::path& value,
                                      const fs::path& executableDir) {
    const fs::path resolved = value.is_absolute() ? value : executableDir / value;
    return fs::absolute(resolved).lexically_normal();
}

[[noreturn]] void argumentError(const std::string& message) {
    throw std::invalid_argument(message +
        "\nUsage: run3_shell [--renderer d3d11|gl3plus] [--frames N]"
        " [--user-dir PATH] [--content-root PATH]");
}

[[nodiscard]] Options parseOptions(int argc, char** argv,
                                   const fs::path& executableDir) {
    Options options;
    options.userDir = defaultUserDir();

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: run3_shell [--renderer d3d11|gl3plus] [--frames N]"
                   " [--user-dir PATH] [--content-root PATH]\n"
                << "Relative paths are resolved from the executable directory.\n";
            std::exit(EXIT_SUCCESS);
        }

        if (index + 1 >= argc) {
            argumentError("Missing value for " + std::string(argument));
        }
        const std::string value(argv[++index]);

        if (argument == "--renderer") {
            options.renderer = value;
        } else if (argument == "--frames") {
            std::size_t parsed{};
            try {
                options.frames = std::stoull(value, &parsed);
            } catch (const std::exception&) {
                argumentError("Invalid frame count: " + value);
            }
            if (parsed != value.size()) {
                argumentError("Invalid frame count: " + value);
            }
        } else if (argument == "--user-dir") {
            options.userDir = resolveCliPath(value, executableDir);
        } else if (argument == "--content-root") {
            options.contentRoot = resolveCliPath(value, executableDir);
        } else {
            argumentError("Unknown argument: " + std::string(argument));
        }
    }

    options.userDir = fs::absolute(options.userDir).lexically_normal();
    return options;
}

[[nodiscard]] std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

class Run3Shell final : public OgreBites::ApplicationContext,
                        public OgreBites::InputListener {
public:
    Run3Shell(Options options, fs::path executableDir)
        : OgreBites::ApplicationContext("Run3 renderer shell"),
          mOptions(std::move(options)),
          mExecutableDir(std::move(executableDir)) {}

    void createRoot() override {
        const fs::path pluginConfig = mExecutableDir / "plugins.cfg";
        if (!fs::is_regular_file(pluginConfig)) {
            throw std::runtime_error(
                "Missing installed Ogre plugin configuration: " +
                pluginConfig.string());
        }

        fs::create_directories(mOptions.userDir);
        mRoot = OGRE_NEW Ogre::Root(
            pluginConfig.string(), (mOptions.userDir / "ogre.cfg").string(),
            (mOptions.userDir / "ogre.log").string());
        mOverlaySystem = OGRE_NEW Ogre::OverlaySystem();
    }

    bool oneTimeConfig() override {
        const std::string requested = lower(mOptions.renderer);
        const std::string wanted =
            requested == "d3d11" ? "direct3d11" :
            (requested == "gl3+" || requested == "gl3plus") ? "opengl 3+" :
            requested;

        Ogre::RenderSystem* selected{};
        for (Ogre::RenderSystem* renderer : mRoot->getAvailableRenderers()) {
            if (lower(renderer->getName()).find(wanted) != std::string::npos) {
                selected = renderer;
                break;
            }
        }
        if (selected == nullptr) {
            std::string available;
            for (const Ogre::RenderSystem* renderer : mRoot->getAvailableRenderers()) {
                available += available.empty() ? "" : ", ";
                available += renderer->getName();
            }
            throw std::runtime_error("Requested renderer '" + mOptions.renderer +
                                     "' is unavailable; available: " + available);
        }

        const Ogre::ConfigOptionMap& config = selected->getConfigOptions();
        if (config.find("Full Screen") != config.end()) {
            selected->setConfigOption("Full Screen", "No");
        }
        if (config.find("VSync") != config.end()) {
            selected->setConfigOption("VSync", "No");
        }
        mRoot->setRenderSystem(selected);
        return true;
    }

    void locateResources() override {
        Ogre::ResourceGroupManager& resources =
            Ogre::ResourceGroupManager::getSingleton();
        const fs::path media =
            (mExecutableDir / ".." / "share" / "run3" / "Media")
                .lexically_normal();
        resources.addResourceLocation((media / "Main").string(), "FileSystem",
                                      Ogre::RGN_INTERNAL);
        resources.addResourceLocation((media / "RTShaderLib").string(),
                                      "FileSystem", Ogre::RGN_INTERNAL);

        if (mOptions.contentRoot) {
            if (!fs::is_directory(*mOptions.contentRoot)) {
                throw std::runtime_error("Content root is not a directory: " +
                                         mOptions.contentRoot->string());
            }
            resources.addResourceLocation(mOptions.contentRoot->string(),
                                          "FileSystem", "Run3Content");
        }
    }

    void setup() override {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        mSceneManager = mRoot->createSceneManager();
        mSceneManager->addRenderQueueListener(mOverlaySystem);
#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
        mShaderGenerator->addSceneManager(mSceneManager);
#endif
        mSceneManager->setAmbientLight(Ogre::ColourValue(0.25F, 0.25F, 0.25F));

        Ogre::Light* light = mSceneManager->createLight("Run3ShellLight");
        light->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* lightNode =
            mSceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->setDirection(
            Ogre::Vector3(-1.0F, -1.0F, -1.0F).normalisedCopy(),
            Ogre::Node::TS_WORLD);
        lightNode->attachObject(light);

        Ogre::Entity* cube =
            mSceneManager->createEntity("Run3ShellCube", Ogre::SceneManager::PT_CUBE);
        mCubeNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
        mCubeNode->attachObject(cube);

        mCamera = mSceneManager->createCamera("Run3ShellCamera");
        mCamera->setNearClipDistance(5.0F);
        Ogre::SceneNode* cameraNode =
            mSceneManager->getRootSceneNode()->createChildSceneNode();
        cameraNode->setPosition(0.0F, 75.0F, 300.0F);
        cameraNode->lookAt(Ogre::Vector3::ZERO, Ogre::Node::TS_WORLD);
        cameraNode->attachObject(mCamera);

        Ogre::Viewport* viewport = getRenderWindow()->addViewport(mCamera);
        viewport->setBackgroundColour(Ogre::ColourValue(0.04F, 0.06F, 0.1F));
        updateAspectRatio();

        Ogre::LogManager::getSingleton().logMessage(
            "Run3 shell pinned Ogre version: " + std::string(kOgreVersion));
        Ogre::LogManager::getSingleton().logMessage(
            "Run3 shell selected render system: " +
            mRoot->getRenderSystem()->getName());
        Ogre::LogManager::getSingleton().logMessage(
            "Run3 shell user directory: " + mOptions.userDir.string());
        Ogre::LogManager::getSingleton().logMessage(
            "Run3 shell content root: " +
            (mOptions.contentRoot ? mOptions.contentRoot->string() : "<none>"));
    }

    bool frameRenderingQueued(const Ogre::FrameEvent& event) override {
        if (mCubeNode != nullptr) {
            mCubeNode->yaw(Ogre::Degree(30.0F * event.timeSinceLastFrame));
        }
        return OgreBites::ApplicationContext::frameRenderingQueued(event);
    }

    bool frameEnded(const Ogre::FrameEvent&) override {
        ++mRenderedFrames;
        if (mOptions.frames != 0 && mRenderedFrames >= mOptions.frames) {
            mRoot->queueEndRendering();
        }
        return true;
    }

    bool keyPressed(const OgreBites::KeyboardEvent& event) override {
        if (event.keysym.sym == OgreBites::SDLK_ESCAPE) {
            mRoot->queueEndRendering();
            return true;
        }
        return false;
    }

    void windowResized(Ogre::RenderWindow*) override { updateAspectRatio(); }

    bool windowClosing(Ogre::RenderWindow*) override {
        mRoot->queueEndRendering();
        return true;
    }

    void windowClosed(Ogre::RenderWindow*) override {
        mRoot->queueEndRendering();
    }

private:
    void updateAspectRatio() {
        if (mCamera == nullptr || getRenderWindow() == nullptr ||
            getRenderWindow()->getHeight() == 0) {
            return;
        }
        mCamera->setAspectRatio(
            static_cast<Ogre::Real>(getRenderWindow()->getWidth()) /
            static_cast<Ogre::Real>(getRenderWindow()->getHeight()));
    }

    Options mOptions;
    fs::path mExecutableDir;
    Ogre::SceneManager* mSceneManager{};
    Ogre::SceneNode* mCubeNode{};
    Ogre::Camera* mCamera{};
    std::uint64_t mRenderedFrames{};
};

}  // namespace

int main(int argc, char** argv) {
    try {
        const fs::path executableDir =
            fs::weakly_canonical(executablePath(argv[0])).parent_path();
        Options options = parseOptions(argc, argv, executableDir);
        Run3Shell shell(std::move(options), executableDir);
        shell.initApp();
        if (shell.getRoot() == nullptr ||
            shell.getRoot()->getRenderSystem() == nullptr) {
            throw std::runtime_error("Ogre application initialization failed");
        }
        try {
            shell.getRoot()->startRendering();
        } catch (...) {
            shell.closeApp();
            throw;
        }
        shell.closeApp();
        return EXIT_SUCCESS;
    } catch (const Ogre::Exception& error) {
        std::cerr << "Ogre error: " << error.getFullDescription() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "run3_shell error: " << error.what() << '\n';
    }
    return EXIT_FAILURE;
}
