#include <run3/rendering/OgreLighting.hpp>
#include "OgreLightingPost.hpp"
#include "OgreCompositorEffects.hpp"
#include "SurfaceMaps.hpp"
#include "GBufferGeometry.hpp"
#include <Ogre.h>
#include <OgreShaderGenerator.h>
#include <OgreShaderRenderState.h>
#include <OgreShaderSubRenderState.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <mutex>

namespace run3::rendering {
namespace {
constexpr const char *group = "Run3Lighting";
void message(const std::string &text) {
  Ogre::LogManager::getSingleton().logMessage("Step 9B: " + text);
}
Ogre::ColourValue colour(const std::array<float, 3> &v) {
  return Ogre::ColourValue(v[0], v[1], v[2]);
}
void parameter(Ogre::RTShader::SubRenderState *state, const char *name,
               const std::string &value) {
  if (!state->setParameter(name, value))
    throw std::runtime_error(std::string("RTSS rejected ") + name + "=" + value);
}
class ShaderErrors final : public Ogre::LogListener {
public:
  ShaderErrors() { Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this); }
  ~ShaderErrors() override { Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this); }
  void messageLogged(const Ogre::String &text,Ogre::LogMessageLevel,bool,
                     const Ogre::String &,bool &) override {
    if(text.find("error X")!=std::string::npos || text.find("error C")!=std::string::npos ||
       text.find("compile error")!=std::string::npos || text.find("failed to compile")!=std::string::npos ||
       text.find("Error compiling")!=std::string::npos || text.find("ERROR:")!=std::string::npos) {
      std::lock_guard<std::mutex> lock(mutex_);
      failure_=text;
    }
  }
  void check() {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!failure_.empty()) throw std::runtime_error("Required lighting shader failure: "+failure_);
  }
private:
  std::mutex mutex_;
  std::string failure_;
};

// Ogre 14.5.2's multi-light shadow fallback assumes that the first shadow
// texture has a corresponding light. If no shadow-casting light intersects the
// current frustum, getShadowTexIndex(0) returns the end of the texture array;
// resolving the second RTSS shadow sampler then underflows an unsigned camera
// index and leaves AutoParamDataSource with an invalid projector pointer.
//
// Keep Ogre on its valid, ordinary shadow-texture path by supplying an
// invisible, zero-power light only for an otherwise caster-free frustum. Its
// light mask is empty, so its shadow target is just the cleared depth texture
// and it cannot alter the scene. Real authored lights retain all atlas slots.
class ShadowFallbackGuard final : public Ogre::ShadowTextureListener {
public:
  explicit ShadowFallbackGuard(Ogre::SceneManager &scene) : scene_(scene) {
    anchor_ = scene_.createLight("Run3/InternalShadowFallback");
    anchor_->setType(Ogre::Light::LT_DIRECTIONAL);
    anchor_->setDiffuseColour(Ogre::ColourValue::Black);
    anchor_->setSpecularColour(Ogre::ColourValue::Black);
    anchor_->setPowerScale(0.0F);
    anchor_->setLightMask(0);
    anchor_->setCastShadows(true);
    anchor_->setVisible(false);
    node_ = scene_.getRootSceneNode()->createChildSceneNode(
        "Run3/InternalShadowFallbackNode");
    node_->attachObject(anchor_);
    node_->setDirection(Ogre::Vector3(0.0F, -1.0F, 0.0F));
    scene_.addShadowTextureListener(this);
  }

  ~ShadowFallbackGuard() override {
    scene_.removeShadowTextureListener(this);
    node_->detachObject(anchor_);
    scene_.destroySceneNode(node_);
    scene_.destroyLight(anchor_);
  }

  bool sortLightsAffectingFrustum(Ogre::LightList &lights) override {
    const auto caster = std::find_if(lights.begin(), lights.end(),
        [](const Ogre::Light *light) { return light->getCastShadows(); });
    if (caster == lights.end())
      lights.push_back(anchor_);
    // Let Ogre retain its normal stable ordering after the fallback is added.
    return false;
  }

private:
  Ogre::SceneManager &scene_;
  Ogre::Light *anchor_{};
  Ogre::SceneNode *node_{};
};
}

