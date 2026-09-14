#include <run3/content/AssetValidation.hpp>

#include <lua.hpp>
#include <nlohmann/json.hpp>
#include <tinyxml2.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace run3 {
namespace fs = std::filesystem;
using Json = nlohmann::json;

namespace {

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string generic(const fs::path &path) { return path.generic_string(); }

std::vector<std::string> stringArray(const Json &object, const char *key) {
  if (!object.contains(key) || !object.at(key).is_array()) {
    throw std::runtime_error(std::string("Manifest field '") + key +
                             "' must be an array");
  }
  return object.at(key).get<std::vector<std::string>>();
}

struct CaseResult {
  bool exists{};
  bool exact{};
  fs::path actual;
};

CaseResult inspectCase(const fs::path &root, const fs::path &relative) {
  if (relative.is_absolute()) {
    return {fs::exists(relative), false, relative};
  }
  fs::path current = root;
  bool exact = true;
  for (const fs::path &part : relative.lexically_normal()) {
    if (part.empty() || part == ".") {
      continue;
    }
    if (part == "..") {
      return {};
    }
    if (!fs::is_directory(current)) {
      return {};
    }
    const std::string wanted = lower(part.string());
    std::optional<fs::path> foldedMatch;
    std::optional<fs::path> exactMatch;
    for (const auto &entry : fs::directory_iterator(current)) {
      const std::string actualName = entry.path().filename().string();
      if (actualName == part.string()) {
        exactMatch = entry.path();
        break;
      }
      if (!foldedMatch && lower(actualName) == wanted) {
        foldedMatch = entry.path();
      }
    }
    if (exactMatch) {
      current = *exactMatch;
    } else if (foldedMatch) {
      current = *foldedMatch;
      exact = false;
    } else {
      return {};
    }
  }
  return {true, exact, current.lexically_normal()};
}

std::uint16_t read16(std::istream &stream) {
  unsigned char bytes[2]{};
  stream.read(reinterpret_cast<char *>(bytes), 2);
  return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8U));
}

std::uint32_t read32(std::istream &stream) {
  unsigned char bytes[4]{};
  stream.read(reinterpret_cast<char *>(bytes), 4);
  return static_cast<std::uint32_t>(bytes[0] | (bytes[1] << 8U) |
                                    (bytes[2] << 16U) | (bytes[3] << 24U));
}

std::vector<std::string> zipEntries(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Cannot open ZIP archive");
  }
  input.seekg(0, std::ios::end);
  const auto end = input.tellg();
  const std::streamoff window = std::min<std::streamoff>(end, 65557);
  input.seekg(end - window);
  std::vector<unsigned char> tail(static_cast<std::size_t>(window));
  input.read(reinterpret_cast<char *>(tail.data()),
             static_cast<std::streamsize>(tail.size()));
  std::size_t eocd = std::string::npos;
  for (std::size_t index = tail.size() >= 22 ? tail.size() - 22 : 0;
       index + 3 < tail.size(); --index) {
    if (tail[index] == 0x50 && tail[index + 1] == 0x4b &&
        tail[index + 2] == 0x05 && tail[index + 3] == 0x06) {
      eocd = index;
      break;
    }
    if (index == 0) {
      break;
    }
  }
  if (eocd == std::string::npos) {
    throw std::runtime_error("ZIP central directory was not found");
  }
  auto tail16 = [&tail](std::size_t offset) {
    return static_cast<std::uint16_t>(tail[offset] | (tail[offset + 1] << 8U));
  };
  auto tail32 = [&tail](std::size_t offset) {
    return static_cast<std::uint32_t>(tail[offset] | (tail[offset + 1] << 8U) |
                                      (tail[offset + 2] << 16U) |
                                      (tail[offset + 3] << 24U));
  };
  const std::uint16_t count = tail16(eocd + 10);
  const std::uint32_t offset = tail32(eocd + 16);
  input.clear();
  input.seekg(offset);
  std::vector<std::string> entries;
  for (std::uint16_t index = 0; index < count; ++index) {
    if (read32(input) != 0x02014b50U) {
      throw std::runtime_error("Invalid ZIP central-directory entry");
    }
    input.seekg(24, std::ios::cur);
    const std::uint16_t nameLength = read16(input);
    const std::uint16_t extraLength = read16(input);
    const std::uint16_t commentLength = read16(input);
    input.seekg(12, std::ios::cur);
    std::string name(nameLength, '\0');
    input.read(name.data(), nameLength);
    input.seekg(static_cast<std::streamoff>(extraLength + commentLength),
                std::ios::cur);
    std::replace(name.begin(), name.end(), '\\', '/');
    if (!name.empty() && name.back() != '/') {
      entries.push_back(std::move(name));
    }
  }
  return entries;
}

