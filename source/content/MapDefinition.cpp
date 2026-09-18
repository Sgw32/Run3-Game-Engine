#include <run3/content/MapDefinition.hpp>

#include <run3/content/XmlParser.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace run3::content {
namespace fs = std::filesystem;

namespace {

std::string trim(std::string value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

bool equalFolded(const std::string &left, const std::string &right) {
  return lower(left) == lower(right);
}

fs::path resolveExact(const fs::path &root, const fs::path &relative,
                      const SourceLocation &source) {
  if (relative.empty() || relative.is_absolute()) {
    throw MapDefinitionError(source,
                             "content reference must be a non-empty relative path");
  }
  const fs::path normalized = relative.lexically_normal();
  if (normalized.empty() || *normalized.begin() == "..") {
    throw MapDefinitionError(source, "content reference escapes its map root: " +
                                         relative.generic_string());
  }

  fs::path current = root;
  for (const fs::path &component : normalized) {
    const std::string wanted = component.string();
    bool exact{};
    std::optional<std::string> foldedMatch;
    std::error_code error;
    for (const fs::directory_entry &entry : fs::directory_iterator(current, error)) {
      const std::string actual = entry.path().filename().string();
      if (actual == wanted) {
        current /= entry.path().filename();
        exact = true;
        break;
      }
      if (equalFolded(actual, wanted)) {
        foldedMatch = actual;
      }
    }
    if (error) {
      throw MapDefinitionError(source, "cannot inspect content path '" +
                                           current.generic_string() + "': " +
                                           error.message());
    }
    if (!exact) {
      if (foldedMatch) {
        throw MapDefinitionError(
            source, "case mismatch: requested '" + wanted + "', on disk '" +
                        *foldedMatch + "' below " + current.generic_string());
      }
      throw MapDefinitionError(source, "missing content path: " +
                                           (current / component).generic_string());
    }
  }
  return current.lexically_normal();
}

AuthoredElement copyElement(const XmlNode &node, const fs::path &source,
                            std::size_t &order) {
  AuthoredElement result;
  result.tag = node.name;
  result.attributes = node.attributes;
  result.text = node.text;
  result.source = {source, node.line, 1};
  result.order = order++;
  result.children.reserve(node.children.size());
  for (const XmlNode &child : node.children) {
    result.children.push_back(copyElement(child, source, order));
  }
  return result;
}

const std::set<std::string> &knownSceneTags() {
  static const std::set<std::string> tags{
      "scene",          "integratedSequence", "sequence",
      "adents",        "events",             "nodes",
      "node",           "position",           "rotation",
      "scale",          "lookTarget",         "trackTarget",
      "entity",         "subentity",          "phys",
      "subphys",        "physbox",            "physcyl",
      "pblock",         "blockbox",           "nocollide",
      "subnocollide",   "breakable",          "ragdoll",
      "tree",           "mirror",             "light",
      "colourDiffuse",  "colourSpecular",     "lightRange",
      "lightAttenuation", "dynamic",          "camera",
      "clipping",       "particleSystem",     "fire",
      "billboardSet",   "plane",              "aiNodes",
      "npcnode",        "externals",          "environment",
      "fog",            "skyBox",             "newtonWorld",
      "player",         "hud",                "water",
      "skyx",           "skyDome",            "skyPlane",
      "fade",           "colourAmbient",      "farClip",
      "sounds",         "sound",              "ambient",
      "music",          "terrain",            "portals",
      "portal",         "zone",               "userDataReference",
      "octree",         "normal",             "sunColor",
      "sunPos",         "frames",             "frame",
      "trigger",        "cutscene",            "seqscript",
      "lua",            "run",                 "door",
      "changelevel",    "hurt",                "entc"};
  return tags;
}

const std::set<std::string> &knownSequenceTags() {
  static const std::set<std::string> tags{
      "sequence",   "adents",      "events",       "trigger",
      "pickup",     "computer",    "event",        "flare",
      "timer",      "darkzone",    "ladder",       "fire",
      "npc",        "npcgroup",    "button",       "cutscene",
      "seqscript",  "train",       "pendulum",     "fuzzy",
      "door",       "rot",         "lua",          "onexit",
      "position",   "rotation",    "rotate",       "scale",
      "physPosit",  "physSize",    "axis",         "angle",
      "sounds",     "farFind",     "light",        "lighton",
      "lightoff",   "frame",       "frames",       "run",
      "entity",     "keyPoint",    "object",       "nocollide",
      "subnocollide", "psys",      "changelevel",  "hurt",
      "entc",       "player",      "blockboxes"};
  return tags;
}

void collectUnknown(const AuthoredElement &element,
                    const std::set<std::string> &known,
                    std::vector<DefinitionIssue> &issues) {
  if (known.count(element.tag) == 0) {
    issues.push_back({DefinitionIssueKind::UnknownElement, element.source,
                      element.tag,
                      "preserved unknown element <" + element.tag + ">"});
  }
  if (element.tag == "integratedSequence") {
    return;
  }
  for (const AuthoredElement &child : element.children) {
    collectUnknown(child, known, issues);
  }
}

SequenceDefinition makeSequence(const AuthoredElement &root,
                                const SequenceOrigin origin) {
  SequenceDefinition result;
  result.origin = origin;
  result.source = root.source;
  const AuthoredElement *container = &root;
  if (root.tag == "integratedSequence") {
    if (const AuthoredElement *sequence = root.firstChild("sequence")) {
      container = sequence;
    }
  }
  if (const AuthoredElement *adents = container->firstChild("adents")) {
    result.declarations = adents->children;
  }
  if (const AuthoredElement *events = container->firstChild("events")) {
    result.events = events->children;
  }
  return result;
}

void findIntegrated(const AuthoredElement &element,
                    std::vector<const AuthoredElement *> &result) {
  if (element.tag == "integratedSequence") {
    result.push_back(&element);
    return;
  }
  for (const AuthoredElement &child : element.children) {
    findIntegrated(child, result);
  }
}

std::string errorMessage(const SourceLocation &source,
                         const std::string &cause) {
  return source.file.generic_string() + ":" + std::to_string(source.line) +
         ":" + std::to_string(source.column) + " [map-definition]: " + cause;
}

} // namespace

const std::string *AuthoredElement::attribute(const std::string_view name) const {
  const auto found = std::find_if(
      attributes.begin(), attributes.end(), [name](const auto &entry) {
        return entry.first == name;
      });
  return found == attributes.end() ? nullptr : &found->second;
}

const AuthoredElement *
AuthoredElement::firstChild(const std::string_view tagName) const {
  const auto found = std::find_if(children.begin(), children.end(),
                                  [tagName](const AuthoredElement &child) {
                                    return child.tag == tagName;
                                  });
  return found == children.end() ? nullptr : &*found;
}

MapDefinitionError::MapDefinitionError(SourceLocation source, std::string cause)
    : std::runtime_error(errorMessage(source, cause)), source_(std::move(source)),
      cause_(std::move(cause)) {}

MapDefinition loadMapDefinition(const AppPaths &paths, std::string mapName,
                                std::string quality,
                                const MapDefinitionOptions options) {
  mapName = lower(trim(std::move(mapName)));
  quality = lower(trim(std::move(quality)));
  if (mapName == "tlwhome2") {
    mapName = "tlwhome02";
  }
  if (mapName.empty() || quality.empty() ||
      mapName.find_first_of("/\\") != std::string::npos ||
      quality.find_first_of("/\\") != std::string::npos) {
    throw MapDefinitionError({paths.contentRoot(), 1, 1},
                             "map and quality must be simple non-empty names");
  }

  MapDefinition result;
  result.mapName = mapName;
  result.quality = quality;
  result.mapDirectory = paths.contentPath(fs::path("run3") / "maps" / quality /
                                          mapName);
  result.configFile = resolveExact(result.mapDirectory, "scene.cfg",
                                   {result.mapDirectory, 1, 1});

  std::ifstream config(result.configFile, std::ios::binary);
  if (!config) {
    throw MapDefinitionError({result.configFile, 1, 1},
                             "cannot open map configuration");
  }
  std::unordered_map<std::string, std::size_t> firstKeys;
  std::string line;
  std::size_t lineNumber{};
  while (std::getline(config, line)) {
    ++lineNumber;
    line = trim(std::move(line));
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      throw MapDefinitionError({result.configFile, lineNumber, 1},
                               "expected key=value entry");
    }
    ConfigEntry entry{trim(line.substr(0, equals)),
                      trim(line.substr(equals + 1)),
                      {result.configFile, lineNumber, equals + 2}};
    if (entry.key.empty() || entry.value.empty()) {
      throw MapDefinitionError(entry.source, "configuration key/value is empty");
    }
    const auto inserted = firstKeys.emplace(entry.key, result.config.size());
    if (!inserted.second) {
      result.issues.push_back(
          {DefinitionIssueKind::DuplicateConfigKey, entry.source, entry.key,
           "duplicate configuration key; legacy first declaration is used"});
    }
    result.config.push_back(std::move(entry));
  }