class OgreLighting::Impl {
public:
  Ogre::SceneManager &scene;
  Ogre::Camera &camera;
  Ogre::Viewport &viewport;
  LightingSettings settings;
  std::vector<Ogre::Entity *> entities;
  std::vector<Ogre::Light *> lights;
  Ogre::SceneNode *lab{};
  Ogre::SceneNode *moving{};
  Ogre::SceneNode *caster{};
  std::unique_ptr<LightingPost> post;
  std::unique_ptr<OgreCompositorEffects> compositorEffects;
  std::unique_ptr<ShadowFallbackGuard> shadowFallback;
  SurfaceMapsFactory mapsFactory;
  GBufferSurfaceFactory gbufferFactory;
  GBufferGeometryFactory geometryFactory;
  ShaderErrors shaderErrors;
  std::chrono::steady_clock::time_point started{std::chrono::steady_clock::now()};
  std::chrono::steady_clock::time_point lastFrame{};
  std::vector<double> frameMilliseconds;
  std::size_t frames{};
  Ogre::ColourValue probeAmbient{-1, -1, -1};

  Impl(Ogre::SceneManager &s, Ogre::Camera &c, Ogre::Viewport &v,
       LightingSettings config) : scene(s), camera(c), viewport(v), settings(config) {}
  // Initialise only after unique ownership is established, so shader/compiler
  // failures execute teardown rather than leaving registered factory pointers.
  void initialise() {
    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    resources.createResourceGroup(group);
    auto &generator = Ogre::RTShader::ShaderGenerator::getSingleton();
    generator.addSubRenderStateFactory(&mapsFactory);
    generator.addSubRenderStateFactory(&gbufferFactory);
    generator.addSubRenderStateFactory(&geometryFactory);
    auto *globalState = generator.getRenderState(Ogre::MSN_SHADERGEN);
    viewport.setMaterialScheme(Ogre::MSN_SHADERGEN);
    if (settings.pipeline == LightingPipeline::Pbr) {
      // Neutral, deterministic fallback probe; authored HDR probes can replace it.
      auto probe = Ogre::TextureManager::getSingleton().createManual(
          "Run3/NeutralProbe", group, Ogre::TEX_TYPE_CUBE_MAP, 16, 16, 4,
          Ogre::PF_FLOAT32_RGBA, Ogre::TU_DEFAULT);
      for (unsigned face = 0; face < 6; ++face) {
        for (unsigned mip = 0; mip <= probe->getNumMipmaps(); ++mip) {
          auto buffer = probe->getBuffer(face, mip);
          buffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
          auto box = buffer->getCurrentLock();
          for (std::size_t y = 0; y < box.getHeight(); ++y)
            for (std::size_t x = 0; x < box.getWidth(); ++x)
              box.setColourAt(Ogre::ColourValue(0.18F, 0.20F, 0.24F), x, y, 0);
          buffer->unlock();
        }
      }
    }
    const auto budget = shadowBudget(settings.shadows, settings.pipeline);
    if (budget.textures) {
      // Ogre's default caster enables lighting, which can make global RTSS
      // per-pixel lighting demand NORMAL0 even for an unlit/normal-less mesh.
      // Depth rendering only needs position; Ogre derives alpha rejection
      // from the original pass for alpha-tested casters.
      auto casterMaterial = Ogre::MaterialManager::getSingleton().create("Run3/DepthCaster", group);
      auto *casterPass = casterMaterial->getTechnique(0)->getPass(0);
      casterPass->setLightingEnabled(false);
      casterPass->setDepthWriteEnabled(true);
      casterPass->setDepthCheckEnabled(true);
      casterMaterial->setReceiveShadows(false);
      // Let RTSS generate the position-only caster for every programmable
      // pipeline. Ogre/ShadowBlend is a colour-shadow receiver/caster pair;
      // binding it to our PF_DEPTH16 atlas crashes D3D11 when local lights
      // begin rendering their shadow cameras.
      scene.setShadowTextureCasterMaterial(casterMaterial);
      scene.setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
      scene.setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 1);
      scene.setShadowTextureCountPerLightType(Ogre::Light::LT_SPOTLIGHT, 1);
      // Ogre Classic's one-map point shadow is a camera-facing 120-degree
      // approximation, not an omnidirectional shadow. Never allocate it: it
      // creates a view-dependent clipping plane through point-light volume.
      scene.setShadowTextureCountPerLightType(Ogre::Light::LT_POINT, 0);
      scene.setShadowTextureSettings(static_cast<Ogre::uint16>(budget.resolution),
                                    static_cast<Ogre::uint16>(budget.textures), Ogre::PF_DEPTH16);
      scene.setShadowTextureSelfShadow(true);
      scene.setShadowFarDistance(budget.distance);
      shadowFallback = std::make_unique<ShadowFallbackGuard>(scene);
      auto *shadow = generator.createSubRenderState("SGX_IntegratedPSSM3");
      parameter(shadow, "light_count", std::to_string(budget.textures));
      parameter(shadow, "filter", budget.filterSamples == 16 ? "pcf16" : "pcf4");
      globalState->setLightCountAutoUpdate(false);
      globalState->setLightCount(budget.textures);
      globalState->addTemplateSubRenderState(shadow);
      message("local shadow atlas=" + std::to_string(budget.textures) + "x" +
              std::to_string(budget.resolution) +
              "; spotlights use authored cones; point lights remain "
              "omnidirectional and do not use Ogre's camera-facing shadow approximation");
    }
    generator.invalidateScheme(Ogre::MSN_SHADERGEN);
    if(settings.pipeline==LightingPipeline::Pbr || settings.pipeline==LightingPipeline::Deferred)
      post=std::make_unique<LightingPost>(scene,camera,viewport,settings);
    // The authored Ogre compositor chain is independent of the lighting
    // pipeline and follows LightingPost so effects process its final image.
    compositorEffects = std::make_unique<OgreCompositorEffects>(viewport);
    message("requested/effective pipeline=" + std::string(pipelineName(settings.pipeline)));
  }
  ~Impl() {
    compositorEffects.reset();
    post.reset();
    for (auto *entity : entities) scene.destroyEntity(entity);
    for (auto *light : lights) scene.destroyLight(light);
    if (lab) {
      lab->removeAndDestroyAllChildren();
      scene.destroySceneNode(lab);
    }
    scene.setShadowTechnique(Ogre::SHADOWTYPE_NONE);
    shadowFallback.reset();
    scene.setShadowTextureCasterMaterial(Ogre::MaterialPtr{});
    auto &generator = Ogre::RTShader::ShaderGenerator::getSingleton();
    generator.getRenderState(Ogre::MSN_SHADERGEN)->resetToBuiltinSubRenderStates();
    generator.removeAllShaderBasedTechniques();
    generator.removeSubRenderStateFactory(&mapsFactory);
    generator.removeSubRenderStateFactory(&gbufferFactory);
    generator.removeSubRenderStateFactory(&geometryFactory);
    if (Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(group))
      Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(group);
  }
};

