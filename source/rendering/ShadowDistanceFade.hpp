#pragma once

#include "SurfaceMaps.hpp"
#include <run3/rendering/Lighting.hpp>

namespace run3::rendering {

// Fade the directional entries of Ogre's shared shadow-factor array before
// forward lighting or the deferred G-buffer consumes them. View-space length
// gives a spherical, orientation-independent fade; spotlight ranges are kept.
class ShadowDistanceFade final : public Ogre::RTShader::SubRenderState {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type = "Run3ShadowDistanceFade";
    return type;
  }
  int getExecutionOrder() const override { return Ogre::RTShader::FFP_LIGHTING; }
  void copyFrom(const Ogre::RTShader::SubRenderState &) override {}
  bool createCpuSubPrograms(Ogre::RTShader::ProgramSet *programs) override {
    using namespace Ogre;
    using namespace Ogre::RTShader;
    auto *vs = programs->getCpuProgram(GPT_VERTEX_PROGRAM);
    auto *ps = programs->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    auto *vertex = vs->getMain();
    auto *fragment = ps->getMain();
    const auto factors = fragment->getLocalParameter("lShadowFactor");
    if (!factors) return true; // materials which do not receive shadows
    const auto count = static_cast<unsigned>(factors->getSize());
    auto position = vertex->getLocalParameter(Parameter::SPC_POSITION_OBJECT_SPACE);
    if (!position) position = vertex->getInputParameter(Parameter::SPC_POSITION_OBJECT_SPACE);
    const auto viewPosition = vertex->resolveOutputParameter(Parameter::SPC_POSITION_VIEW_SPACE);
    const auto worldView = vs->resolveParameter(GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
    vs->addDependency("FFPLib_Transform");
    vertex->getStage(FFP_VS_POST_PROCESS).callFunction(
        "FFP_Transform", worldView, position, viewPosition);
    const auto viewInput = fragment->resolveInputParameter(viewPosition);
    const auto lights = ps->resolveParameter(GpuProgramParameters::ACT_LIGHT_POSITION_ARRAY, count);
    ps->addDependency("SGXLib_IntegratedPSSM");
    auto stage = fragment->getStage(FFP_PS_COLOUR_BEGIN + 1);
    for (unsigned index = 0; index < count; ++index) {
      stage.callFunction("Run3_FadeDirectionalShadow", std::vector<Operand>{
          In(viewInput), In(lights), At(index),
          In(Vector2(directionalShadowFadeStart, directionalShadowFadeEnd)),
          InOut(factors), At(index)});
    }
    return true;
  }
};

class ShadowDistanceFadeFactory final : public Ogre::RTShader::SubRenderStateFactory {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type = "Run3ShadowDistanceFade";
    return type;
  }
protected:
  Ogre::RTShader::SubRenderState *createInstanceImpl() override {
    return new ShadowDistanceFade;
  }
};

} // namespace run3::rendering
