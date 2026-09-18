#pragma once

#include <run3/app/AppPaths.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace run3::content {

struct SourceLocation {
  std::filesystem::path file;
  std::size_t line{1};
  std::size_t column{1};
};

struct AuthoredElement {
  std::string tag;
  std::vector<std::pair<std::string, std::string>> attributes;
  std::string text;
  SourceLocation source;
  std::size_t order{};
  std::vector<AuthoredElement> children;

  [[nodiscard]] const std::string *attribute(std::string_view name) const;
  [[nodiscard]] const AuthoredElement *firstChild(std::string_view tagName) const;
};

enum class DefinitionIssueKind {
  UnknownElement,
  DuplicateConfigKey,
  DuplicateName,
  MissingReference,
  AmbiguousReference
};

struct DefinitionIssue {
  DefinitionIssueKind kind{DefinitionIssueKind::UnknownElement};
  SourceLocation source;
  std::string context;
  std::string message;
};

struct ConfigEntry {
  std::string key;
  std::string value;
  SourceLocation source;
};

enum class SequenceOrigin { Integrated, External };

struct SequenceDefinition {
  SequenceOrigin origin{SequenceOrigin::External};
  SourceLocation source;
  std::vector<AuthoredElement> declarations;
  std::vector<AuthoredElement> events;
};

struct MapDefinition {
  std::string mapName;
  std::string quality;
  std::filesystem::path mapDirectory;
  std::filesystem::path configFile;
  std::filesystem::path sceneFile;
  std::optional<std::filesystem::path> externalSequenceFile;
  std::vector<ConfigEntry> config;
  AuthoredElement scene;
  std::vector<SequenceDefinition> sequences;
  std::vector<DefinitionIssue> issues;
};

struct MapDefinitionOptions {
  bool rejectUnknownElements{};
};

class MapDefinitionError final : public std::runtime_error {
public:
  MapDefinitionError(SourceLocation source, std::string cause);

  [[nodiscard]] const SourceLocation &source() const noexcept { return source_; }
  [[nodiscard]] const std::string &cause() const noexcept { return cause_; }

private:
  SourceLocation source_;
  std::string cause_;
};

[[nodiscard]] MapDefinition loadMapDefinition(
    const AppPaths &paths, std::string mapName, std::string quality = "low",
    MapDefinitionOptions options = {});

[[nodiscard]] std::vector<const AuthoredElement *>
sequenceDeclarations(const MapDefinition &definition);
[[nodiscard]] std::vector<const AuthoredElement *>
sequenceEvents(const MapDefinition &definition);

} // namespace run3::content