OgreLighting::OgreLighting(Ogre::SceneManager &s, Ogre::Camera &c,
                         Ogre::Viewport &v, LightingSettings settings)
    : impl_(std::make_unique<Impl>(s, c, v, settings)) { impl_->initialise(); }
OgreLighting::~OgreLighting() = default;

void OgreLighting::configureLegacyCompositors(
    const std::filesystem::path &contentRoot,
    const std::filesystem::path &supportAssets,
    const std::filesystem::path &programCache,
    const std::string_view textureQuality) {
  impl_->compositorEffects->configure(contentRoot, supportAssets, programCache,
                                      textureQuality);
}

void OgreLighting::configureMaterial(Ogre::Material &material,
                                    const MaterialDescription &description,
                                    LightingSettings settings, bool tangents) {
  auto &generator = Ogre::RTShader::ShaderGenerator::getSingleton();
  auto *pass = material.getTechnique(0)->getPass(0);
  const bool lit = description.lighting && description.surface != Surface::Unlit &&
                   description.reflectionMapping != ReflectionMapping::CubeDirection;
  pass->setLightingEnabled(lit);
  pass->setAmbient(1, 1, 1);
  pass->setDiffuse(description.diffuse[0], description.diffuse[1],
                   description.diffuse[2], description.diffuse[3]);
  pass->setSpecular(colour(description.specular));
  pass->setSelfIllumination(colour(description.emissive));
  pass->setShininess(description.shininess);
  pass->setMaxSimultaneousLights(static_cast<unsigned short>(
      std::min(description.lightLimit, 6U)));
  pass->setDepthCheckEnabled(true);
  pass->setDepthWriteEnabled(description.surface != Surface::Transparent);
  if (description.surface == Surface::Transparent)
    pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  if (description.surface == Surface::Cutout)
    pass->setAlphaRejectSettings(Ogre::CMPF_GREATER_EQUAL,
                                static_cast<unsigned char>(description.alphaCutoff * 255));
  pass->setCullingMode(description.doubleSided ? Ogre::CULL_NONE : Ogre::CULL_CLOCKWISE);
  material.setReceiveShadows(description.receiveShadows && lit);
  material.setTransparencyCastsShadows(false);
  if (!description.diffuseMap.name.empty()) {
    auto *texture = pass->createTextureUnitState();
    if (!description.diffuseAnimationFrames.empty()) {
      std::vector<Ogre::String> frames(description.diffuseAnimationFrames.begin(),
                                       description.diffuseAnimationFrames.end());
      texture->setAnimatedTextureName(frames,
                                      description.diffuseAnimationDuration);
    } else if (!description.diffuseAnimationBase.empty() &&
               description.diffuseAnimationFrameCount > 0) {
      texture->setAnimatedTextureName(description.diffuseAnimationBase,
                                      description.diffuseAnimationFrameCount,
                                      description.diffuseAnimationDuration);
    } else {
      texture->setTextureName(description.diffuseMap.name);
    }
    if (description.diffuseScrollU != 0.0F || description.diffuseScrollV != 0.0F)
      texture->setScrollAnimation(description.diffuseScrollU,
                                  description.diffuseScrollV);
    if (description.diffuseRotate != 0.0F)
      texture->setRotateAnimation(description.diffuseRotate);
    const auto transform = [](const TextureTransform value) {
      switch (value) {
      case TextureTransform::TranslateU: return Ogre::TextureUnitState::TT_TRANSLATE_U;
      case TextureTransform::TranslateV: return Ogre::TextureUnitState::TT_TRANSLATE_V;
      case TextureTransform::ScaleU: return Ogre::TextureUnitState::TT_SCALE_U;
      case TextureTransform::ScaleV: return Ogre::TextureUnitState::TT_SCALE_V;
      case TextureTransform::Rotate: return Ogre::TextureUnitState::TT_ROTATE;
      }
      return Ogre::TextureUnitState::TT_TRANSLATE_U;
    };
    const auto waveform = [](const TextureWaveform value) {
      switch (value) {
      case TextureWaveform::Sine: return Ogre::WFT_SINE;
      case TextureWaveform::Triangle: return Ogre::WFT_TRIANGLE;
      case TextureWaveform::Square: return Ogre::WFT_SQUARE;
      case TextureWaveform::Sawtooth: return Ogre::WFT_SAWTOOTH;
      case TextureWaveform::InverseSawtooth: return Ogre::WFT_INVERSE_SAWTOOTH;
      }
      return Ogre::WFT_SINE;
    };
    for (const auto &animation : description.diffuseWaveAnimations)
      texture->setTransformAnimation(
          transform(animation.transform), waveform(animation.waveform),
          animation.base, animation.frequency, animation.phase,
          animation.amplitude);
    texture->setHardwareGammaEnabled(settings.pipeline == LightingPipeline::Pbr || settings.pipeline == LightingPipeline::Deferred);
    texture->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
    texture->setTextureAnisotropy(settings.pipeline == LightingPipeline::FastForward ? 2 : 8);
  }
  if (!description.reflectionMap.name.empty()) {
    auto *reflection = pass->createTextureUnitState();
    if (description.reflectionMapping == ReflectionMapping::Cube ||
        description.reflectionMapping == ReflectionMapping::CubeDirection) {
      reflection->setTextureName(description.reflectionMap.name, Ogre::TEX_TYPE_CUBE_MAP);
      // SkyBox supplies float3 cube directions but no normals. It is not a
      // reflective surface and must not receive ENV_REFLECTION or PBR UV0.
      if (description.reflectionMapping == ReflectionMapping::Cube)
        reflection->setEnvironmentMap(true, Ogre::TextureUnitState::ENV_REFLECTION);
    } else {
      reflection->setTextureName(description.reflectionMap.name);
      reflection->setEnvironmentMap(true, Ogre::TextureUnitState::ENV_CURVED);
    }
    reflection->setColourOperationEx(Ogre::LBX_BLEND_MANUAL, Ogre::LBS_TEXTURE,
        Ogre::LBS_CURRENT, Ogre::ColourValue::White, Ogre::ColourValue::White,
        description.reflectionWeight);
    // Preserve albedo alpha for layered reflection; reflection-only authored
    // glass obtains its opacity from the environment texture's alpha.
    if (!description.diffuseMap.name.empty())
      reflection->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_CURRENT);
    reflection->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
    reflection->setHardwareGammaEnabled(settings.pipeline == LightingPipeline::Pbr || settings.pipeline == LightingPipeline::Deferred);
  }
  if (!generator.createShaderBasedTechnique(material.getTechnique(0), Ogre::MSN_SHADERGEN))
    throw std::runtime_error("Cannot generate lighting technique for " + material.getName());
  auto *state = generator.getRenderState(Ogre::MSN_SHADERGEN, material, 0);
  if (!state) throw std::runtime_error("Missing RTSS material state: " + material.getName());
  if (lit && settings.pipeline == LightingPipeline::Pbr) {
    // Ambient is supplied once through the scene-coloured neutral IBL probe.
    pass->setAmbient(0, 0, 0);
    pass->setSpecular(description.roughness, description.metallic, 0, 1);
    auto *pbr = generator.createSubRenderState("CookTorranceLighting");
    if (!description.metalRoughnessMap.name.empty())
      parameter(pbr, "texture", description.metalRoughnessMap.name);
    state->addTemplateSubRenderState(pbr);
    auto *ibl = generator.createSubRenderState("ImageBasedLighting");
    parameter(ibl, "texture", "Run3/NeutralProbe");
    parameter(ibl, "luminance", "1");
    state->addTemplateSubRenderState(ibl);
  } else if (lit) {
    // Lighting implementations share the FFP_LIGHTING execution slot. Keep
    // the standard per-pixel implementation local to non-PBR materials so it
    // cannot collide with CookTorrance's output semantics.
    state->addTemplateSubRenderState(
        generator.createSubRenderState("SGX_PerPixelLighting"));
  }
  if (lit && !description.normalMap.name.empty()) {
    if (tangents) {
      const auto index = pass->getNumTextureUnitStates();
      auto *normalTexture = pass->createTextureUnitState(description.normalMap.name);
      normalTexture->setHardwareGammaEnabled(false);
      generator._markNonFFP(normalTexture);
      auto *normal = generator.createSubRenderState(Ogre::RTShader::SRS_NORMALMAP);
      parameter(normal, "texture_index", std::to_string(index));
      parameter(normal, "normalmap_space", "tangent_space");
      state->addTemplateSubRenderState(normal);
    } else {
      message(description.name + ": normal-map fallback (mesh has no tangent basis)");
    }
  }
  if (!description.specularMap.name.empty() || !description.aoMap.name.empty()) {
    auto *maps=static_cast<SurfaceMaps *>(generator.createSubRenderState("Run3SurfaceMaps"));
    maps->pbr=lit && settings.pipeline==LightingPipeline::Pbr;
    maps->unlitTint=false;
    auto dataMap=[&](const std::string &name) {
      const int index=static_cast<int>(pass->getNumTextureUnitStates());
      auto *texture=pass->createTextureUnitState(name);
      texture->setHardwareGammaEnabled(false);generator._markNonFFP(texture);
      return index;
    };
    if(!description.specularMap.name.empty())
      maps->specularIndex=dataMap(description.specularMap.name);
    if(!description.aoMap.name.empty()) maps->aoIndex=dataMap(description.aoMap.name);
    state->addTemplateSubRenderState(maps);
    if(!description.specularMap.name.empty() && maps->pbr)
      message(description.name+": legacy specular mask modulates PBR F0; not interpreted as metallic");
  }
  if(settings.pipeline == LightingPipeline::Deferred) {
    LightingPost::configureDeferred(material,description.surface,lit,
                                   !description.reflectionMap.name.empty());
    if(auto *deferred=generator.getRenderState("Run3/GBuffer",material,0)) {
      if(auto *normal=state->getSubRenderState(Ogre::RTShader::SRS_NORMALMAP)) {
        auto *copy=generator.createSubRenderState(Ogre::RTShader::SRS_NORMALMAP);
        copy->copyFrom(*normal);deferred->addTemplateSubRenderState(copy);
      }
      if(auto *maps=static_cast<SurfaceMaps *>(state->getSubRenderState("Run3SurfaceMaps"))) {
        auto *surface=static_cast<GBufferSurface *>(generator.createSubRenderState("Run3GBufferSurface"));
        surface->specularIndex=maps->specularIndex;surface->aoIndex=maps->aoIndex;
        deferred->addTemplateSubRenderState(surface);
      }
      generator.invalidateMaterial("Run3/GBuffer",material.getName(),material.getGroup());
    }
  }
}