bool serializerHeaderLooksReadable(const AssetFile &file) {
  std::ifstream input(file.absolutePath, std::ios::binary);
  std::string header(160, '\0');
  input.read(header.data(), static_cast<std::streamsize>(header.size()));
  header.resize(static_cast<std::size_t>(input.gcount()));
  if (header.size() < 4) {
    return false;
  }
  const auto first = static_cast<unsigned char>(header[0]);
  const auto second = static_cast<unsigned char>(header[1]);
  const bool serializerChunk =
      (first == 0x00 && second == 0x10) || (first == 0x10 && second == 0x00);
  const std::string marker = file.extension == ".mesh" ? "[MeshSerializer_"
                                                       : "[Serializer_";
  return serializerChunk && header.find(marker) != std::string::npos;
}

std::vector<std::string> directLogicalNames(const AssetResourceLocation &location) {
  std::vector<std::string> names;
  if (lower(location.type) == "filesystem") {
    for (const auto &entry : fs::directory_iterator(location.absolutePath)) {
      if (entry.is_regular_file()) {
        names.push_back(entry.path().filename().generic_string());
      }
    }
  } else if (lower(location.type) == "zip") {
    names = zipEntries(location.absolutePath);
  }
  return names;
}

void inspectXmlReferences(tinyxml2::XMLElement *element,
                          const std::function<void(std::string)> &accept) {
  for (auto *attribute = element->FirstAttribute(); attribute != nullptr;
       attribute = attribute->Next()) {
    accept(attribute->Value());
  }
  for (auto *child = element->FirstChildElement(); child != nullptr;
       child = child->NextSiblingElement()) {
    inspectXmlReferences(child, accept);
  }
}

} // namespace

void AssetReport::addIssue(AssetIssueSeverity severity, std::string category,
                           fs::path path, std::string message) {
  issues.push_back(
      AssetIssue{severity, std::move(category), std::move(path), std::move(message)});
}

std::size_t AssetReport::errorCount() const noexcept {
  return static_cast<std::size_t>(std::count_if(
      issues.begin(), issues.end(), [](const AssetIssue &issue) {
        return issue.severity == AssetIssueSeverity::Error;
      }));
}

std::size_t AssetReport::warningCount() const noexcept {
  return issues.size() - errorCount();
}

std::string AssetReport::conciseReport() const {
  std::map<std::string, std::size_t> categories;
  for (const AssetIssue &issue : issues) {
    ++categories[issue.category];
  }
  std::ostringstream output;
  output << "Run3 asset check: " << (passed() ? "PASS" : "FAIL") << "\n"
         << "  content: " << contentId << " " << contentVersion << "\n"
         << "  files: " << files.size() << "\n"
         << "  errors: " << errorCount() << ", warnings: " << warningCount()
         << "\n";
  for (const auto &[category, count] : categories) {
    output << "  " << category << ": " << count << "\n";
  }
  return output.str();
}

