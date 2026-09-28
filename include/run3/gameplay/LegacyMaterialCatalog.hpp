#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <run3/rendering/Lighting.hpp>

namespace run3::gameplay {

struct LegacyMaterialInfo {
  std::string texture;
  bool lighting{true};
  bool transparent{};
  bool doubleSided{};
  rendering::MaterialDescription surface;
};

class LegacyMaterialCatalog final {
public:
  void scan(const std::filesystem::path &root,
            const std::filesystem::path &preferredRoot = {},
            const std::filesystem::path &overlayCore = {});
  [[nodiscard]] std::optional<LegacyMaterialInfo>
  find(const std::string &materialName) const;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::vector<std::string> names() const;

private:
  struct Impl;
  std::shared_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