void OgreLighting::createLab() {
  auto &s = *impl_;
  s.lab = s.scene.getRootSceneNode()->createChildSceneNode("Run3/LightingLab");
  s.scene.setAmbientLight(Ogre::ColourValue(0.08F, 0.08F, 0.08F));
  s.camera.setFarClipDistance(30000);
  s.camera.getParentSceneNode()->setPosition(650, 360, 950);
  s.camera.getParentSceneNode()->lookAt({0, 100, -200}, Ogre::Node::TS_WORLD);
  auto object = [&](const std::string &name, Ogre::Vector3 pos, Ogre::Vector3 scale,
                    MaterialDescription description) {
    auto *entity = s.scene.createEntity(name, Ogre::SceneManager::PT_CUBE);
    s.entities.push_back(entity);
    auto *node = s.lab->createChildSceneNode();
    node->setPosition(pos); node->setScale(scale); node->attachObject(entity);
    auto material = Ogre::MaterialManager::getSingleton().create(name, group);
    bool tangents=false;
    if(!description.normalMap.name.empty()) {
      unsigned short uv{},target{};
      if(!entity->getMesh()->suggestTangentVectorBuildParams(Ogre::VES_TANGENT,uv,target))
        entity->getMesh()->buildTangentVectors(Ogre::VES_TANGENT,uv,target);
      tangents=true;
    }
    configureMaterial(*material, description, s.settings,tangents);
    entity->setMaterial(material);
    entity->setCastShadows(description.castShadows && description.surface != Surface::Transparent);
    return node;
  };
  for(const auto *name:{"Checker","Normal","Specular","MR","AO","Cutout"}) {
    auto texture=Ogre::TextureManager::getSingleton().createManual(
        std::string("Run3/Lab/")+name,group,Ogre::TEX_TYPE_2D,16,16,0,Ogre::PF_BYTE_RGBA,Ogre::TU_DEFAULT);
    std::array<unsigned char,16*16*4> pixels{};
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) {
      const unsigned i=(y*16+x)*4;
      const unsigned char value=((x/4+y/4)%2)==0?200:65;
      pixels[i]=value;pixels[i+1]=value;pixels[i+2]=value;pixels[i+3]=255;
      if(std::string(name)=="Normal") {pixels[i]=static_cast<unsigned char>(96+x*4);pixels[i+1]=128;pixels[i+2]=250;}
      if(std::string(name)=="MR") {pixels[i]=255;pixels[i+1]=static_cast<unsigned char>(32+x*13);pixels[i+2]=230;}
      if(std::string(name)=="Cutout") pixels[i+3]=value==65?0:255;
    }
    texture->getBuffer()->blitFromMemory(Ogre::PixelBox(16,16,1,Ogre::PF_BYTE_RGBA,pixels.data()));
  }
  MaterialDescription d;
  d.name = "floor"; d.diffuse = {.4F, .4F, .4F, 1};
  object("Run3/LabFloor", {0,-30,-2500}, {35,.2F,70}, d);
  for (int i = 0; i < 6; ++i) {
    d = {}; d.name = "reference";
    d.diffuse = {0.15F + 0.12F * static_cast<float>(i), .25F, .1F, 1};
    d.roughness = .08F + static_cast<float>(i)*.16F;
    d.shininess = 128.0F / static_cast<float>(i+1);
    d.metallic = i % 2 == 0 ? 0.0F : 1.0F;
    if(i>=1) d.diffuseMap.name="Run3/Lab/Checker";
    if(i>=2) d.normalMap.name="Run3/Lab/Normal";
    if(i==3) d.specularMap.name="Run3/Lab/Specular";
    if(i==4) {d.metalRoughnessMap.name="Run3/Lab/MR";d.aoMap.name="Run3/Lab/AO";}
    if(i==5) {d.surface=Surface::Cutout;d.diffuseMap.name="Run3/Lab/Cutout";d.doubleSided=true;}
    auto *node = object("Run3/LabCube"+std::to_string(i),
                       {-500.0F+static_cast<float>(i)*200, 70, 0}, {1,1.4F,1}, d);
    if (i == 2) s.caster = node;
  }
  d = {}; d.name="glass"; d.surface=Surface::Transparent; d.diffuse={.2F,.7F,1,.3F};
  object("Run3/LabGlass", {400,100,300}, {2,2,.1F}, d);
  d = {}; d.name="emissive"; d.emissive={2,.15F,0};
  object("Run3/LabEmission", {-700,70,-300}, {1,1,1}, d);
  for (int i=1; i<=4; ++i) {
    d={}; d.diffuse={.5F,.55F,.6F,1};
    object("Run3/LabDistance"+std::to_string(i), {0,250,-static_cast<float>(i)*3000}, {6,5,4}, d);
  }
  for (int i=0; i<2; ++i) {
    auto *light=s.scene.createLight("Run3/LabLight"+std::to_string(i));
    s.lights.push_back(light);
    light->setType(i==0 ? Ogre::Light::LT_POINT : Ogre::Light::LT_SPOTLIGHT);
    light->setDiffuseColour(i==0 ? Ogre::ColourValue(.15F,.3F,1) : Ogre::ColourValue(1,.25F,.08F));
    light->setSpecularColour(light->getDiffuseColour());
    light->setAttenuation(2500,1,.0005F,.000001F);
    if (i==1) light->setSpotlightRange(Ogre::Degree(25),Ogre::Degree(65));
    light->setCastShadows(i == 1);
    auto *node=s.lab->createChildSceneNode(); node->attachObject(light);
    node->setPosition(i==0 ? -350.0F : 400.0F, 350, 200);
    node->setDirection({0,-1,-.5F});
    if(i==0) s.moving=node;
  }
  message("LightingLab: deterministic 60-Hz presentation time; stable point lighting and spot shadows enabled");
}
void OgreLighting::update(double seconds) {
  impl_->compositorEffects->update(seconds);
  impl_->shaderErrors.check();
  if (impl_->settings.pipeline == LightingPipeline::Pbr &&
      impl_->scene.getAmbientLight() != impl_->probeAmbient) {
    impl_->probeAmbient = impl_->scene.getAmbientLight();
    const auto linear = [](float value) {
      value = std::max(0.0F, value);
      return value <= .04045F ? value / 12.92F : std::pow((value + .055F) / 1.055F, 2.4F);
    };
    const auto ambient = impl_->probeAmbient;
    const Ogre::ColourValue radiance(linear(ambient.r), linear(ambient.g), linear(ambient.b));
    auto probe = Ogre::TextureManager::getSingleton().getByName("Run3/NeutralProbe", group);
    for (unsigned face = 0; face < 6; ++face)
      for (unsigned mip = 0; mip <= probe->getNumMipmaps(); ++mip) {
        auto buffer = probe->getBuffer(face, mip);
        buffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        auto box = buffer->getCurrentLock();
        for (std::size_t y = 0; y < box.getHeight(); ++y)
          for (std::size_t x = 0; x < box.getWidth(); ++x)
            box.setColourAt(radiance, x, y, 0);
        buffer->unlock();
      }
  }
  const auto now = std::chrono::steady_clock::now();
  if (impl_->frames >= 3)
    impl_->frameMilliseconds.push_back(
        std::chrono::duration<double, std::milli>(now - impl_->lastFrame).count());
  impl_->lastFrame = now;
  ++impl_->frames;
  if(impl_->moving) impl_->moving->setPosition(static_cast<float>(500*std::sin(seconds)),350,200);
  if(impl_->caster) impl_->caster->setOrientation(Ogre::Quaternion(Ogre::Radian(static_cast<float>(seconds)),Ogre::Vector3::UNIT_Y));
}
void OgreLighting::setCompositorEnabled(const std::string_view name,
                                        const bool enabled) {
  impl_->compositorEffects->setEnabled(name, enabled);
}
void OgreLighting::setCompositorShaderParameter(
    const std::string_view material, const std::string_view parameter,
    const std::string_view value) {
  impl_->compositorEffects->setShaderParameter(material, parameter, value);
}
void OgreLighting::clearCompositorEffects() noexcept {
  impl_->compositorEffects->clear();
}
void OgreLighting::writeReport(const std::filesystem::path &path, Ogre::RenderWindow &window) {
  impl_->shaderErrors.check();
  const auto &s=*impl_;
  const auto &stats=window.getStatistics();
  const auto budget=shadowBudget(s.settings.shadows,s.settings.pipeline);
  nlohmann::json report{{"schema",1},{"pipeline",pipelineName(s.settings.pipeline)},
    {"renderer",Ogre::Root::getSingleton().getRenderSystem()->getName()},
    {"width",window.getWidth()},{"height",window.getHeight()},
    {"frames",s.frames},{"average_fps",stats.avgFPS},{"last_frame_triangles",stats.triangleCount},
    {"last_frame_draw_calls",stats.batchCount},{"gpu_ms",nullptr},
    {"draw_call_note","Window-target only; HDR/G-buffer/shadow targets are not included"},
    {"lights",s.scene.getMovableObjects("Light").size()},
    {"rtss_vertex_programs",Ogre::RTShader::ShaderGenerator::getSingleton().getShaderCount(Ogre::GPT_VERTEX_PROGRAM)},
    {"rtss_fragment_programs",Ogre::RTShader::ShaderGenerator::getSingleton().getShaderCount(Ogre::GPT_FRAGMENT_PROGRAM)},
    {"gpu_timing_note","No GPU timer query; FPS is not GPU timing"},
    {"shadow_maps",budget.textures},{"shadow_resolution",budget.resolution},
    {"cpu_wall_seconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-s.started).count()}};
  if(s.post) report["offscreen"]=s.post->report();
  auto samples = s.frameMilliseconds;
  std::sort(samples.begin(), samples.end());
  report["frame_wall_samples_after_three_warmup_frames"] = samples.size();
  report["frame_wall_note"] = "Between frame updates, including render, gameplay and pacing; not CPU/GPU isolation";
  if (!samples.empty()) {
    report["frame_wall_median_ms"] = samples[samples.size() / 2];
    report["frame_wall_p95_ms"] = samples[static_cast<std::size_t>((samples.size() - 1) * .95)];
  }
  if (s.lab) {
    std::vector<unsigned char> pixels(window.getWidth() * window.getHeight() * 4);
    window.copyContentsToMemory(Ogre::Box(0, 0, window.getWidth(), window.getHeight()),
        Ogre::PixelBox(window.getWidth(), window.getHeight(), 1, Ogre::PF_BYTE_RGBA, pixels.data()));
    double maximumDeviation = 0;
    for (std::size_t channel = 0; channel < 3; ++channel) {
      double sum = 0, squares = 0;
      for (std::size_t i = channel; i < pixels.size(); i += 4) {
        sum += pixels[i]; squares += double(pixels[i]) * pixels[i];
      }
      const double count = static_cast<double>(pixels.size() / 4);
      maximumDeviation = std::max(maximumDeviation,
          std::sqrt(std::max(0.0, squares / count - (sum / count) * (sum / count))));
    }
    report["lab_image_max_channel_stddev"] = maximumDeviation;
    if (maximumDeviation < 2.0)
      throw std::runtime_error("LightingLab produced a near-uniform image; refusing a false smoke-test pass");
  }
  std::ofstream out(path);
  if(!out) throw std::runtime_error("Cannot write lighting report: "+path.string());
  out << report.dump(2) << '\n';
}
} // namespace run3::rendering
