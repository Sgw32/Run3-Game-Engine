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
  bool transparent{};
  bool doubleSided{};
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

      std::istringstream tokens(cleaned);
      std::string keyword;
      tokens >> keyword;
      if (keyword == "set_texture_alias") {
        std::string alias;
        std::string texture;
        tokens >> alias >> texture;
        if (alias == "MainTexture" && !texture.empty()) {
          active->aliasTexture = unquote(std::move(texture));
        }
      } else if (keyword == "texture" && active->directTexture.empty()) {
        tokens >> active->directTexture;
        active->directTexture = unquote(std::move(active->directTexture));
        if (!active->directTexture.empty() && active->directTexture.front() == '$') {
          active->directTexture.clear();
        }
      } else if (keyword == "scene_blend") {
        std::string mode;
        tokens >> mode;
        // Ordinary opaque Run3 materials use an additive second pass for
        // per-light accumulation. That does not make the whole material
        // transparent and must not disable compatibility-material depth writes.
        active->transparent = active->transparent || mode == "alpha_blend";
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
    result.transparent = found->second.transparent;
    result.doubleSided = found->second.doubleSided;
    if (!found->second.parent.empty()) {
      if (const auto parent = resolve(found->second.parent, visiting)) {
        if (result.texture.empty()) {
          result.texture = parent->texture;
        }
        result.transparent = result.transparent || parent->transparent;
        result.doubleSided = result.doubleSided || parent->doubleSided;
      }
    }
    visiting.erase(found->first);
    return result;
  }
};

void LegacyMaterialCatalog::scan(const fs::path &root,
                                 const fs::path &preferredRoot) {
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
}

std::optional<LegacyMaterialInfo>
LegacyMaterialCatalog::find(const std::string &materialName) const {
  if (!implementation_) {
    return std::nullopt;
  }
  std::unordered_set<std::string> visiting;
  const auto result = implementation_->resolve(materialName, visiting);
  return result && !result->texture.empty() ? result : std::nullopt;
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
