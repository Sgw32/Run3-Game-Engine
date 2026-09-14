#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace run3 {

enum class AssetIssueSeverity { Warning, Error };

struct AssetIssue {
  AssetIssueSeverity severity{AssetIssueSeverity::Error};
  std::string category;
  std::filesystem::path path;
  std::string message;
};

struct AssetFile {
  std::filesystem::path absolutePath;
  std::string relativePath;
  std::string extension;
  std::uintmax_t size{};
  std::string sha256;
};

struct AssetResourceLocation {
  std::string group;
  std::string type;
  std::filesystem::path absolutePath;
  std::string configuredPath;
  std::string sourceConfig;
  std::size_t line{};
};

struct AssetReport {
  unsigned schemaVersion{1};
  unsigned manifestVersion{};
  std::string toolVersion{"1.0"};
  std::string contentId;
  std::string contentVersion;
  std::string luaVersion;
  std::string ogreVersion;
  std::string resourceProfile;
  std::filesystem::path contentRoot;
  std::filesystem::path manifestPath;
  std::vector<AssetFile> files;
  std::vector<AssetResourceLocation> resourceLocations;
  std::vector<AssetIssue> issues;
  std::map<std::string, std::size_t> checks;

  void addIssue(AssetIssueSeverity severity, std::string category,
                std::filesystem::path path, std::string message);
  std::size_t errorCount() const noexcept;
  std::size_t warningCount() const noexcept;
  bool passed() const noexcept { return errorCount() == 0; }
  std::string conciseReport() const;
  void writeJson(const std::filesystem::path &path) const;
};

struct AssetValidationOptions {
  std::filesystem::path contentRoot;
  std::filesystem::path manifestPath;
};

AssetReport validateContent(const AssetValidationOptions &options);
std::string sha256File(const std::filesystem::path &path);

} // namespace run3
