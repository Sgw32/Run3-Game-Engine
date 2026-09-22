#include <run3/app/Configuration.hpp>

#include <charconv>
#include <fstream>
#include <stdexcept>

namespace run3 {
namespace fs = std::filesystem;

namespace {

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

fs::path cliPath(const std::string &value, const fs::path &executableDir) {
  const fs::path path(value);
  std::error_code error;
  const fs::path resolved = fs::absolute(
      path.is_absolute() ? path : executableDir / path, error);
  if (error) {
    throw std::invalid_argument("Cannot resolve command-line path: " + value);
  }
  return resolved.lexically_normal();
}

[[noreturn]] void argumentError(const std::string &message) {
  throw std::invalid_argument(
      message +
      "\nUsage: run3_shell [--renderer d3d11|gl3plus] [--frames N]"
      " [--user-dir PATH] [--content-root PATH] [--validate-content]"
      " [--manifest PATH] [--report PATH]"
      " [--map tlwcao|tlwhome02] [--scene-quality low|medium|high]"
      " [--texture-quality low|medium|high]"
      " [--model-quality low|medium|high] [--resource-profile FILE]"
      " [--player-height-cm N] [--fov DEGREES]"
      " [--resolution WIDTHxHEIGHT]"
      " [--fullscreen|--windowed] [--noclip] [--physics-debug]"
      " [--audio-backend auto|miniaudio|null]"
      " [--render-hz 30|60|144]");
}

} // namespace

ConfigValues Configuration::readFile(const fs::path &path) {
  ConfigValues values;
  std::ifstream stream(path);
  if (!stream) {
    return values;
  }
  std::string line;
  std::size_t lineNumber = 0;
  while (std::getline(stream, line)) {
    ++lineNumber;
    line = trim(std::move(line));
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos) {
      throw std::runtime_error(path.string() + ":" +
                               std::to_string(lineNumber) +
                               ": expected key=value");
    }
    std::string key = trim(line.substr(0, separator));
    if (key.empty()) {
      throw std::runtime_error(path.string() + ":" +
                               std::to_string(lineNumber) + ": empty key");
    }
    values[std::move(key)] = trim(line.substr(separator + 1));
  }
  return values;
}

Configuration Configuration::merge(const ConfigValues &contentDefaults,
                                   const ConfigValues &userValues,
                                   const ConfigValues &commandLineValues) {
  Configuration result;
  result.values_ = contentDefaults;
  for (const auto &[key, value] : userValues) {
    result.values_[key] = value;
  }
  for (const auto &[key, value] : commandLineValues) {
    result.values_[key] = value;
  }
  return result;
}

std::optional<std::string> Configuration::find(std::string_view key) const {
  const auto found = values_.find(std::string(key));
  return found == values_.end() ? std::nullopt
                               : std::optional<std::string>(found->second);
}

std::string Configuration::valueOr(std::string_view key,
                                   std::string fallback) const {
  if (const auto found = find(key)) {
    return *found;
  }
  return fallback;
}

std::uint64_t Configuration::unsignedOr(std::string_view key,
                                        std::uint64_t fallback) const {
  const auto found = find(key);
  if (!found) {
    return fallback;
  }
  std::uint64_t result{};
  const char *begin = found->data();
  const char *end = begin + found->size();
  const auto conversion = std::from_chars(begin, end, result);
  if (conversion.ec != std::errc{} || conversion.ptr != end) {
    throw std::runtime_error("Configuration value for '" + std::string(key) +
                             "' is not an unsigned integer: " + *found);
  }
  return result;
}

CommandLine parseCommandLine(const std::vector<std::string> &arguments,
                             const fs::path &executableDir) {
  CommandLine result;
  for (std::size_t index = 1; index < arguments.size(); ++index) {
    const std::string &argument = arguments[index];
    if (argument == "--help" || argument == "-h") {
      result.help = true;
      continue;
    }
    if (argument == "--validate-content") {
      result.validateContent = true;
      continue;
    }
    if (argument == "--fullscreen") {
      result.values["fullscreen"] = "true";
      continue;
    }
    if (argument == "--windowed") {
      result.values["fullscreen"] = "false";
      continue;
    }
    if (argument == "--noclip" || argument == "--physics-debug") {
      result.values[argument.substr(2)] = "true";
      continue;
    }
    if (index + 1 >= arguments.size()) {
      argumentError("Missing value for " + argument);
    }
    const std::string &value = arguments[++index];
    if (argument == "--renderer") {
      result.values["renderer"] = value;
    } else if (argument == "--frames") {
      result.values["frames"] = value;
    } else if (argument == "--user-dir") {
      result.userRoot = cliPath(value, executableDir);
    } else if (argument == "--content-root") {
      result.contentRoot = cliPath(value, executableDir);
    } else if (argument == "--manifest") {
      result.values["manifest"] = cliPath(value, executableDir).string();
    } else if (argument == "--report") {
      result.values["report"] = cliPath(value, executableDir).string();
    } else if (argument == "--map") {
      result.values["map"] = value;
    } else if (argument == "--map-quality" ||
               argument == "--scene-quality") {
      result.values["scene-quality"] = value;
    } else if (argument == "--texture-quality") {
      result.values["texture-quality"] = value;
    } else if (argument == "--model-quality") {
      result.values["model-quality"] = value;
    } else if (argument == "--resource-profile") {
      result.values["resource-profile"] = value;
    } else if (argument == "--player-height-cm") {
      result.values["player-height-cm"] = value;
    } else if (argument == "--fov") {
      result.values["fov"] = value;
    } else if (argument == "--resolution") {
      result.values["resolution"] = value;
    } else if (argument == "--render-hz") {
      result.values["render-hz"] = value;
    } else if (argument == "--audio-backend") {
      result.values["audio-backend"] = value;
    } else {
      argumentError("Unknown argument: " + argument);
    }
  }
  return result;
}

} // namespace run3
