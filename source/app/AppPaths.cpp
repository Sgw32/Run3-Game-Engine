#include <run3/app/AppPaths.hpp>

#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace run3 {
namespace fs = std::filesystem;

namespace {

fs::path absoluteNormal(const fs::path &path, const fs::path &base = {}) {
  const fs::path combined = path.is_absolute() || base.empty() ? path : base / path;
  std::error_code error;
  const fs::path absolute = fs::absolute(combined, error);
  if (error) {
    throw std::runtime_error("Cannot make path absolute: " + path.string());
  }
  return absolute.lexically_normal();
}

fs::path checkedRelative(const fs::path &root, const fs::path &relative) {
  if (relative.empty() || relative.is_absolute()) {
    throw std::invalid_argument("Application path must be a non-empty relative path");
  }
  const fs::path normalized = relative.lexically_normal();
  if (*normalized.begin() == "..") {
    throw std::invalid_argument("Application path may not escape its root");
  }
  return (root / normalized).lexically_normal();
}

std::optional<std::string> environmentValue(const char *name) {
#ifdef _WIN32
  char *raw{};
  std::size_t size{};
  if (_dupenv_s(&raw, &size, name) != 0 || raw == nullptr) {
    return std::nullopt;
  }
  const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
  return std::string(value.get());
#else
  if (const char *value = std::getenv(name)) {
    return std::string(value);
  }
  return std::nullopt;
#endif
}

} // namespace

AppPaths AppPaths::resolve(const fs::path &executable,
                           const std::optional<fs::path> &contentRoot,
                           const std::optional<fs::path> &userRoot) {
  AppPaths result;
  result.executable_ = absoluteNormal(executable);
  result.executableDir_ = result.executable_.parent_path();
  result.contentRoot_ = absoluteNormal(
      contentRoot.value_or(result.executableDir_ / ".." / "share" / "run3" /
                           "content"),
      result.executableDir_);
  result.userRoot_ = absoluteNormal(userRoot.value_or(defaultUserRoot()),
                                    result.executableDir_);
  result.configDir_ = result.userRoot_ / "config";
  result.saveDir_ = result.userRoot_ / "saves";
  result.logDir_ = result.userRoot_ / "logs";
  result.cacheDir_ = result.userRoot_ / "cache";
  return result;
}

fs::path AppPaths::executablePath(const char *argv0) {
#ifdef _WIN32
  std::vector<wchar_t> buffer(32768);
  const DWORD length = GetModuleFileNameW(
      nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (length != 0 && length < buffer.size()) {
    return fs::path(std::wstring_view(buffer.data(), length));
  }
#else
  std::error_code error;
  const fs::path resolved = fs::read_symlink("/proc/self/exe", error);
  if (!error) {
    return resolved;
  }
#endif
  if (argv0 == nullptr || *argv0 == '\0') {
    throw std::runtime_error("Cannot resolve the application executable path");
  }
  return absoluteNormal(argv0);
}

fs::path AppPaths::defaultUserRoot() {
#ifdef _WIN32
  if (const auto localAppData = environmentValue("LOCALAPPDATA")) {
    return fs::path(*localAppData) / "Run3";
  }
#else
  if (const auto stateHome = environmentValue("XDG_STATE_HOME")) {
    return fs::path(*stateHome) / "run3";
  }
  if (const auto homeDirectory = environmentValue("HOME")) {
    return fs::path(*homeDirectory) / ".local" / "state" / "run3";
  }
#endif
  return fs::temp_directory_path() / "run3-user";
}

fs::path AppPaths::contentPath(const fs::path &relative) const {
  return checkedRelative(contentRoot_, relative);
}

fs::path AppPaths::userPath(const fs::path &relative) const {
  return checkedRelative(userRoot_, relative);
}

void AppPaths::createWritableDirectories() const {
  for (const fs::path *directory : {&configDir_, &saveDir_, &logDir_, &cacheDir_}) {
    std::error_code error;
    fs::create_directories(*directory, error);
    if (error) {
      throw std::runtime_error("Cannot create writable Run3 directory: " +
                               directory->string());
    }
  }
}

} // namespace run3
