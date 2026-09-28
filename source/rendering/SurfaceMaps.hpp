#pragma once
#include <OgreShaderSubRenderState.h>
#include <OgreShaderProgramSet.h>
#include <OgreShaderProgram.h>
#include <OgreShaderFunction.h>
#include <OgreShaderFFPRenderState.h>

namespace run3::rendering {
// Private RTSS extension: data textures are never multiplied into the albedo
// by FFP texturing. PBR AO affects indirect light before direct-light evaluation.
class SurfaceMaps final : public Ogre::RTShader::SubRenderState {
public:
  int specularIndex{-1}, aoIndex{-1};
  bool pbr{};
  bool unlitTint{};
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3SurfaceMaps"; return type;
  }
  int getExecutionOrder() const override { return Ogre::RTShader::FFP_TEXTURING+10; }
  void copyFrom(const Ogre::RTShader::SubRenderState &source) override {
    const auto &other=static_cast<const SurfaceMaps &>(source);
    specularIndex=other.specularIndex;aoIndex=other.aoIndex;pbr=other.pbr;
    unlitTint=other.unlitTint;
  }
  bool createCpuSubPrograms(Ogre::RTShader::ProgramSet *programs) override {
    using namespace Ogre;
    using namespace Ogre::RTShader;
    auto *vs=programs->getCpuProgram(GPT_VERTEX_PROGRAM);
    auto *ps=programs->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    auto *vertex=vs->getMain(); auto *fragment=ps->getMain();
    if (unlitTint) {
      auto tint = ps->resolveParameter(GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
      auto output = fragment->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);
      fragment->getStage(FFP_PS_COLOUR_BEGIN+1).mul(output,tint,output);
    }
    if (specularIndex < 0 && aoIndex < 0) return true;
    auto outUv=vertex->getOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
    if(!outUv) {
      auto uv=vertex->resolveInputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
      outUv=vertex->resolveOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
      vertex->getStage(FFP_VS_TEXTURING).assign(uv,outUv);
    }
    auto uv=fragment->resolveInputParameter(outUv);
    ps->addDependency("FFPLib_Texturing");
    if(specularIndex>=0) {
      auto sampler=ps->resolveParameter(GCT_SAMPLER2D,"run3Specular",specularIndex);
      auto texel=fragment->resolveLocalParameter(GCT_FLOAT4,"run3SpecularTexel");
      auto stage=fragment->getStage(pbr ? FFP_PS_PBR_LIGHTING_BEGIN+1 : FFP_PS_COLOUR_END-1);
      stage.sampleTexture(sampler,uv,texel);
      if (pbr) {
        // After PBR_MakeParams, before IBL/direct lighting. A legacy specular
        // mask changes dielectric reflectance, never invents metalness.
        ps->addDependency("Run3SurfaceMaps");
        auto pixel = fragment->getLocalParameter("pixel");
        if (!pixel) throw std::runtime_error("PBR specular map requires PixelParams");
        stage.callFunction("Run3_ApplySpecularMask", In(texel).xyz(), InOut(pixel));
      } else {
        auto specular=fragment->resolveLocalParameter(Parameter::SPC_COLOR_SPECULAR);
        stage.mul(In(specular).xyz(),In(texel).xyz(),Out(specular).xyz());
      }
    }
    if(aoIndex>=0) {
      auto sampler=ps->resolveParameter(GCT_SAMPLER2D,"run3Ao",aoIndex);
      auto texel=fragment->resolveLocalParameter(GCT_FLOAT4,"run3AoTexel");
      auto output=fragment->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);
      auto stage=fragment->getStage(pbr?FFP_PS_PBR_LIGHTING_BEGIN+6:FFP_PS_COLOUR_END-1);
      stage.sampleTexture(sampler,uv,texel);
      stage.mul(In(output).xyz(),In(texel).x(),Out(output).xyz());
    }
    return true;
  }
};
class SurfaceMapsFactory final : public Ogre::RTShader::SubRenderStateFactory {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3SurfaceMaps"; return type;
  }
protected:
  Ogre::RTShader::SubRenderState *createInstanceImpl() override { return new SurfaceMaps; }
};
class GBufferSurface final : public Ogre::RTShader::SubRenderState {
public:
  int specularIndex{-1}, aoIndex{-1};
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3GBufferSurface";return type;
  }
  int getExecutionOrder() const override {return Ogre::RTShader::FFP_TEXTURING+20;}
  void copyFrom(const Ogre::RTShader::SubRenderState &source) override {
    const auto &other=static_cast<const GBufferSurface &>(source);
    specularIndex=other.specularIndex;aoIndex=other.aoIndex;
  }
  bool createCpuSubPrograms(Ogre::RTShader::ProgramSet *programs) override {
    using namespace Ogre;using namespace Ogre::RTShader;
    auto *ps=programs->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    auto *fragment=ps->getMain();
    auto output=fragment->resolveOutputParameter(Parameter::SPS_COLOR,2,Parameter::SPC_UNKNOWN,GCT_FLOAT4);
    auto specular=ps->resolveParameter(GpuProgramParameters::ACT_SURFACE_SPECULAR_COLOUR);
    auto stage=fragment->getStage(FFP_PS_COLOUR_END+1);
    stage.assign(In(specular).xyz(),Out(output).xyz());
    stage.assign(1.0F,Out(output).w());
    if(specularIndex<0 && aoIndex<0) return true;
    auto *vertex=programs->getCpuProgram(GPT_VERTEX_PROGRAM)->getMain();
    auto outUv=vertex->getOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
    if(!outUv) {
      auto uv=vertex->resolveInputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
      outUv=vertex->resolveOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0,GCT_FLOAT2);
      vertex->getStage(FFP_VS_TEXTURING).assign(uv,outUv);
    }
    auto uv=fragment->resolveInputParameter(outUv);
    ps->addDependency("FFPLib_Texturing");
    auto texel=fragment->resolveLocalParameter(GCT_FLOAT4,"run3GbufferTexel");
    if(specularIndex>=0) {
      auto sampler=ps->resolveParameter(GCT_SAMPLER2D,"run3Specular",specularIndex);
      stage.sampleTexture(sampler,uv,texel);
      stage.mul(In(output).xyz(),In(texel).xyz(),Out(output).xyz());
    }
    if(aoIndex>=0) {
      auto sampler=ps->resolveParameter(GCT_SAMPLER2D,"run3Ao",aoIndex);
      stage.sampleTexture(sampler,uv,texel);
      stage.assign(In(texel).x(),Out(output).w());
    }
    return true;
  }
};
class GBufferSurfaceFactory final : public Ogre::RTShader::SubRenderStateFactory {
public:
  const Ogre::String &getType() const override {
    static const Ogre::String type="Run3GBufferSurface";return type;
  }
protected:
  Ogre::RTShader::SubRenderState *createInstanceImpl() override {return new GBufferSurface;}
};
}
