#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreFont.h>
#include <OgreFontManager.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>
#include <OgreOverlaySystem.h>
#include <OgreTextAreaOverlayElement.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

constexpr const char *group = "OgreOverlayRttDemo";
constexpr const char *texturedVertex = "OgreOverlayRttDemo/TexturedVS";
constexpr const char *texturedFragment = "OgreOverlayRttDemo/TexturedFS";
constexpr const char *solidVertex = "OgreOverlayRttDemo/SolidVS";
constexpr const char *solidFragment = "OgreOverlayRttDemo/SolidFS";
constexpr const char *litVertex = "OgreOverlayRttDemo/LitVS";
constexpr const char *litFragment = "OgreOverlayRttDemo/LitFS";

class OverlayRttDemo final : public OgreBites::ApplicationContext {
public:
  OverlayRttDemo(fs::path contentRoot, fs::path output)
      : OgreBites::ApplicationContext("Ogre Overlay RTT Demo"),
        contentRoot_(std::move(contentRoot)), output_(std::move(output)) {}

  bool oneTimeConfig() override {
    Ogre::RenderSystem *selected = nullptr;
    for (Ogre::RenderSystem *renderer : getRoot()->getAvailableRenderers()) {
      if (renderer->getName().find("Direct3D11") != std::string::npos) {
        selected = renderer;
        break;
      }
    }
    if (selected == nullptr)
      throw std::runtime_error("Direct3D11 renderer is unavailable");
    selected->setConfigOption("Full Screen", "No");
    const auto options = selected->getConfigOptions();
    if (const auto found = options.find("Video Mode"); found != options.end()) {
      for (const std::string &mode : found->second.possibleValues) {
        if (mode.find("1280 x 720") != std::string::npos) {
          selected->setConfigOption("Video Mode", mode);
          break;
        }
      }
    }
    getRoot()->setRenderSystem(selected);
    return true;
  }

  void locateResources() override {
    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    const fs::path sourceRoot = RUN3_RTT_DEMO_SOURCE_ROOT;
    resources.addResourceLocation((sourceRoot / "OgreSDK/Media/Main").string(),
                                  "FileSystem", Ogre::RGN_INTERNAL);
    resources.addResourceLocation(
        (sourceRoot / "OgreSDK/Media/RTShaderLib").string(), "FileSystem",
        Ogre::RGN_INTERNAL);
  }

  void setup() override {
    OgreBites::ApplicationContext::setup();
    // Register game assets only after ApplicationContext has parsed global
    // scripts. This proof needs four files, not the game's legacy material
    // catalogue; late registration prevents unrelated .material parsing.
    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    resources.createResourceGroup(group);
    resources.addResourceLocation((contentRoot_ / "run3/models").string(),
                                  "FileSystem", group);
    resources.addResourceLocation((contentRoot_ / "run3/mats/high").string(),
                                  "FileSystem", group);
    resources.addResourceLocation((contentRoot_ / "run3/fonts").string(),
                                  "FileSystem", group);
    scene_ = getRoot()->createSceneManager();
    scene_->addRenderQueueListener(getOverlaySystem());
    createPrograms();

    camera_ = scene_->createCamera("DemoCamera");
    camera_->setNearClipDistance(1.0F);
    camera_->setFarClipDistance(2000.0F);
    camera_->setAutoAspectRatio(true);
    auto *cameraNode = scene_->getRootSceneNode()->createChildSceneNode();
    cameraNode->setPosition(0.0F, 55.0F, 600.0F);
    cameraNode->lookAt({0.0F, 35.0F, 0.0F}, Ogre::Node::TS_WORLD);
    cameraNode->attachObject(camera_);

    Ogre::Viewport *mainViewport = getRenderWindow()->addViewport(camera_);
    mainViewport->setBackgroundColour(
        Ogre::ColourValue(0.015F, 0.025F, 0.045F));
    mainViewport->setOverlaysEnabled(false);

    scene_->setAmbientLight(Ogre::ColourValue(0.32F, 0.32F, 0.32F));
    Ogre::Light *light = scene_->createLight("DemoKeyLight");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDiffuseColour(1.0F, 0.92F, 0.78F);
    auto *lightNode = scene_->getRootSceneNode()->createChildSceneNode();
    lightNode->setDirection({-0.35F, -0.65F, -0.65F}, Ogre::Node::TS_WORLD);
    lightNode->attachObject(light);

    createRenderTexture();
    createOverlay();
    createComputer();
  }

