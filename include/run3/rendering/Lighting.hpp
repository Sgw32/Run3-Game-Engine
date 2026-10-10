#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace run3::rendering {

enum class LightingPipeline { LegacyForward, Deferred, Pbr, FastForward };
enum class ShadowQuality { Off, Low, Medium, High, Ultra };
enum class Surface { Opaque, Cutout, Transparent, Additive, Unlit };
enum class ColourSpace { Srgb, Linear };
enum class ReflectionMapping { None, Spherical, Cube, CubeDirection };
struct TextureSlot {
  std::string name;
  ColourSpace colourSpace{ColourSpace::Linear};
};
enum class TextureTransform { TranslateU, TranslateV, ScaleU, ScaleV, Rotate };
enum class TextureWaveform { Sine, Triangle, Square, Sawtooth, InverseSawtooth };
struct TextureWaveAnimation {
  TextureTransform transform{TextureTransform::TranslateU};
  TextureWaveform waveform{TextureWaveform::Sine};
  float base{}, frequency{1.0F}, phase{}, amplitude{1.0F};
};
struct MaterialDescription {
  std::string name;
  Surface surface{Surface::Opaque};
  TextureSlot diffuseMap{{}, ColourSpace::Srgb};
  // Legacy anim_texture frames share the diffuse texture unit. An empty list
  // is an ordinary static diffuse texture.
  std::vector<std::string> diffuseAnimationFrames;
  std::string diffuseAnimationBase;
  unsigned diffuseAnimationFrameCount{};
  float diffuseAnimationDuration{};
  float diffuseScrollU{}, diffuseScrollV{}, diffuseRotate{};
  std::vector<TextureWaveAnimation> diffuseWaveAnimations;
  TextureSlot normalMap, specularMap, metalRoughnessMap, aoMap;
  TextureSlot reflectionMap{{}, ColourSpace::Srgb};
  ReflectionMapping reflectionMapping{ReflectionMapping::None};
  float reflectionWeight{0.25F};
  bool lighting{true}; // Independent of opacity: unlit leaves still alpha-test.
  std::vector<std::string> compatibilityNotes;
  std::array<float, 4> diffuse{1, 1, 1, 1};
  std::array<float, 3> specular{0.025F, 0.025F, 0.025F}, emissive{0, 0, 0};
  float shininess{64}, roughness{0.35F}, metallic{}, alphaCutoff{0.5F};
  unsigned lightLimit{8};
  bool doubleSided{}, castShadows{true}, receiveShadows{true};
};
struct LightingSettings {
  LightingPipeline pipeline{LightingPipeline::LegacyForward};
  ShadowQuality shadows{ShadowQuality::Off};
  float exposure{1.0F};
  // One is Ogre's original every-frame behaviour. The application selects a
  // conservative interval of two for legacy/fast-forward authored lighting.
  unsigned shadowUpdateInterval{1};
};
struct ShadowBudget {
  unsigned resolution{}, textures{}, splits{}, filterSamples{};
  float distance{};
};
LightingPipeline parseLightingPipeline(std::string_view value);
ShadowQuality parseShadowQuality(std::string_view value);
std::string_view pipelineName(LightingPipeline value);
ShadowBudget shadowBudget(ShadowQuality quality, LightingPipeline pipeline);
float roughnessFromShininess(float shininess);
// Validates constants and drops only unavailable optional texture slots.
// The returned diagnostics are mandatory presentation/log output.
std::vector<std::string> resolveMaterial(
    MaterialDescription &material, const std::vector<std::string> &available);

} // namespace run3::rendering
