#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace run3::content {

enum class XmlSchema {
  scene,
  sequence,
  save,
  facialAnimation,
  configAdjacent,
};

struct XmlNode {
  std::string name;
  std::vector<std::pair<std::string, std::string>> attributes;
  std::string text;
  std::size_t line{};
  std::vector<XmlNode> children;

  [[nodiscard]] const std::string *attribute(std::string_view key) const;
  [[nodiscard]] const XmlNode *firstChild(std::string_view childName) const;
};

struct XmlDocument {
  XmlSchema schema{XmlSchema::configAdjacent};
  XmlNode root;
  bool legacyAttributeNormalization{};
};

class XmlParseError final : public std::runtime_error {
public:
  XmlParseError(std::filesystem::path source, XmlSchema schema,
                std::size_t line, std::size_t column, std::string cause);

  [[nodiscard]] const std::filesystem::path &source() const noexcept {
    return source_;
  }
  [[nodiscard]] XmlSchema schema() const noexcept { return schema_; }
  [[nodiscard]] std::size_t line() const noexcept { return line_; }
  [[nodiscard]] std::size_t column() const noexcept { return column_; }
  [[nodiscard]] const std::string &cause() const noexcept { return cause_; }

private:
  std::filesystem::path source_;
  XmlSchema schema_;
  std::size_t line_{};
  std::size_t column_{};
  std::string cause_;
};

[[nodiscard]] std::string_view xmlSchemaName(XmlSchema schema) noexcept;
[[nodiscard]] XmlDocument parseXml(
    std::string_view text, const std::filesystem::path &source,
    XmlSchema schema);
[[nodiscard]] XmlDocument parseXmlFile(const std::filesystem::path &path,
                                       XmlSchema schema);

// Stable semantic representation used by the pre-migration golden fixtures.
// Attribute ordering in source XML does not affect the snapshot.
[[nodiscard]] std::string canonicalXmlSnapshot(const XmlDocument &document);

} // namespace run3::content