void AssetReport::writeJson(const fs::path &path) const {
  Json root;
  root["schema_version"] = schemaVersion;
  root["manifest_version"] = manifestVersion;
  root["tool_version"] = toolVersion;
  root["content_id"] = contentId;
  root["content_version"] = contentVersion;
  root["compatibility_runtime"] = {{"lua", luaVersion},
                                     {"ogre", ogreVersion},
                                     {"resource_profile", resourceProfile}};
  root["content_root"] = generic(contentRoot);
  root["manifest"] = generic(manifestPath);
  root["passed"] = passed();
  root["summary"] = {{"files", files.size()},
                     {"errors", errorCount()},
                     {"warnings", warningCount()}};
  root["checks"] = checks;
  root["files"] = Json::array();
  for (const AssetFile &file : files) {
    root["files"].push_back({{"path", file.relativePath},
                             {"extension", file.extension},
                             {"size", file.size},
                             {"sha256", file.sha256}});
  }
  root["resource_locations"] = Json::array();
  for (const AssetResourceLocation &location : resourceLocations) {
    root["resource_locations"].push_back(
        {{"group", location.group},
         {"type", location.type},
         {"path", generic(location.absolutePath)},
         {"configured_path", location.configuredPath},
         {"source_config", location.sourceConfig},
         {"line", location.line}});
  }
  root["issues"] = Json::array();
  for (const AssetIssue &issue : issues) {
    root["issues"].push_back(
        {{"severity", issue.severity == AssetIssueSeverity::Error ? "error"
                                                                  : "warning"},
         {"category", issue.category},
         {"path", generic(issue.path)},
         {"message", issue.message}});
  }
  fs::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Cannot write asset report: " + path.string());
  }
  output << root.dump(2) << '\n';
}

