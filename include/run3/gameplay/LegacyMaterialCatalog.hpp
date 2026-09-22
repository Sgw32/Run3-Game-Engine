#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace run3::gameplay {

struct LegacyMaterialInfo {
  std::string texture;
  bool transparent{};
  bool doubleSided{};
};

class LegacyMaterialCatalog final {
public:
  void scan(const std::filesystem::path &root,
            const std::filesystem::path &preferredRoot = {});
  [[nodiscard]] std::optional<LegacyMaterialInfo>
  find(const std::string &materialName) const;
  [[nodiscard]] std::size_t size() const noexcept;

private:
  struct Impl;
  std::shared_ptr<Impl> implementation_;
};

} // namespace run3::gameplay
