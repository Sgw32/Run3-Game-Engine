#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace run3 {

using ConfigValues = std::unordered_map<std::string, std::string>;

class Configuration {
public:
  static ConfigValues readFile(const std::filesystem::path &path);
  static Configuration merge(const ConfigValues &contentDefaults,
                             const ConfigValues &userValues,
                             const ConfigValues &commandLineValues);

  std::optional<std::string> find(std::string_view key) const;
  std::string valueOr(std::string_view key, std::string fallback) const;
  std::uint64_t unsignedOr(std::string_view key, std::uint64_t fallback) const;

private:
  ConfigValues values_;
};

struct CommandLine {
  ConfigValues values;
  std::optional<std::filesystem::path> contentRoot;
  std::optional<std::filesystem::path> userRoot;
  bool help{};
  bool validateContent{};
};

CommandLine parseCommandLine(const std::vector<std::string> &arguments,
                             const std::filesystem::path &executableDir);

} // namespace run3
