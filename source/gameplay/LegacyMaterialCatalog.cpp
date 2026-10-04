#include <run3/gameplay/LegacyMaterialCatalog.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace run3::gameplay {
namespace fs = std::filesystem;
namespace {

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

std::size_t count(std::string_view value, char character) {
  return static_cast<std::size_t>(std::count(value.begin(), value.end(), character));
}

struct SourceMaterial {
  std::string parent;
  std::string aliasTexture;
  std::string directTexture;
  std::vector<std::string> animationFrames;
  std::string animationBase;
  unsigned animationFrameCount{};
  float animationDuration{};
  std::optional<std::array<float, 2>> scroll;
  std::optional<float> rotate;
  std::vector<rendering::TextureWaveAnimation> waveAnimations;
  std::optional<bool> lighting;
  bool transparent{};
  bool doubleSided{};
  std::string normal, specularMap, ao, metalRoughness;
  std::optional<std::array<float, 4>> diffuse;
  std::optional<std::array<float, 3>> specular;
  std::optional<float> shininess;
  std::optional<unsigned> lightLimit;
  std::optional<float> alphaCutoff;
  unsigned fixedLightCount{};
  unsigned techniques{};
  bool oncePerLight{};
  std::string reflection;
  rendering::ReflectionMapping reflectionMapping{rendering::ReflectionMapping::None};
  std::optional<float> reflectionWeight;
  bool volumeTexture{};
};

std::string unquote(std::string value) {
  if (value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') ||
       (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

} // namespace

struct LegacyMaterialCatalog::Impl {
  std::unordered_map<std::string, SourceMaterial> source;
  std::unordered_map<std::string, std::string> canonicalNames;

  void parse(const fs::path &path) {
    std::ifstream stream(path);
    if (!stream) {
      return;
    }

    std::string activeName;
    SourceMaterial *active{};
    std::size_t depth{};
    std::size_t textureDepth{};
    std::string textureUnit, unitTexture;
    std::string line;
    while (std::getline(stream, line)) {
      if (const auto comment = line.find("//"); comment != std::string::npos) {
        line.erase(comment);
      }
      const std::string cleaned = trim(line);
      if (cleaned.empty()) {
        continue;
      }

      if (active == nullptr && cleaned.rfind("material ", 0) == 0) {
        std::string declaration = trim(cleaned.substr(9));
        if (const auto brace = declaration.find('{'); brace != std::string::npos) {
          declaration.erase(brace);
        }
        std::string parent;
        if (const auto colon = declaration.find(':'); colon != std::string::npos) {
          parent = trim(declaration.substr(colon + 1));
          declaration.erase(colon);
        }
        std::istringstream nameStream(trim(declaration));
        nameStream >> activeName;
        activeName = unquote(activeName);
        if (activeName.empty()) {
          continue;
        }
        SourceMaterial &material = source[activeName];
        material = {};
        material.parent = unquote(std::move(parent));
        canonicalNames[lower(activeName)] = activeName;
        active = &material;
        depth = count(cleaned, '{') - count(cleaned, '}');
        if (depth == 0 && cleaned.find('{') != std::string::npos) {
          active = nullptr;
          activeName.clear();
        }
        continue;
      }

      if (active == nullptr) {
        continue;
      }

      // Many legacy GUI materials use the compact form
      // `{ set_texture_alias tex image.png }`. Braces are structural tokens,
      // not part of the directive, so strip leading braces before tokenizing.
      std::string statement = cleaned;
      while (!statement.empty() && statement.front() == '{')
        statement = trim(statement.substr(1));
      while (!statement.empty() && statement.back() == '}') {
        statement.pop_back();
        statement = trim(statement);
      }
      std::istringstream tokens(statement);
      std::string keyword;
      tokens >> keyword;
      // Subsequent techniques are fallbacks/deferred exports, not overrides
      // of the selected forward material's alpha, lighting or texture state.
      if (active->techniques > 1 && keyword != "technique") keyword.clear();
      if (keyword == "texture_unit") {
        tokens >> textureUnit;
        textureUnit = lower(textureUnit);
        textureUnit.erase(std::remove(textureUnit.begin(), textureUnit.end(), '_'), textureUnit.end());
        unitTexture.clear();
        textureDepth = depth + 1;
      }
      if (keyword == "set_texture_alias") {
        std::string alias;
        std::string texture;
        tokens >> alias >> texture;
        alias = lower(alias);
        alias.erase(std::remove(alias.begin(), alias.end(), '_'), alias.end());
        if ((alias == "maintexture" || alias == "tex" ||
             alias == "diffusemap" || alias == "albedo") &&
            !texture.empty()) {
          active->aliasTexture = unquote(std::move(texture));
        } else if (alias == "normalmap") {
          active->normal = unquote(texture);
        } else if (alias == "specularmap") {
          active->specularMap = unquote(texture);
        } else if (alias == "aomap") {
          active->ao = unquote(texture);
        } else if (alias == "metalroughnessmap") {
          active->metalRoughness = unquote(texture);
        }
      } else if (keyword == "technique") {
        ++active->techniques;
      } else if (keyword == "iteration" && active->techniques <= 1) {
        std::string mode; tokens >> mode;
        active->oncePerLight = mode == "once_per_light";
      } else if (keyword == "param_named_auto" && active->techniques <= 1) {
        std::string name, semantic; unsigned index{};
        if (tokens >> name >> semantic >> index;
            semantic == "light_position_object_space" || semantic == "light_position_object_space_array")
          active->fixedLightCount = std::max(active->fixedLightCount,
              semantic == "light_position_object_space_array" ? index : index + 1);
      } else if (keyword == "max_lights" && active->techniques <= 1) {
        unsigned limit{};
        if (tokens >> limit; limit != 0) active->lightLimit = limit;
      } else if (keyword == "alpha_rejection") {
        std::string comparison; unsigned threshold{};
        if (tokens >> comparison >> threshold;
            comparison == "greater_equal" || comparison == "greater")
          active->alphaCutoff = static_cast<float>(std::min(255U, threshold + (comparison == "greater" ? 1U : 0U))) / 255.0F;
      } else if (keyword == "set") {
        std::string variable; tokens >> variable;
        std::string valueText; std::getline(tokens, valueText);
        tokens.clear(); tokens.str(unquote(trim(valueText)));
        if (variable == "$diffuseCol") {
          std::array<float,4> value{1,1,1,1};
          if (tokens >> value[0] >> value[1] >> value[2]) {
            tokens >> value[3]; active->diffuse=value;
          }
        } else if (variable == "$specularCol") {
          std::array<float,3> value{};
          if (tokens >> value[0] >> value[1] >> value[2]) active->specular=value;
        } else if (variable == "$shininess") {
          float value{}; if (tokens >> value) active->shininess=value;
        }
      } else if (keyword == "diffuse") {
        std::array<float,4> value{1,1,1,1};
        if (tokens >> value[0] >> value[1] >> value[2]) {
          // Ignore ambient-only pass's black diffuse; keep the lit pass.
          if (value[0]+value[1]+value[2] > 0) {
            tokens >> value[3]; active->diffuse=value;
          }
        }
      } else if (keyword == "specular") {
        std::array<float,3> value{}; float exponent{};
        if (tokens >> value[0] >> value[1] >> value[2] >> exponent) {
          if(value[0]+value[1]+value[2] > 0) {
            active->specular=value; active->shininess=exponent;
          }
        }
      } else if (keyword == "texture" && active->techniques <= 1) {
        std::string dimension;
        tokens >> unitTexture >> dimension;
        unitTexture = unquote(unitTexture);
        if (dimension == "3d" || dimension == "1d" || dimension == "2darray") {
          // Volume noise belongs to its effect shader, never to UV0 albedo.
          active->volumeTexture = true;
        } else if (!unitTexture.empty() && unitTexture.front() != '$') {
          if (textureUnit == "normalmap") active->normal = unitTexture;
          else if (textureUnit == "specularmap") active->specularMap = unitTexture;
          else if (textureUnit == "aomap") active->ao = unitTexture;
          else if (textureUnit == "metalroughnessmap") active->metalRoughness = unitTexture;
          else if ((textureUnit.find("map") == std::string::npos || textureUnit == "diffusemap") && active->directTexture.empty())
            active->directTexture = unitTexture;
        }
      } else if (keyword == "anim_texture" && active->techniques <= 1 &&
                 (textureUnit.find("map") == std::string::npos ||
                  textureUnit == "diffusemap")) {
        std::vector<std::string> values;
        for (std::string value; tokens >> value;)
          values.push_back(unquote(std::move(value)));
        if (values.size() >= 2) {
          try {
            std::size_t consumed{};
            const float duration = std::stof(values.back(), &consumed);
            if (consumed == values.back().size() && duration >= 0.0F) {
              values.pop_back();
              unsigned frameCount{};
              std::size_t frameConsumed{};
              if (values.size() == 2) {
                try {
                  frameCount = static_cast<unsigned>(
                      std::stoul(values[1], &frameConsumed));
                } catch (const std::exception &) {
                  frameConsumed = 0;
                }
              }
              if (values.size() == 2 && frameConsumed == values[1].size() &&
                  frameCount > 0) {
                active->animationBase = values.front();
                active->animationFrameCount = frameCount;
                active->animationFrames.clear();
              } else {
                active->animationFrames = std::move(values);
                active->animationBase.clear();
                active->animationFrameCount = 0;
              }
              active->animationDuration = duration;
              if (!active->animationFrames.empty()) {
                active->directTexture = active->animationFrames.front();
              } else {
                const fs::path base(active->animationBase);
                active->directTexture =
                    (base.parent_path() /
                     (base.stem().string() + "_0" + base.extension().string()))
                        .generic_string();
              }
            }
          } catch (const std::exception &) {
            // Leave malformed legacy effects on the ordinary texture fallback.
          }
        }
      } else if (keyword == "scroll_anim" && active->techniques <= 1) {
        std::array<float, 2> value{};
        if (tokens >> value[0] >> value[1]) active->scroll = value;
      } else if (keyword == "rotate_anim" && active->techniques <= 1) {
        float value{};
        if (tokens >> value) active->rotate = value;
      } else if (keyword == "wave_xform" && active->techniques <= 1) {
        std::string transform, waveform;
        rendering::TextureWaveAnimation animation;
        if (tokens >> transform >> waveform >> animation.base >>
                animation.frequency >> animation.phase >> animation.amplitude) {
          const auto transformName = lower(transform);
          const auto waveformName = lower(waveform);
          bool validTransform = true;
          if (transformName == "scroll_x") animation.transform = rendering::TextureTransform::TranslateU;
          else if (transformName == "scroll_y") animation.transform = rendering::TextureTransform::TranslateV;
          else if (transformName == "scale_x") animation.transform = rendering::TextureTransform::ScaleU;
          else if (transformName == "scale_y") animation.transform = rendering::TextureTransform::ScaleV;
          else if (transformName == "rotate") animation.transform = rendering::TextureTransform::Rotate;
          else validTransform = false;
          if (waveformName == "triangle") animation.waveform = rendering::TextureWaveform::Triangle;
          else if (waveformName == "square") animation.waveform = rendering::TextureWaveform::Square;
          else if (waveformName == "sawtooth") animation.waveform = rendering::TextureWaveform::Sawtooth;
          else if (waveformName == "inverse_sawtooth") animation.waveform = rendering::TextureWaveform::InverseSawtooth;
          if (validTransform) active->waveAnimations.push_back(animation);
        }
      } else if (keyword == "cubic_texture" && active->techniques <= 1) {
        std::string coordinates;
        tokens >> active->reflection >> coordinates;
        active->reflection = unquote(active->reflection);
        active->reflectionMapping = coordinates == "separateUV"
            ? rendering::ReflectionMapping::CubeDirection : rendering::ReflectionMapping::Cube;
      } else if (keyword == "env_map" && active->techniques <= 1) {
        std::string mode; tokens >> mode;
        if (mode == "spherical" || mode == "cubic_reflection") {
          active->reflectionMapping = mode == "spherical" ? rendering::ReflectionMapping::Spherical : rendering::ReflectionMapping::Cube;
          if (active->reflection.empty()) active->reflection = unitTexture;
          if (active->directTexture == active->reflection) active->directTexture.clear();
        }
      } else if ((keyword == "colour_op_ex" || keyword == "color_op_ex") && active->techniques <= 1) {
        std::string operation, first, second; float weight{};
        if (tokens >> operation >> first >> second >> weight; operation == "blend_manual")
          active->reflectionWeight = weight;
      } else if (keyword == "scene_blend") {
        std::string mode;
        tokens >> mode;
        // Ordinary opaque Run3 materials use an additive second pass for
        // per-light accumulation. That does not make the whole material
        // transparent and must not disable compatibility-material depth writes.
        active->transparent = active->transparent || mode == "alpha_blend";
      } else if (keyword == "lighting") {
        std::string value;
        tokens >> value;
        active->lighting = lower(value) != "off";
      } else if (keyword == "depth_write") {
        std::string value;
        tokens >> value;
        active->transparent = active->transparent || value == "off";
      } else if (keyword == "cull_hardware") {
        std::string value;
        tokens >> value;
        active->doubleSided = value == "none";
      }

      const std::size_t opens = count(cleaned, '{');
      const std::size_t closes = count(cleaned, '}');
      if (opens >= closes) {
        depth += opens - closes;
      } else {
        const std::size_t reduction = closes - opens;
        depth = reduction >= depth ? 0 : depth - reduction;
      }
      if (depth == 0) {
        active = nullptr;
        activeName.clear();
      }
      if (textureDepth && depth < textureDepth && keyword != "texture_unit") {
        textureDepth = 0; textureUnit.clear(); unitTexture.clear();
      }
    }
  }

  std::optional<LegacyMaterialInfo>
  resolve(const std::string &requestedName,
          std::unordered_set<std::string> &visiting) const {
    auto found = source.find(requestedName);
    if (found == source.end()) {
      const auto canonical = canonicalNames.find(lower(requestedName));
      if (canonical == canonicalNames.end()) {
        return std::nullopt;
      }
      found = source.find(canonical->second);
    }
    if (found == source.end() || !visiting.insert(found->first).second) {
      return std::nullopt;
    }

    LegacyMaterialInfo result;
    result.texture = !found->second.aliasTexture.empty()
                         ? found->second.aliasTexture
                         : found->second.directTexture;
    result.lighting = found->second.lighting.value_or(true);
    if(found->second.oncePerLight || found->second.lightLimit || found->second.fixedLightCount)
      result.lighting = true;
    result.transparent = found->second.transparent;
    result.doubleSided = found->second.doubleSided;
    if (!found->second.parent.empty()) {
      if (const auto parent = resolve(found->second.parent, visiting)) {
        result.surface = parent->surface;
        if (result.texture.empty()) {
          result.texture = parent->texture;
        }
        if (!found->second.lighting.has_value()) {
          result.lighting = parent->lighting;
        }
        result.transparent = result.transparent || parent->transparent;
        result.doubleSided = result.doubleSided || parent->doubleSided;
      }
    }
    auto &surface = result.surface;
    const auto &input = found->second;
    if (!input.aliasTexture.empty() || !input.directTexture.empty()) {
      surface.diffuseAnimationFrames.clear();
      surface.diffuseAnimationBase.clear();
      surface.diffuseAnimationFrameCount = 0;
      surface.diffuseAnimationDuration = 0.0F;
    }
    surface.name = requestedName;
    surface.diffuseMap.name = result.texture;
    if (!input.animationFrames.empty()) {
      surface.diffuseAnimationFrames = input.animationFrames;
      surface.diffuseAnimationDuration = input.animationDuration;
    } else if (!input.animationBase.empty()) {
      surface.diffuseAnimationBase = input.animationBase;
      surface.diffuseAnimationFrameCount = input.animationFrameCount;
      surface.diffuseAnimationDuration = input.animationDuration;
    }
    if (input.scroll) {
      surface.diffuseScrollU = (*input.scroll)[0];
      surface.diffuseScrollV = (*input.scroll)[1];
    }
    if (input.rotate) surface.diffuseRotate = *input.rotate;
    if (!input.waveAnimations.empty())
      surface.diffuseWaveAnimations = input.waveAnimations;
    if (!input.normal.empty()) surface.normalMap.name = input.normal;
    if (!input.specularMap.empty()) surface.specularMap.name = input.specularMap;
    if (!input.ao.empty()) surface.aoMap.name = input.ao;
    if (!input.metalRoughness.empty()) surface.metalRoughnessMap.name = input.metalRoughness;
    if (!input.reflection.empty()) {
      surface.reflectionMap.name = input.reflection;
      surface.reflectionMapping = input.reflectionMapping;
      surface.reflectionWeight = input.reflectionWeight.value_or(result.texture.empty() ? 1.0F : 0.25F);
    }
    if (input.volumeTexture) {
      surface.compatibilityNotes.push_back(requestedName + ": volume/effect textures excluded from UV0 albedo; effect adapter still required");
      if (result.texture.empty()) result.lighting = false;
    }
    if (input.diffuse) surface.diffuse = *input.diffuse;
    if (input.specular) surface.specular = *input.specular;
    if (input.shininess) {
      surface.shininess = *input.shininess;
      surface.roughness = rendering::roughnessFromShininess(surface.shininess);
    }
    if (input.lightLimit) surface.lightLimit = *input.lightLimit;
    if (!input.oncePerLight && input.fixedLightCount > 0)
      surface.lightLimit = input.fixedLightCount;
    surface.doubleSided = result.doubleSided;
    if (input.alphaCutoff) {
      surface.surface = rendering::Surface::Cutout;
      surface.alphaCutoff = *input.alphaCutoff;
    }
    if (result.transparent && !input.alphaCutoff) surface.surface = rendering::Surface::Transparent;
    surface.lighting = result.lighting;
    visiting.erase(found->first);
    return result;
  }
};

void LegacyMaterialCatalog::scan(const fs::path &root,
                                 const fs::path &preferredRoot,
                                 const fs::path &overlayCore) {
  implementation_ = std::make_shared<Impl>();
  std::error_code error;
  if (!fs::exists(root, error)) {
    return;
  }
  std::vector<fs::path> materialFiles;
  fs::recursive_directory_iterator iterator(
      root, fs::directory_options::skip_permission_denied, error);
  const fs::recursive_directory_iterator end;
  while (iterator != end) {
    if (!error && iterator->is_regular_file(error) &&
        lower(iterator->path().extension().string()) == ".material") {
      materialFiles.push_back(iterator->path());
    }
    iterator.increment(error);
    if (error) {
      error.clear();
    }
  }
  std::sort(materialFiles.begin(), materialFiles.end());
  for (const fs::path &path : materialFiles) {
    implementation_->parse(path);
  }
  // Resource profiles select one material-quality directory, but the
  // compatibility catalogue also scans the rest of the content for materials
  // stored beside models/maps. Reparse the selected directory last so a
  // duplicate legacy material name deterministically uses the requested
  // quality instead of filesystem traversal order.
  if (!preferredRoot.empty() && fs::is_directory(preferredRoot, error)) {
    std::vector<fs::path> preferredFiles;
    fs::recursive_directory_iterator preferredIterator(
        preferredRoot, fs::directory_options::skip_permission_denied, error);
    while (preferredIterator != end) {
      if (!error && preferredIterator->is_regular_file(error) &&
          lower(preferredIterator->path().extension().string()) ==
              ".material") {
        preferredFiles.push_back(preferredIterator->path());
      }
      preferredIterator.increment(error);
      if (error) error.clear();
    }
    std::sort(preferredFiles.begin(), preferredFiles.end());
    for (const fs::path &path : preferredFiles) {
      implementation_->parse(path);
    }
  }
  if (!overlayCore.empty() && fs::is_directory(overlayCore)) {
    std::vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(overlayCore))
      if (entry.is_regular_file() && lower(entry.path().extension().string()) == ".material")
        files.push_back(entry.path());
    std::sort(files.begin(),files.end());
    for(const auto &path:files) implementation_->parse(path);
  }
}

std::optional<LegacyMaterialInfo>
LegacyMaterialCatalog::find(const std::string &materialName) const {
  if (!implementation_) {
    return std::nullopt;
  }
  std::unordered_set<std::string> visiting;
  const auto result = implementation_->resolve(materialName, visiting);
  return result;
}

std::size_t LegacyMaterialCatalog::size() const noexcept {
  return implementation_ ? implementation_->source.size() : 0;
}

std::vector<std::string> LegacyMaterialCatalog::names() const {
  std::vector<std::string> result;
  if (!implementation_) return result;
  result.reserve(implementation_->source.size());
  for (const auto &[name, material] : implementation_->source) {
    static_cast<void>(material);
    result.push_back(name);
  }
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace run3::gameplay