  const auto configValue = [&result](const std::string_view key)
      -> const ConfigEntry * {
    const auto found = std::find_if(
        result.config.begin(), result.config.end(), [key](const ConfigEntry &entry) {
          return entry.key == key;
        });
    return found == result.config.end() ? nullptr : &*found;
  };
  const ConfigEntry *sceneEntry = configValue("Scene");
  if (sceneEntry == nullptr) {
    throw MapDefinitionError({result.configFile, 1, 1},
                             "scene.cfg has no Scene entry");
  }
  result.sceneFile =
      resolveExact(result.mapDirectory, sceneEntry->value, sceneEntry->source);
  const XmlDocument sceneDocument =
      parseXmlFile(result.sceneFile, XmlSchema::scene);
  std::size_t order{};
  result.scene = copyElement(sceneDocument.root, result.sceneFile, order);
  collectUnknown(result.scene, knownSceneTags(), result.issues);

  std::vector<const AuthoredElement *> integrated;
  findIntegrated(result.scene, integrated);
  for (const AuthoredElement *sequence : integrated) {
    result.sequences.push_back(
        makeSequence(*sequence, SequenceOrigin::Integrated));
    collectUnknown(*sequence, knownSequenceTags(), result.issues);
  }

  if (const ConfigEntry *sequenceEntry = configValue("Sequence")) {
    result.externalSequenceFile = resolveExact(
        result.mapDirectory, sequenceEntry->value, sequenceEntry->source);
    const XmlDocument sequenceDocument =
        parseXmlFile(*result.externalSequenceFile, XmlSchema::sequence);
    AuthoredElement external =
        copyElement(sequenceDocument.root, *result.externalSequenceFile, order);
    result.sequences.push_back(makeSequence(external, SequenceOrigin::External));
    collectUnknown(external, knownSequenceTags(), result.issues);
  }

  if (options.rejectUnknownElements) {
    const auto unknown = std::find_if(
        result.issues.begin(), result.issues.end(), [](const DefinitionIssue &issue) {
          return issue.kind == DefinitionIssueKind::UnknownElement;
        });
    if (unknown != result.issues.end()) {
      throw MapDefinitionError(unknown->source, unknown->message);
    }
  }
  return result;
}

std::vector<const AuthoredElement *>
sequenceDeclarations(const MapDefinition &definition) {
  std::vector<const AuthoredElement *> result;
  for (const SequenceDefinition &sequence : definition.sequences) {
    for (const AuthoredElement &declaration : sequence.declarations) {
      result.push_back(&declaration);
    }
  }
  return result;
}

std::vector<const AuthoredElement *>
sequenceEvents(const MapDefinition &definition) {
  std::vector<const AuthoredElement *> result;
  for (const SequenceDefinition &sequence : definition.sequences) {
    for (const AuthoredElement &event : sequence.events) {
      result.push_back(&event);
    }
  }
  return result;
}

} // namespace run3::content