  void shutdown() override {
    OgreBites::ApplicationContext::shutdown();
    scene_ = nullptr;
  }

  int run() {
    initApp();
    if (getRoot() == nullptr || scene_ == nullptr)
      throw std::runtime_error("Ogre demo initialization failed");
    for (unsigned frame = 0; frame < 12; ++frame) {
      pollEvents();
      rttTarget_->update();
      if (!getRoot()->renderOneFrame()) break;
    }
    if (!output_.parent_path().empty())
      fs::create_directories(output_.parent_path());
    getRenderWindow()->writeContentsToFile(output_.string());
    fs::path rttOutput = output_;
    rttOutput.replace_filename(output_.stem().string() + "-rtt" +
                               output_.extension().string());
    rttTarget_->writeContentsToFile(rttOutput.string());
    std::cout << "Wrote verified-frame candidate: " << output_.string() << '\n';
    std::cout << "Wrote native-overlay RTT: " << rttOutput.string() << '\n';
    closeApp();
    return EXIT_SUCCESS;
  }

private:
  Ogre::HighLevelGpuProgramPtr program(const std::string &name,
                                       const std::string &source,
                                       Ogre::GpuProgramType type) {
    auto result = Ogre::HighLevelGpuProgramManager::getSingleton().createProgram(
        name, group, "hlsl", type);
    result->setSource(source);
    result->setParameter("target",
                         type == Ogre::GPT_VERTEX_PROGRAM ? "vs_4_0" : "ps_4_0");
    result->setParameter("entry_point", "main");
    result->load();
    if (result->hasCompileError() || !result->isSupported())
      throw std::runtime_error("cannot compile sample shader " + name);
    return result;
  }

