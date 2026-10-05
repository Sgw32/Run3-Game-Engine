#pragma once
#include "SurfaceMaps.hpp"
#include <OgrePass.h>

namespace run3::rendering {
// Ogre sample-style MRT geometry stage, with an extra shadow mask. Unlike the
// built-in GBuffer stage, this never mutates Material::receiveShadows, so RTSS
// PSSM reception can be retained across shader regeneration and map reloads.
class GBufferGeometry final : public Ogre::RTShader::SubRenderState {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3GBufferGeometry";return type;
  }
  int getExecutionOrder() const override {return Ogre::RTShader::FFP_LIGHTING;}
  void copyFrom(const Ogre::RTShader::SubRenderState &) override {}
  bool preAddToRenderState(const Ogre::RTShader::RenderState *, Ogre::Pass *source,
                           Ogre::Pass *) override {
    return source->getLightingEnabled();
  }
  bool createCpuSubPrograms(Ogre::RTShader::ProgramSet *programs) override {
    using namespace Ogre;using namespace Ogre::RTShader;
    auto *vs=programs->getCpuProgram(GPT_VERTEX_PROGRAM);
    auto *ps=programs->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    auto *vertex=vs->getMain();auto *fragment=ps->getMain();
    auto position=vertex->resolveInputParameter(Parameter::SPC_POSITION_OBJECT_SPACE);
    auto viewPosition=vertex->resolveOutputParameter(Parameter::SPC_POSITION_VIEW_SPACE);
    auto worldView=vs->resolveParameter(GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
    vs->addDependency("FFPLib_Transform");
    vertex->getStage(FFP_VS_POST_PROCESS).callFunction("FFP_Transform",worldView,position,viewPosition);
    auto viewNormal=fragment->getLocalParameter(Parameter::SPC_NORMAL_VIEW_SPACE);
    if(!viewNormal) {
      auto input=vertex->resolveInputParameter(Parameter::SPC_NORMAL_OBJECT_SPACE);
      auto output=vertex->resolveOutputParameter(Parameter::SPC_NORMAL_VIEW_SPACE);
      auto matrix=vs->resolveParameter(GpuProgramParameters::ACT_NORMAL_MATRIX);
      vertex->getStage(FFP_VS_LIGHTING).callBuiltin("mul",matrix,input,output);
      viewNormal=fragment->resolveInputParameter(output);
    }
    auto diffuse=fragment->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);
    auto normalDepth=fragment->resolveOutputParameter(Parameter::SPC_COLOR_SPECULAR);
    auto shadow0=fragment->resolveOutputParameter(Parameter::SPS_COLOR,3,Parameter::SPC_UNKNOWN,GCT_FLOAT4);
    auto shadow1=fragment->resolveOutputParameter(Parameter::SPS_COLOR,4,Parameter::SPC_UNKNOWN,GCT_FLOAT4);
    auto surface=ps->resolveParameter(GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
    fragment->getStage(FFP_PS_COLOUR_BEGIN+1).assign(surface,diffuse);
    auto stage=fragment->getStage(FFP_PS_COLOUR_END+2);
    auto shininess=ps->resolveParameter(GpuProgramParameters::ACT_SURFACE_SHININESS);
    stage.assign(shininess,Out(diffuse).w());
    stage.assign(viewNormal,Out(normalDepth).xyz());
    auto positionIn=fragment->resolveInputParameter(viewPosition);
    auto farClip=ps->resolveParameter(GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
    stage.callBuiltin("length",positionIn,Out(normalDepth).w());
    stage.div(In(normalDepth).w(),farClip,Out(normalDepth).w());
    stage.assign(Vector4(1),shadow0);
    stage.assign(Vector4(1),shadow1);
    if(auto factor=fragment->getLocalParameter("lShadowFactor")) {
      stage.assign({In(factor),At(0),Out(shadow0).x()});
      stage.assign({In(factor),At(1),Out(shadow0).y()});
      stage.assign({In(factor),At(2),Out(shadow0).z()});
      stage.assign({In(factor),At(3),Out(shadow0).w()});
      stage.assign({In(factor),At(4),Out(shadow1).x()});
      stage.assign({In(factor),At(5),Out(shadow1).y()});
    }
    return true;
  }
};
class GBufferGeometryFactory final : public Ogre::RTShader::SubRenderStateFactory {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3GBufferGeometry";return type;
  }
protected:
  Ogre::RTShader::SubRenderState *createInstanceImpl() override {return new GBufferGeometry;}
};
}
