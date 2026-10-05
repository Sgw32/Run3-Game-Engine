#include <run3/rendering/Lighting.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace run3::rendering {
LightingPipeline parseLightingPipeline(std::string_view value) {
  if (value == "legacy-forward") return LightingPipeline::LegacyForward;
  if (value == "deferred") return LightingPipeline::Deferred;
  if (value == "pbr") return LightingPipeline::Pbr;
  if (value == "fast-forward") return LightingPipeline::FastForward;
  throw std::invalid_argument("Unknown lighting pipeline: " + std::string(value));
}
ShadowQuality parseShadowQuality(std::string_view value) {
  if (value == "off") return ShadowQuality::Off;
  if (value == "low") return ShadowQuality::Low;
  if (value == "medium") return ShadowQuality::Medium;
  if (value == "high") return ShadowQuality::High;
  if (value == "ultra") return ShadowQuality::Ultra;
  throw std::invalid_argument("Unknown shadow quality: " + std::string(value));
}
std::string_view pipelineName(LightingPipeline value) {
  switch (value) {
  case LightingPipeline::LegacyForward: return "legacy-forward";
  case LightingPipeline::Deferred: return "deferred";
  case LightingPipeline::Pbr: return "pbr";
  case LightingPipeline::FastForward: return "fast-forward";
  }
  throw std::invalid_argument("Invalid lighting pipeline enum");
}
ShadowBudget shadowBudget(ShadowQuality quality, LightingPipeline pipeline) {
  if (quality == ShadowQuality::Off) return {};
  const unsigned tier = static_cast<unsigned>(quality);
  static_cast<void>(pipeline);
  // One texture is assigned to each shadow-casting light. Six covers the
  // largest authored light set in the shipped maps and leaves enough D3D11
  // sampler slots for the complete legacy/PBR surface texture set.
  // 512, 1024, 2048, 4096 - shadow map size depending on quality
  return {512U << (std::min(tier, 4U) - 1U), 6U, 1U,
          tier >= 3 ? 16U : 4U, 2500.0F * static_cast<float>(1U << (tier - 1U))};
}
float roughnessFromShininess(float shininess) {
  if (!std::isfinite(shininess) || shininess < 0)
    throw std::invalid_argument("Invalid material shininess");
  return std::clamp(std::sqrt(2.0F / (shininess + 2.0F)), 0.045F, 1.0F);
}
std::vector<std::string> resolveMaterial(
    MaterialDescription &material, const std::vector<std::string> &available) {
  if (!std::isfinite(material.roughness) || material.roughness < 0 ||
      material.roughness > 1 || !std::isfinite(material.metallic) ||
      material.metallic < 0 || material.metallic > 1 || material.lightLimit == 0)
    throw std::invalid_argument("Invalid surface parameters for " + material.name);
  std::vector<std::string> warnings = material.compatibilityNotes;
  for (auto *slot : {&material.diffuseMap, &material.normalMap,
                    &material.specularMap, &material.metalRoughnessMap,
                    &material.aoMap, &material.reflectionMap}) {
    if (!slot->name.empty() &&
        std::find(available.begin(), available.end(), slot->name) == available.end()) {
      warnings.push_back(material.name + ": missing texture '" + slot->name +
                         "'; using constant surface fallback");
      slot->name.clear();
    }
  }
  if (!material.diffuseAnimationFrames.empty()) {
    const auto missing = [&](const std::string &frame) {
      return std::find(available.begin(), available.end(), frame) ==
             available.end();
    };
    const auto firstMissing = std::find_if(material.diffuseAnimationFrames.begin(),
                                           material.diffuseAnimationFrames.end(),
                                           missing);
    if (firstMissing != material.diffuseAnimationFrames.end()) {
      warnings.push_back(material.name + ": incomplete animated texture; "
                         "using the first available frame");
      material.diffuseAnimationFrames.clear();
      material.diffuseAnimationDuration = 0.0F;
    }
  }
  if (material.diffuseMap.name.empty()) {
    material.diffuseAnimationFrames.clear();
    material.diffuseAnimationBase.clear();
    material.diffuseAnimationFrameCount = 0;
    material.diffuseAnimationDuration = 0.0F;
  }
  if (material.specularMap.name.empty()) {
    // A mask can legitimately author strong reflections. Without one, keep a
    // small, tight fallback instead of washing the whole mesh in a broad lobe.
    for (float &channel : material.specular)
      channel = std::clamp(channel, 0.0F, 0.08F);
    material.shininess = std::max(material.shininess, 64.0F);
    material.roughness = std::min(material.roughness, 0.35F);
  }
  return warnings;
}
} // namespace run3::rendering
