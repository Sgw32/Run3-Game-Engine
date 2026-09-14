#pragma once

#include <filesystem>
#include <optional>

namespace run3 {

class AppPaths {
public:
  static AppPaths resolve(
      const std::filesystem::path &executable,
      const std::optional<std::filesystem::path> &contentRoot = std::nullopt,
      const std::optional<std::filesystem::path> &userRoot = std::nullopt);

  static std::filesystem::path executablePath(const char *argv0);
  static std::filesystem::path defaultUserRoot();

  const std::filesystem::path &executable() const noexcept { return executable_; }
  const std::filesystem::path &executableDir() const noexcept {
    return executableDir_;
  }
  const std::filesystem::path &contentRoot() const noexcept { return contentRoot_; }
  const std::filesystem::path &userRoot() const noexcept { return userRoot_; }
  const std::filesystem::path &configDir() const noexcept { return configDir_; }
  const std::filesystem::path &saveDir() const noexcept { return saveDir_; }
  const std::filesystem::path &logDir() const noexcept { return logDir_; }
  const std::filesystem::path &cacheDir() const noexcept { return cacheDir_; }

  std::filesystem::path contentPath(
      const std::filesystem::path &relative) const;
  std::filesystem::path userPath(const std::filesystem::path &relative) const;
  void createWritableDirectories() const;

private:
  std::filesystem::path executable_;
  std::filesystem::path executableDir_;
  std::filesystem::path contentRoot_;
  std::filesystem::path userRoot_;
  std::filesystem::path configDir_;
  std::filesystem::path saveDir_;
  std::filesystem::path logDir_;
  std::filesystem::path cacheDir_;
};

} // namespace run3