  void createPrograms() {
    program(texturedVertex, R"(
      float4x4 worldViewProj;
      struct Input { float4 position : POSITION; float2 uv : TEXCOORD0; };
      struct Output { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
      Output main(Input input) {
        Output output;
        output.position = mul(worldViewProj, input.position);
        output.uv = input.uv;
        return output;
      })", Ogre::GPT_VERTEX_PROGRAM);
    program(texturedFragment, R"(
      Texture2D colourTexture : register(t0);
      SamplerState colourSampler : register(s0);
      float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0)
          : SV_Target {
        return colourTexture.Sample(colourSampler, uv);
      })", Ogre::GPT_FRAGMENT_PROGRAM);
    program(solidVertex, R"(
      float4x4 worldViewProj;
      struct Input { float4 position : POSITION; };
      float4 main(Input input) : SV_POSITION {
        return mul(worldViewProj, input.position);
      })", Ogre::GPT_VERTEX_PROGRAM);
    program(solidFragment, R"(
      float4 panelColour;
      float4 main(float4 position : SV_POSITION) : SV_Target {
        return panelColour;
      })", Ogre::GPT_FRAGMENT_PROGRAM);
    program(litVertex, R"(
      float4x4 worldViewProj;
      struct Input {
        float4 position : POSITION;
        float3 normal : NORMAL;
        float2 uv : TEXCOORD0;
      };
      struct Output {
        float4 position : SV_POSITION;
        float2 uv : TEXCOORD0;
        float light : TEXCOORD1;
      };
      Output main(Input input) {
        Output output;
        output.position = mul(worldViewProj, input.position);
        output.uv = input.uv;
        output.light = 0.35 + 0.65 * saturate(dot(normalize(input.normal),
                                                   normalize(float3(0.3, 0.6, 0.7))));
        return output;
      })", Ogre::GPT_VERTEX_PROGRAM);
    program(litFragment, R"(
      Texture2D colourTexture : register(t0);
      SamplerState colourSampler : register(s0);
      float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0,
                  float light : TEXCOORD1) : SV_Target {
        float4 colour = colourTexture.Sample(colourSampler, uv);
        return float4(colour.rgb * light, colour.a);
      })", Ogre::GPT_FRAGMENT_PROGRAM);
  }

  void setPrograms(Ogre::Pass &pass, const std::string &vertex,
                   const std::string &fragment) {
    pass.setVertexProgram(vertex);
    pass.setFragmentProgram(fragment);
    pass.getVertexProgramParameters()->setNamedAutoConstant(
        "worldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
  }

  Ogre::MaterialPtr material(const std::string &name, const std::string &texture,
                             bool lighting) {
    Ogre::MaterialPtr result =
        Ogre::MaterialManager::getSingleton().create(name, group);
    Ogre::Pass *pass = result->getTechnique(0)->getPass(0);
    pass->setLightingEnabled(lighting);
    pass->setMaxSimultaneousLights(lighting ? 1 : 0);
    pass->setAmbient(0.55F, 0.55F, 0.55F);
    pass->setDiffuse(0.9F, 0.9F, 0.9F, 1.0F);
    pass->setSpecular(0.12F, 0.12F, 0.12F, 8.0F);
    if (!texture.empty()) {
      auto *unit = pass->createTextureUnitState(texture);
      unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
      unit->setTextureFiltering(Ogre::TFO_BILINEAR);
    }
    setPrograms(*pass, lighting ? litVertex : texturedVertex,
                lighting ? litFragment : texturedFragment);
    result->load();
    return result;
  }

  Ogre::MaterialPtr solidMaterial(const std::string &name,
                                  const Ogre::ColourValue &colour) {
    Ogre::MaterialPtr result =
        Ogre::MaterialManager::getSingleton().create(name, group);
    Ogre::Pass *pass = result->getTechnique(0)->getPass(0);
    pass->setLightingEnabled(false);
    pass->setDepthCheckEnabled(false);
    pass->setDepthWriteEnabled(false);
    pass->setDiffuse(colour);
    pass->setAmbient(colour);
    pass->setSelfIllumination(colour);
    setPrograms(*pass, solidVertex, solidFragment);
    pass->getFragmentProgramParameters()->setNamedConstant("panelColour", colour);
    result->load();
    return result;
  }

  void createRenderTexture() {
    rtt_ = Ogre::TextureManager::getSingleton().createManual(
        "OgreOverlayRttDemo/Texture", group, Ogre::TEX_TYPE_2D, 1024, 768, 0,
        Ogre::PF_A8B8G8R8, Ogre::TU_RENDERTARGET);
    rttTarget_ = rtt_->getBuffer()->getRenderTarget();
    rttTarget_->setAutoUpdated(false);
    Ogre::Viewport *viewport = rttTarget_->addViewport(camera_);
    viewport->setBackgroundColour(Ogre::ColourValue(0.01F, 0.025F, 0.07F));
    viewport->setClearEveryFrame(true);
    viewport->setSkiesEnabled(false);
    viewport->setShadowsEnabled(false);
    viewport->setOverlaysEnabled(true);
    viewport->setVisibilityMask(0);
  }

  void createOverlay() {
    Ogre::FontPtr font = Ogre::FontManager::getSingleton().create(
        "OgreOverlayRttDemo/Font", group);
    font->setType(Ogre::FT_TRUETYPE);
    font->setSource("console.ttf");
    font->setTrueTypeSize(28.0F);
    font->setTrueTypeResolution(96);
    font->load();
    Ogre::Pass *fontPass = font->getMaterial()->getTechnique(0)->getPass(0);
    setPrograms(*fontPass, texturedVertex, texturedFragment);
    font->getMaterial()->load();

    const auto background = solidMaterial(
        "OgreOverlayRttDemo/Background",
        Ogre::ColourValue(0.015F, 0.055F, 0.16F, 1.0F));
    const auto header = solidMaterial(
        "OgreOverlayRttDemo/Header",
        Ogre::ColourValue(0.0F, 0.55F, 0.72F, 1.0F));
    const auto status = solidMaterial(
        "OgreOverlayRttDemo/Status",
        Ogre::ColourValue(0.04F, 0.22F, 0.32F, 1.0F));

    auto &manager = Ogre::OverlayManager::getSingleton();
    auto *root = static_cast<Ogre::OverlayContainer *>(
        manager.createOverlayElement("Panel", "OgreOverlayRttDemo/Root"));
    root->setMetricsMode(Ogre::GMM_RELATIVE);
    root->setPosition(0.0F, 0.0F);
    root->setDimensions(1.0F, 1.0F);
    root->setMaterial(background);

    auto addPanel = [&](const std::string &name, float left, float top,
                        float width, float height,
                        const Ogre::MaterialPtr &panelMaterial) {
      auto *panel = static_cast<Ogre::OverlayContainer *>(
          manager.createOverlayElement("Panel", name));
      panel->setMetricsMode(Ogre::GMM_RELATIVE);
      panel->setPosition(left, top);
      panel->setDimensions(width, height);
      panel->setMaterial(panelMaterial);
      root->addChild(panel);
    };
    addPanel("OgreOverlayRttDemo/HeaderPanel", 0.04F, 0.055F, 0.92F, 0.13F,
             header);
    addPanel("OgreOverlayRttDemo/StatusPanel", 0.08F, 0.72F, 0.84F, 0.17F,
             status);

    auto addText = [&](const std::string &name, const std::string &caption,
                       float left, float top, float size,
                       const Ogre::ColourValue &colour) {
      auto *text = static_cast<Ogre::TextAreaOverlayElement *>(
          manager.createOverlayElement("TextArea", name));
      text->setMetricsMode(Ogre::GMM_RELATIVE);
      text->setPosition(left, top);
      text->setDimensions(0.9F, 0.2F);
      text->setFontName(font->getName(), group);
      text->setCharHeight(size);
      text->setColour(colour);
      text->setCaption(caption);
      root->addChild(text);
    };
    addText("OgreOverlayRttDemo/Title", "OGRE NATIVE OVERLAY -> RTT", 0.085F,
            0.085F, 0.050F, Ogre::ColourValue::White);
    addText("OgreOverlayRttDemo/Line1", "AMIGA DISPLAY ONLINE", 0.10F, 0.30F,
            0.065F, Ogre::ColourValue(0.25F, 1.0F, 0.55F, 1.0F));
    addText("OgreOverlayRttDemo/Line2", "No overlay is drawn on the main viewport.",
            0.10F, 0.44F, 0.034F,
            Ogre::ColourValue(0.72F, 0.86F, 1.0F, 1.0F));
    addText("OgreOverlayRttDemo/Line3", "Source: Ogre::OverlaySystem",
            0.10F, 0.76F, 0.035F, Ogre::ColourValue::White);

    overlay_ = manager.create("OgreOverlayRttDemo/Overlay");
    overlay_->add2D(root);
    overlay_->show();
  }

  void createComputer() {
    const auto body = material("AMIGA100001-Default", "amiga1000_u.dds", true);
    const auto keyboard =
        material("AMIGA100001-Default2", "amiga1000_u3.dds", true);
    const auto screen = material("AMIGA1000AmigaMon", rtt_->getName(), false);

    Ogre::Entity *computer = scene_->createEntity("DemoAmiga", "amiga1000.mesh", group);
    for (unsigned index = 0; index < computer->getNumSubEntities(); ++index) {
      Ogre::SubEntity *part = computer->getSubEntity(index);
      // amiga1000.mesh stores the CRT plane as submesh 1. Use the mesh's
      // structural identity here because this deliberately isolated sample
      // does not parse the game's imported material scripts.
      if (index == 1)
        part->setMaterial(screen);
      else if (index == 2)
        part->setMaterial(keyboard);
      else
        part->setMaterial(body);
    }
    auto *node = scene_->getRootSceneNode()->createChildSceneNode();
    node->yaw(Ogre::Degree(180.0F));
    node->setScale(3.0F, 3.0F, 3.0F);
    node->attachObject(computer);
  }

  fs::path contentRoot_;
  fs::path output_;
  Ogre::SceneManager *scene_{};
  Ogre::Camera *camera_{};
  Ogre::TexturePtr rtt_;
  Ogre::RenderTarget *rttTarget_{};
  Ogre::Overlay *overlay_{};
};

} // namespace

int main(int argc, char **argv) {
  try {
    const fs::path sourceRoot = RUN3_RTT_DEMO_SOURCE_ROOT;
    const fs::path content = argc > 1
                                 ? fs::path(argv[1])
                                 : sourceRoot / "Games/The Long Way/TheLongWay";
    const fs::path output = argc > 2
                                ? fs::path(argv[2])
                                : sourceRoot / "build/repro/ogre-overlay-rtt-demo.png";
    OverlayRttDemo demo(content, output);
    return demo.run();
  } catch (const Ogre::Exception &error) {
    std::cerr << "Ogre Overlay RTT demo error: " << error.getFullDescription()
              << '\n';
  } catch (const std::exception &error) {
    std::cerr << "Ogre Overlay RTT demo error: " << error.what() << '\n';
  }
  return EXIT_FAILURE;
}
