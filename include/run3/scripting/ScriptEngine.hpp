#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace run3::scripting {

struct BindingSpec {
  std::string group;
  std::string name;
  std::string signature;
  std::string legacyCallback;
  std::string source;
  std::size_t line{};
};

struct ScriptCall {
  std::string group;
  std::string name;
  std::vector<std::string> arguments;
};

using ScriptValue = std::variant<std::monostate, bool, double, std::string>;
using ScriptDispatch = std::function<ScriptValue(const ScriptCall &)>;

struct ScriptEngineConfig {
  std::filesystem::path contentRoot;
  std::filesystem::path userRoot;
  std::size_t instructionBudget{1'000'000};
};

struct ScriptCheckResult {
  std::filesystem::path path;
  std::vector<std::string> compatibilityShims;
};

class ScriptError final : public std::runtime_error {
public:
  ScriptError(std::filesystem::path path, std::string cause);

  [[nodiscard]] const std::filesystem::path &path() const noexcept {
    return path_;
  }
  [[nodiscard]] const std::string &cause() const noexcept { return cause_; }

private:
  std::filesystem::path path_;
  std::string cause_;
};

// Stable inventory captured from the legacy lua_register calls. The runtime
// registers these in named groups, while gameplay can inject typed dispatch.
[[nodiscard]] const std::vector<BindingSpec> &legacyBindingCatalog();
[[nodiscard]] std::string exportedApiSnapshot();
[[nodiscard]] std::string luaRuntimeVersion();

class ScriptEngine final {
public:
  explicit ScriptEngine(ScriptEngineConfig config,
                        ScriptDispatch dispatch = {});
  ~ScriptEngine();

  ScriptEngine(const ScriptEngine &) = delete;
  ScriptEngine &operator=(const ScriptEngine &) = delete;
  ScriptEngine(ScriptEngine &&) noexcept;
  ScriptEngine &operator=(ScriptEngine &&) noexcept;

  [[nodiscard]] ScriptCheckResult
  checkFile(const std::filesystem::path &path);
  void executeFile(const std::filesystem::path &path);
  void executeText(std::string source, std::string virtualName);

  [[nodiscard]] const std::vector<ScriptCall> &calls() const noexcept;
  void clearCalls() noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace run3::scripting