AssetReport validateContent(const AssetValidationOptions &options) {
  AssetReport report;
  report.contentRoot = fs::absolute(options.contentRoot).lexically_normal();
  report.manifestPath = fs::absolute(options.manifestPath).lexically_normal();
  std::unordered_map<std::string, CaseResult> caseCache;
  auto inspectContentPath = [&](const fs::path &relative) {
    const std::string key = generic(relative.lexically_normal());
    const auto found = caseCache.find(key);
    if (found != caseCache.end()) {
      return found->second;
    }
    const CaseResult result = inspectCase(report.contentRoot, relative);
    caseCache.emplace(key, result);
    return result;
  };

  std::ifstream manifestStream(report.manifestPath);
  if (!manifestStream) {
    throw std::runtime_error("Cannot open content manifest: " +
                             report.manifestPath.string());
  }
  Json manifest;
  manifestStream >> manifest;
  if (manifest.value("schema_version", 0U) != 1U) {
    throw std::runtime_error("Unsupported content manifest schema version");
  }
  report.manifestVersion = manifest.value("manifest_version", 0U);
  if (report.manifestVersion != 1U) {
    throw std::runtime_error("Unsupported content manifest version");
  }
  report.contentId = manifest.at("content_id").get<std::string>();
  report.contentVersion = manifest.at("content_version").get<std::string>();
  const Json &runtime = manifest.at("compatibility_runtime");
  report.luaVersion = runtime.at("lua").get<std::string>();
  report.ogreVersion = runtime.at("ogre").get<std::string>();
  report.resourceProfile = runtime.at("resource_profile").get<std::string>();
  const std::string compiledLua = std::string(LUA_VERSION_MAJOR) + "." +
                                  LUA_VERSION_MINOR + "." + LUA_VERSION_RELEASE;
  if (report.luaVersion != compiledLua) {
    throw std::runtime_error("Manifest requires Lua " + report.luaVersion +
                             ", but run3_asset_check uses Lua " + compiledLua);
  }
  const auto scanRoots = stringArray(manifest, "scan_roots");
  const auto resourceConfigs = stringArray(manifest, "resource_configs");
  if (std::find(resourceConfigs.begin(), resourceConfigs.end(),
                report.resourceProfile) == resourceConfigs.end()) {
    throw std::runtime_error(
        "compatibility_runtime.resource_profile is not in resource_configs");
  }
  std::unordered_set<std::string> xmlExtensions;
  std::unordered_set<std::string> luaExtensions;
  std::unordered_set<std::string> ogreScriptExtensions;
  for (std::string extension : stringArray(manifest, "xml_extensions")) {
    xmlExtensions.insert(lower(std::move(extension)));
  }
  for (std::string extension : stringArray(manifest, "lua_extensions")) {
    luaExtensions.insert(lower(std::move(extension)));
  }
  for (std::string extension :
       stringArray(manifest, "ogre_script_extensions")) {
    ogreScriptExtensions.insert(lower(std::move(extension)));
  }

  std::set<fs::path> uniqueFiles;
  for (const std::string &scanRootValue : scanRoots) {
    const CaseResult scanRoot = inspectContentPath(scanRootValue);
    if (!scanRoot.exists || !fs::is_directory(scanRoot.actual)) {
      report.addIssue(AssetIssueSeverity::Error, "scan-root-missing",
                      scanRootValue, "Manifest scan root is not a directory");
      continue;
    }
    if (!scanRoot.exact) {
      report.addIssue(AssetIssueSeverity::Error, "case-mismatch", scanRootValue,
                      "Scan root casing differs from the filesystem");
    }
    for (const auto &entry : fs::recursive_directory_iterator(
             scanRoot.actual, fs::directory_options::skip_permission_denied)) {
      if (entry.is_regular_file()) {
        uniqueFiles.insert(fs::absolute(entry.path()).lexically_normal());
      }
    }
  }
  for (const fs::path &path : uniqueFiles) {
    try {
      const fs::path relative = fs::relative(path, report.contentRoot);
      report.files.push_back(AssetFile{path, generic(relative),
                                       lower(path.extension().string()),
                                       fs::file_size(path), sha256File(path)});
    } catch (const std::exception &error) {
      report.addIssue(AssetIssueSeverity::Error, "file-read", path,
                      error.what());
    }
  }
  std::sort(report.files.begin(), report.files.end(),
            [](const AssetFile &left, const AssetFile &right) {
              return left.relativePath < right.relativePath;
            });
  report.checks["sha256"] = report.files.size();

  for (const std::string &configValue : resourceConfigs) {
    const CaseResult config = inspectContentPath(configValue);
    if (!config.exists || !fs::is_regular_file(config.actual)) {
      report.addIssue(AssetIssueSeverity::Error, "resource-path-missing",
                      configValue, "Resource configuration does not exist");
      continue;
    }
    if (!config.exact) {
      report.addIssue(AssetIssueSeverity::Error, "case-mismatch", configValue,
                      "Resource configuration casing differs from the filesystem");
    }
    std::ifstream stream(config.actual);
    std::string group = "General";
    std::string line;
    std::size_t lineNumber{};
    while (std::getline(stream, line)) {
      ++lineNumber;
      line = trim(std::move(line));
      if (line.empty() || line.front() == '#' || line.front() == ';') {
        continue;
      }
      if (line.front() == '[' && line.back() == ']') {
        group = trim(line.substr(1, line.size() - 2));
        continue;
      }
      const auto separator = line.find('=');
      if (separator == std::string::npos) {
        report.addIssue(AssetIssueSeverity::Error, "resource-config-syntax",
                        configValue, "Line " + std::to_string(lineNumber) +
                                         " is not type=path");
        continue;
      }
      const std::string type = trim(line.substr(0, separator));
      const std::string configured = trim(line.substr(separator + 1));
      if (lower(type) != "filesystem" && lower(type) != "zip") {
        report.addIssue(AssetIssueSeverity::Warning, "resource-type-unsupported",
                        configValue, "Line " + std::to_string(lineNumber) +
                                         " uses resource type " + type);
        continue;
      }
      const CaseResult resource = inspectContentPath(configured);
      const bool wantedDirectory = lower(type) == "filesystem";
      if (!resource.exists || (wantedDirectory && !fs::is_directory(resource.actual)) ||
          (!wantedDirectory && !fs::is_regular_file(resource.actual))) {
        report.addIssue(AssetIssueSeverity::Error, "resource-path-missing",
                        configured, configValue + ":" +
                                        std::to_string(lineNumber));
        continue;
      }
      if (!resource.exact) {
        report.addIssue(AssetIssueSeverity::Error, "case-mismatch", configured,
                        configValue + ":" + std::to_string(lineNumber));
      }
      report.resourceLocations.push_back(
          AssetResourceLocation{group, type, resource.actual, configured,
                                configValue, lineNumber});
    }
  }
  report.checks["resource_locations"] = report.resourceLocations.size();

  std::map<std::pair<std::string, std::string>,
           std::unordered_map<std::string, std::vector<std::string>>>
      logicalNames;
  for (const AssetResourceLocation &location : report.resourceLocations) {
    try {
      for (const std::string &name : directLogicalNames(location)) {
        logicalNames[{location.sourceConfig, location.group}][lower(name)].push_back(
            location.configuredPath + ":" + name);
      }
    } catch (const std::exception &error) {
      report.addIssue(AssetIssueSeverity::Error, "resource-archive-read",
                      location.absolutePath, error.what());
    }
  }
  for (const auto &[scope, names] : logicalNames) {
    for (const auto &[logical, sources] : names) {
      if (sources.size() > 1) {
        std::ostringstream message;
        message << "Logical resource '" << logical << "' appears in ";
        for (std::size_t index = 0; index < sources.size(); ++index) {
          message << (index == 0 ? "" : ", ") << sources[index];
        }
        report.addIssue(AssetIssueSeverity::Error, "duplicate-logical-resource",
                        scope.first, message.str());
      }
    }
  }

  std::unordered_map<std::string, std::vector<std::string>> lowerFiles;
  std::unordered_map<std::string, std::vector<std::string>> lowerBasenames;
  std::unordered_set<std::string> exactFiles;
  for (const AssetFile &file : report.files) {
    exactFiles.insert(file.relativePath);
    lowerFiles[lower(file.relativePath)].push_back(file.relativePath);
    lowerBasenames[lower(fs::path(file.relativePath).filename().generic_string())]
        .push_back(file.relativePath);
  }
  for (const auto &[folded, paths] : lowerFiles) {
    if (paths.size() > 1) {
      report.addIssue(AssetIssueSeverity::Error, "case-collision", paths.front(),
                      "Paths collide on a case-insensitive filesystem: " +
                          folded);
    }
  }

  const std::unordered_set<std::string> referenceExtensions{
      ".mesh", ".skeleton", ".scene", ".xml", ".lua", ".material",
      ".program", ".compositor", ".particle", ".overlay", ".fontdef",
      ".png", ".jpg", ".jpeg", ".tga", ".dds", ".bmp", ".wav",
      ".mp3", ".ogg", ".cg", ".hlsl", ".glsl", ".vert", ".frag"};
  std::set<std::string> uniqueResourcePrefixes;
  for (const AssetResourceLocation &location : report.resourceLocations) {
    if (lower(location.type) != "filesystem") {
      continue;
    }
    std::error_code error;
    const fs::path relative =
        fs::relative(location.absolutePath, report.contentRoot, error);
    if (!error) {
      uniqueResourcePrefixes.insert(generic(relative.lexically_normal()));
    }
  }
  std::set<std::pair<std::string, std::string>> inspectedReferences;
  auto inspectReference = [&](const AssetFile &source, std::string value) {
    value = trim(std::move(value));
    std::replace(value.begin(), value.end(), '\\', '/');
    const std::string extension = lower(fs::path(value).extension().string());
    if (referenceExtensions.count(extension) == 0 || value.find('$') != std::string::npos ||
        value.find('*') != std::string::npos || !inspectedReferences.emplace(source.relativePath, value).second) {
      return;
    }
    std::vector<std::string> candidates;
    candidates.reserve(uniqueResourcePrefixes.size() + 1);
    candidates.push_back(value);
    for (const std::string &prefix : uniqueResourcePrefixes) {
      candidates.push_back(
          generic((fs::path(prefix) / value).lexically_normal()));
    }
    for (const std::string &candidate : candidates) {
      if (exactFiles.count(candidate) != 0) {
        ++report.checks["references_resolved"];
        return;
      }
    }
    for (const std::string &candidate : candidates) {
      const auto folded = lowerFiles.find(lower(candidate));
      if (folded != lowerFiles.end()) {
        report.addIssue(AssetIssueSeverity::Error, "case-mismatch", source.relativePath,
                        "Reference '" + value + "' resolves only as '" +
                            folded->second.front() + "'");
        return;
      }
    }
    if (fs::path(value).parent_path().empty()) {
      const auto basename = lowerBasenames.find(lower(value));
      if (basename != lowerBasenames.end()) {
        const auto exact = std::find_if(
            basename->second.begin(), basename->second.end(),
            [&value](const std::string &path) {
              return fs::path(path).filename().generic_string() == value;
            });
        if (exact != basename->second.end()) {
          ++report.checks["references_resolved"];
          return;
        }
        report.addIssue(AssetIssueSeverity::Error, "case-mismatch", source.relativePath,
                        "Reference '" + value + "' differs in case from '" +
                            basename->second.front() + "'");
        return;
      }
    }
    report.addIssue(AssetIssueSeverity::Error, "referenced-file-missing",
                    source.relativePath, "Referenced file was not found: " + value);
  };

  lua_State *lua = luaL_newstate();
  if (lua == nullptr) {
    throw std::runtime_error("Could not create the Lua syntax-check state");
  }
  const std::regex quotedReference(
      R"re(["']([^"']+\.[A-Za-z0-9_]+)["'])re");
  const std::regex scriptReference(
      R"re((?:source|texture|mesh|skeleton)\s+([^\s{}]+))re",
      std::regex::icase);
  for (const AssetFile &file : report.files) {
    if (xmlExtensions.count(file.extension) != 0) {
      tinyxml2::XMLDocument document;
      const auto result = document.LoadFile(file.absolutePath.string().c_str());
      ++report.checks["xml_files"];
      if (result != tinyxml2::XML_SUCCESS) {
        report.addIssue(AssetIssueSeverity::Error, "xml-not-well-formed",
                        file.relativePath,
                        document.ErrorStr() == nullptr ? "XML parse failed"
                                                       : document.ErrorStr());
      } else if (auto *root = document.RootElement()) {
        inspectXmlReferences(root, [&](std::string value) {
          inspectReference(file, std::move(value));
        });
      }
    }
    if (luaExtensions.count(file.extension) != 0) {
      ++report.checks["lua_files"];
      if (luaL_loadfile(lua, file.absolutePath.string().c_str()) != LUA_OK) {
        const char *message = lua_tostring(lua, -1);
        report.addIssue(AssetIssueSeverity::Error, "lua-parse",
                        file.relativePath,
                        message == nullptr ? "Lua syntax parse failed" : message);
        lua_pop(lua, 1);
      } else {
        lua_pop(lua, 1);
      }
    }
    if (file.extension == ".mesh" || file.extension == ".skeleton") {
      ++report.checks[file.extension == ".mesh" ? "mesh_headers"
                                                 : "skeleton_headers"];
      if (!serializerHeaderLooksReadable(file)) {
        report.addIssue(AssetIssueSeverity::Error,
                        file.extension == ".mesh" ? "mesh-readability"
                                                   : "skeleton-readability",
                        file.relativePath,
                        "Ogre serializer header/version marker is invalid");
      }
    }
    // XML references were already visited through tinyxml2 attributes above.
    // Avoid a second regex pass over every scene; large legacy maps otherwise
    // pay twice for the same references.
    if (luaExtensions.count(file.extension) != 0 ||
        ogreScriptExtensions.count(file.extension) != 0) {
      std::ifstream stream(file.absolutePath, std::ios::binary);
      const std::string text((std::istreambuf_iterator<char>(stream)),
                             std::istreambuf_iterator<char>());
      for (auto match = std::sregex_iterator(text.begin(), text.end(), quotedReference);
           match != std::sregex_iterator(); ++match) {
        inspectReference(file, (*match)[1].str());
      }
      if (ogreScriptExtensions.count(file.extension) != 0) {
        ++report.checks["ogre_script_files"];
        for (auto match = std::sregex_iterator(text.begin(), text.end(), scriptReference);
             match != std::sregex_iterator(); ++match) {
          inspectReference(file, (*match)[1].str());
        }
      }
    }
  }
  lua_close(lua);
  return report;
}

} // namespace run3
