#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace run3::rendering {

enum class LightingPipeline { LegacyForward, Deferred, Pbr, FastForward };
enum class ShadowQuality { Off, Low, Medium, High, Ultra };
enum class Surface { Opaque, Cutout, Transparent, Unlit };
enum class ColourSpace { Srgb, Linear };
enum class ReflectionMapping { None, Spherical, Cube, CubeDirection };
struct TextureSlot {
  std::string name;
  ColourSpace colourSpace{ColourSpace::Linear};
};
struct MaterialDescription {
  std::string name;
  Surface surface{Surface::Opaque};
  TextureSlot diffuseMap{{}, ColourSpace::Srgb};
  TextureSlot normalMap, specularMap, metalRoughnessMap, aoMap;
  TextureSlot reflectionMap{{}, ColourSpace::Srgb};
  ReflectionMapping reflectionMapping{ReflectionMapping::None};
  float reflectionWeight{0.25F};
  bool lighting{true}; // Independent of opacity: unlit leaves still alpha-test.
  std::vector<std::string> compatibilityNotes;
  std::array<float, 4> diffuse{1, 1, 1, 1};
  std::array<float, 3> specular{0.04F, 0.04F, 0.04F}, emissive{0, 0, 0};
  float shininess{32}, roughness{0.5F}, metallic{}, alphaCutoff{0.5F};
  unsigned lightLimit{8};
  bool doubleSided{}, castShadows{true}, receiveShadows{true};
};
struct LightingSettings {
  LightingPipeline pipeline{LightingPipeline::LegacyForward};
  ShadowQuality shadows{ShadowQuality::Off};
  float exposure{1.0F};
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
