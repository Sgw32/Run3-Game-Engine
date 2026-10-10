#include "OgreLightingPost.hpp"
#include <Ogre.h>
#include <OgreCompositorManager.h>
#include <OgreCompositorInstance.h>
#include <OgreCompositionTechnique.h>
#include <OgreCompositionTargetPass.h>
#include <OgreCompositionPass.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreShaderGenerator.h>
#include <OgreShaderRenderState.h>
#include <OgreShaderSubRenderState.h>
#include <array>
#include <nlohmann/json.hpp>

namespace run3::rendering {
namespace {
constexpr auto group = "Run3Lighting";
constexpr auto gbuffer = "Run3/GBuffer";
constexpr auto transparent = "Run3/ForwardOnly";
Ogre::HighLevelGpuProgramPtr program(const std::string &name, const std::string &file,
                                    Ogre::GpuProgramType type) {
  const bool hlsl=Ogre::Root::getSingleton().getRenderSystem()->getName().find("Direct3D")!=std::string::npos;
  auto shader=Ogre::HighLevelGpuProgramManager::getSingleton().createProgram(name,group,hlsl?"hlsl":"glsl",type);
  shader->setSourceFile(file);
  if(hlsl) {
    shader->setParameter("target",type==Ogre::GPT_VERTEX_PROGRAM?"vs_4_0":"ps_4_0");
    shader->setParameter("entry_point","main");
  }
  shader->load();
  if(shader->hasCompileError() || !shader->isSupported())
    throw std::runtime_error("Required lighting shader failed: "+name);
  return shader;
}
Ogre::MaterialPtr fullscreen(const std::string &name, const std::string &fragment, unsigned textures) {
  auto material=Ogre::MaterialManager::getSingleton().create(name,group);
  auto *pass=material->getTechnique(0)->getPass(0);
  pass->setLightingEnabled(false); pass->setDepthCheckEnabled(false); pass->setDepthWriteEnabled(false);
  pass->setCullingMode(Ogre::CULL_NONE);
  auto vs=program(name+"/vs","fullscreen.vert",Ogre::GPT_VERTEX_PROGRAM);
  auto ps=program(name+"/ps",fragment,Ogre::GPT_FRAGMENT_PROGRAM);
  pass->setVertexProgram(vs->getName()); pass->setFragmentProgram(ps->getName());
  pass->getVertexProgramParameters()->setNamedAutoConstant("worldViewProj",Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
  for(unsigned i=0;i<textures;++i) {
    auto *tu=pass->createTextureUnitState();
    tu->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
    tu->setTextureFiltering(Ogre::TFO_NONE);
  }
  if(ps->getLanguage()=="glsl") {
    const auto parameters=pass->getFragmentProgramParameters();
    if(textures==1) parameters->setNamedConstant("sceneColour",0);
    else {
      parameters->setNamedConstant("diffuseBuffer",0);
      // GLSL 4.20+ explicit layout(binding=1) is already bound by the driver;
      // Ogre deliberately omits such nonzero-binding uniforms from its table.
      if(parameters->_findNamedConstantDefinition("normalDepthBuffer",false))
        parameters->setNamedConstant("normalDepthBuffer",1);
      if(parameters->_findNamedConstantDefinition("surfaceBuffer",false))
        parameters->setNamedConstant("surfaceBuffer",2);
      if(parameters->_findNamedConstantDefinition("shadowBuffer",false))
        parameters->setNamedConstant("shadowBuffer",3);
      if(parameters->_findNamedConstantDefinition("localShadowBuffer",false))
        parameters->setNamedConstant("localShadowBuffer",4);
    }
  }
  return material;
}
}
class LightingPost::Impl : public Ogre::CompositorInstance::Listener,
                           public Ogre::MaterialManager::Listener {
public:
  Ogre::SceneManager &scene;
  Ogre::Camera &camera;
  Ogre::Viewport &viewport;
  LightingSettings settings;
  Ogre::CompositorInstance *instance{};
  bool deferred{};
  bool listenerRegistered{};
  Impl(Ogre::SceneManager &s,Ogre::Camera &c,Ogre::Viewport &v,LightingSettings config)
      :scene(s),camera(c),viewport(v),settings(config),deferred(config.pipeline==LightingPipeline::Deferred) {}
  void initialise() {
    auto &generator=Ogre::RTShader::ShaderGenerator::getSingleton();
    generator.setTargetLinearColours(true);
    if(deferred) {
      generator.createScheme(gbuffer);
      generator.createScheme(transparent);
      generator.getRenderState(transparent)->addTemplateSubRenderState(
          generator.createSubRenderState("SGX_PerPixelLighting"));
      auto *state=generator.getRenderState(gbuffer);
      state->setLightCountAutoUpdate(false);state->setLightCount(6);
      state->addTemplateSubRenderState(generator.createSubRenderState("Run3GBufferGeometry"));
      state->addTemplateSubRenderState(generator.createSubRenderState("Run3GBufferSurface"));
      if(auto *shadow=generator.getRenderState(Ogre::MSN_SHADERGEN)->getSubRenderState(Ogre::RTShader::SRS_SHADOW_MAPPING)) {
        auto *copy=generator.createSubRenderState(Ogre::RTShader::SRS_SHADOW_MAPPING);
        copy->copyFrom(*shadow);state->addTemplateSubRenderState(copy);
      }
      auto *resolvePass=fullscreen("Run3/DeferredLighting","deferred.frag",5)->getTechnique(0)->getPass(0);
      resolvePass->setDepthWriteEnabled(true);
      resolvePass->setDepthCheckEnabled(true);
      resolvePass->setDepthFunction(Ogre::CMPF_ALWAYS_PASS);
    }
    fullscreen("Run3/Tonemap","tonemap.frag",1);
    auto compositor=Ogre::CompositorManager::getSingleton().create("Run3/LightingPost",group);
    auto *technique=compositor->createTechnique();
    auto *hdr=technique->createTextureDefinition("hdr");
    hdr->formatList.push_back(Ogre::PF_FLOAT16_RGBA);
    Ogre::CompositionTargetPass *target{};
    if(deferred) {
      auto *gb=technique->createTextureDefinition("gbuffer");
      gb->formatList={Ogre::PF_FLOAT16_RGBA,Ogre::PF_FLOAT16_RGBA,
                      Ogre::PF_FLOAT16_RGBA,Ogre::PF_FLOAT16_RGBA,
                      Ogre::PF_FLOAT16_RGBA};
      auto *geometry=technique->createTargetPass();geometry->setOutputName("gbuffer");
      geometry->setMaterialScheme(gbuffer);geometry->setShadowsEnabled(settings.shadows!=ShadowQuality::Off);
      geometry->createPass(Ogre::CompositionPass::PT_CLEAR)->setClearColour(Ogre::ColourValue(0,0,0,0));
      geometry->createPass(Ogre::CompositionPass::PT_RENDERSCENE)->setLastRenderQueue(Ogre::RENDER_QUEUE_SKIES_LATE);
      target=technique->createTargetPass();target->setOutputName("hdr");
      target->createPass(Ogre::CompositionPass::PT_CLEAR);
      auto *resolve=target->createPass(Ogre::CompositionPass::PT_RENDERQUAD);
      resolve->setMaterialName("Run3/DeferredLighting");resolve->setIdentifier(2);
      resolve->setInput(0,"gbuffer",0);resolve->setInput(1,"gbuffer",1);
      resolve->setInput(2,"gbuffer",2);
      resolve->setInput(3,"gbuffer",3);
      resolve->setInput(4,"gbuffer",4);
      target->setMaterialScheme(transparent);
      target->createPass(Ogre::CompositionPass::PT_RENDERSCENE)->setLastRenderQueue(Ogre::RENDER_QUEUE_SKIES_LATE);
    } else {
      target=technique->createTargetPass();target->setOutputName("hdr");
      target->setMaterialScheme(Ogre::MSN_SHADERGEN);
      target->createPass(Ogre::CompositionPass::PT_CLEAR)->setClearColour(viewport.getBackgroundColour());
      target->createPass(Ogre::CompositionPass::PT_RENDERSCENE)->setLastRenderQueue(Ogre::RENDER_QUEUE_SKIES_LATE);
    }
    auto *output=technique->getOutputTargetPass();
    auto *tone=output->createPass(Ogre::CompositionPass::PT_RENDERQUAD);
    tone->setMaterialName("Run3/Tonemap");tone->setInput(0,"hdr");tone->setIdentifier(1);
    auto *ui=output->createPass(Ogre::CompositionPass::PT_RENDERSCENE);
    ui->setFirstRenderQueue(Ogre::RENDER_QUEUE_OVERLAY);
    ui->setLastRenderQueue(Ogre::RENDER_QUEUE_MAX);
    instance=Ogre::CompositorManager::getSingleton().addCompositor(&viewport,"Run3/LightingPost");
    if(!instance) throw std::runtime_error("Renderer lacks required HDR/MRT compositor capabilities");
    instance->addListener(this);instance->setEnabled(true);
    if(deferred) {
      Ogre::MaterialManager::getSingleton().addListener(this);
      listenerRegistered = true;
    }
  }
  ~Impl() {
    if(instance) Ogre::CompositorManager::getSingleton().removeCompositor(&viewport,"Run3/LightingPost");
    if(deferred) {
      if(listenerRegistered) Ogre::MaterialManager::getSingleton().removeListener(this);
      Ogre::RTShader::ShaderGenerator::getSingleton().getRenderState(gbuffer)->resetToBuiltinSubRenderStates();
      Ogre::RTShader::ShaderGenerator::getSingleton().getRenderState(transparent)->resetToBuiltinSubRenderStates();
    }
    Ogre::RTShader::ShaderGenerator::getSingleton().setTargetLinearColours(false);
  }
  Ogre::Technique *handleSchemeNotFound(unsigned short,const Ogre::String &scheme,
      Ogre::Material *material,unsigned short,const Ogre::Renderable *) override {
    if(scheme!=gbuffer && scheme!=transparent) return nullptr;
    if (material->getNumTechniques() == 0 || material->getTechnique(0)->getNumPasses() == 0)
      return nullptr;
    const auto *pass=material->getTechnique(0)->getPass(0);
    if(pass->hasFragmentProgram() &&
       pass->getFragmentProgramName().rfind("Run3/",0)==0)
      return material->getTechnique(0);
    configureDeferred(*material,pass->isTransparent()?Surface::Transparent:Surface::Opaque,
                      pass->getLightingEnabled());
    return material->getTechnique(scheme);
  }
  void notifyMaterialRender(Ogre::uint32 id,Ogre::MaterialPtr &material) override {
    const auto params=material->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
    if(id==1) {params->setNamedConstant("exposure",settings.exposure);return;}
    if(id!=2) return;
    const auto projection=camera.getProjectionMatrixWithRSDepth();
    params->setNamedConstant("inverseProjection",projection.inverse());
    params->setNamedConstant("projection",projection);
    params->setNamedConstant("farClip",camera.getFarClipDistance());
    const auto ambient=scene.getAmbientLight();
    params->setNamedConstant("ambient",Ogre::Vector3(ambient.r,ambient.g,ambient.b));
    std::array<Ogre::Vector4,64> positions{},directions{},colours{},attenuations{},cones{};
    // Ogre orders this list with shadow casters first. Keeping that exact
    // order makes G-buffer factor N refer to deferred light N.
    const auto &lights=scene._getLightsAffectingFrustum();
    if(lights.size()>64) throw std::runtime_error("Deferred light budget exceeded: 64; no lights silently discarded");
    const auto view=camera.getViewMatrix();
    for(std::size_t i=0;i<lights.size();++i) {
      const auto *light=lights[i];
      const auto pos=view*light->getDerivedPosition();
      const auto dir=view.linear()*light->getDerivedDirection();
      const float type=light->getType()==Ogre::Light::LT_DIRECTIONAL?0.0F:
          light->getType()==Ogre::Light::LT_POINT?1.0F:2.0F;
      positions[i]={pos.x,pos.y,pos.z,type};directions[i]={dir.x,dir.y,dir.z,0};
      const auto c=light->getDiffuseColour()*light->getPowerScale();
      colours[i]={c.r,c.g,c.b,light->getCastShadows() ? 1.0F:0.0F};
      attenuations[i]={light->getAttenuationRange(),light->getAttenuationConstant(),light->getAttenuationLinear(),light->getAttenuationQuadric()};
      cones[i]={std::cos(light->getSpotlightInnerAngle().valueRadians()*.5F),std::cos(light->getSpotlightOuterAngle().valueRadians()*.5F),light->getSpotlightFalloff(),0};
    }
    params->setNamedConstant("lightCount",static_cast<int>(lights.size()));
    params->setNamedConstant("lightPosition",positions.front().ptr(),positions.size());
    params->setNamedConstant("lightDirection",directions.front().ptr(),directions.size());
    params->setNamedConstant("lightColour",colours.front().ptr(),colours.size());
    params->setNamedConstant("attenuation",attenuations.front().ptr(),attenuations.size());
    params->setNamedConstant("cone",cones.front().ptr(),cones.size());
  }
};
LightingPost::LightingPost(Ogre::SceneManager &s,Ogre::Camera &c,Ogre::Viewport &v,LightingSettings config)
    :impl_(std::make_unique<Impl>(s,c,v,config)) { impl_->initialise(); }
LightingPost::~LightingPost()=default;
nlohmann::json LightingPost::report() const {
  nlohmann::json result;
  result["hdr_batches"]=impl_->instance->getRenderTarget("hdr")->getStatistics().batchCount;
  if(!impl_->deferred) return result;
  result["gbuffer_batches"]=impl_->instance->getRenderTarget("gbuffer")->getStatistics().batchCount;
  for(unsigned index=0;index<5;++index) {
    const auto &texture=impl_->instance->getTextureInstance("gbuffer",index);
    std::vector<float> data(texture->getWidth()*texture->getHeight()*4);
    texture->getBuffer()->blitToMemory(Ogre::PixelBox(texture->getWidth(),texture->getHeight(),1,Ogre::PF_FLOAT32_RGBA,data.data()));
    std::array<float,4> maximum{};
    for(std::size_t pixel=0;pixel<data.size();pixel+=4)
      for(std::size_t channel=0;channel<4;++channel)
        maximum[channel]=std::max(maximum[channel],data[pixel+channel]);
    result["gbuffer_maxima"].push_back(maximum);
  }
  return result;
}
void LightingPost::configureDeferred(Ogre::Material &material,Surface surface, bool lit, bool reflection) {
  auto &generator=Ogre::RTShader::ShaderGenerator::getSingleton();
  const bool forward=!lit || reflection || surface==Surface::Transparent ||
                     surface==Surface::Additive || surface==Surface::Unlit;
  const char *emptyScheme=forward?gbuffer:transparent;
  const char *drawScheme=forward?transparent:gbuffer;
  if(!material.getTechnique(emptyScheme)) {
    auto *empty=material.createTechnique();empty->setName(emptyScheme);empty->setSchemeName(emptyScheme);
  }
  if(!material.getTechnique(drawScheme)) {
    if(!generator.createShaderBasedTechnique(material.getTechnique(0),drawScheme))
      throw std::runtime_error("Cannot create deferred material scheme: "+material.getName());
    if (forward) {
      auto *source = generator.getRenderState(Ogre::MSN_SHADERGEN, material, 0);
      auto *target = generator.getRenderState(drawScheme, material, 0);
      if (source && target)
        for (auto *state : source->getSubRenderStates()) {
          auto *copy = generator.createSubRenderState(state->getType());
          copy->copyFrom(*state);
          target->addTemplateSubRenderState(copy);
        }
    }
    generator.validateMaterial(drawScheme,material.getName(),material.getGroup());
    for(auto *technique:material.getTechniques())
      if(technique->getSchemeName()==drawScheme) technique->setName(drawScheme);
  }
}
}
