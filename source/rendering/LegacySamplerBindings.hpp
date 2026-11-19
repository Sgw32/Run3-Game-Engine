#pragma once

#include <regex>
#include <string>
#include <vector>

namespace run3::rendering::detail {

inline std::string separateD3D11SamplerBindings(std::string source) {
  // Cg's sN binds both a texture and its sampler. HLSL compatibility mode
  // preserves sN but assigns tN independently, in order of texture use.
  // Effect8 samples pat first: without explicit tN, scene and pattern swap.
  // Handle both global samplers (Effect8) and function parameters (Glow).
  const std::string binding =
      R"(([A-Za-z_][A-Za-z0-9_]*)\s*:\s*register\s*\(\s*s([0-9]+)\s*\))";
  const std::regex global("\\bsampler(?:2D)?\\s+" + binding + R"(\s*;)");
  const std::regex parameter("\\buniform\\s+sampler2D\\s+" + binding);
  std::vector<std::string> samplers;
  for (const auto *declaration : {&global, &parameter}) {
    for (std::sregex_iterator match(source.begin(), source.end(), *declaration),
         end; match != end; ++match)
      samplers.push_back((*match)[1].str());
  }
  source = std::regex_replace(source, global,
      "Texture2D $1Texture : register(t$2); "
      "SamplerState $1Sampler : register(s$2);");
  source = std::regex_replace(source, parameter,
      "uniform Texture2D $1Texture : register(t$2), "
      "uniform SamplerState $1Sampler : register(s$2)");
  for (const auto &sampler : samplers)
    source = std::regex_replace(source,
        std::regex(R"(\btex2D\s*\(\s*)" + sampler + R"(\s*,\s*)"),
        sampler + "Texture.Sample(" + sampler + "Sampler, ");
  return source;
}

} // namespace run3::rendering::detail
