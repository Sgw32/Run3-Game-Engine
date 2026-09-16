#include <run3/content/XmlParser.hpp>

#include <tinyxml2.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>

namespace run3::content {
namespace fs = std::filesystem;

namespace {

std::string expectedRoot(const XmlSchema schema) {
  switch (schema) {
  case XmlSchema::scene: return "scene";
  case XmlSchema::sequence: return "sequence";
  case XmlSchema::save: return "savefile";
  case XmlSchema::facialAnimation: return "facialanimation";
  case XmlSchema::configAdjacent: return {};
  }
  return {};
}

std::string errorMessage(const fs::path &source, const XmlSchema schema,
                         const std::size_t line, const std::size_t column,
                         const std::string &cause) {
  return source.generic_string() + ":" + std::to_string(line) + ":" +
         std::to_string(column) + " [" +
         std::string(xmlSchemaName(schema)) + "]: " + cause;
}

std::string readText(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw XmlParseError(path, XmlSchema::configAdjacent, 1, 1,
                        "cannot open XML file");
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

std::string normalizeTag(std::string tag, bool &changed) {
  static const std::regex missingSeparator(
      R"SEP((["'])(?=[A-Za-z_:][A-Za-z0-9_.:-]*\s*=))SEP");
  const std::string separated = std::regex_replace(tag, missingSeparator, "$1 ");
  changed = changed || separated != tag;
  tag = separated;

  static const std::regex unquoted(
      R"ATTR((\s[A-Za-z_:][A-Za-z0-9_.:-]*\s*=\s*)([^"' \t\r\n<>]+))ATTR");
  std::string output;
  std::size_t position = 0;
  for (std::sregex_iterator it(tag.begin(), tag.end(), unquoted), end;
       it != end; ++it) {
    const std::smatch &match = *it;
    output.append(tag, position,
                  static_cast<std::size_t>(match.position()) - position);
    output += match[1].str();
    std::string value = match[2].str();
    std::string suffix;
    if (!value.empty() && value.back() == '/') {
      value.pop_back();
      suffix = "/";
    }
    output += '"';
    output += value;
    output += '"';
    output += suffix;
    position = static_cast<std::size_t>(match.position() + match.length());
    changed = true;
  }
  output.append(tag, position, std::string::npos);
  return output;
}

std::string normalizeLegacyAttributes(const std::string_view input,
                                      bool &changed) {
  std::string output;
  output.reserve(input.size());
  std::size_t position = 0;
  while (position < input.size()) {
    const std::size_t opening = input.find('<', position);
    if (opening == std::string_view::npos) {
      output.append(input.substr(position));
      break;
    }
    output.append(input.substr(position, opening - position));
    std::size_t closing = opening + 1;
    char quote{};
    for (; closing < input.size(); ++closing) {
      const char current = input[closing];
      if (quote != 0) {
        if (current == quote) {
          quote = 0;
        }
      } else if (current == '"' || current == '\'') {
        quote = current;
      } else if (current == '>') {
        break;
      }
    }
    if (closing == input.size()) {
      output.append(input.substr(opening));
      break;
    }
    std::string tag(input.substr(opening, closing - opening + 1));
    if (tag.rfind("<!--", 0) != 0 && tag.rfind("<![CDATA[", 0) != 0 &&
        tag.rfind("<?", 0) != 0 && tag.rfind("<!", 0) != 0 &&
        tag.rfind("</", 0) != 0) {
      tag = normalizeTag(std::move(tag), changed);
    }
    output += tag;
    position = closing + 1;
  }
  return output;
}

XmlNode copyElement(const tinyxml2::XMLElement &element) {
  XmlNode node;
  node.name = element.Name() == nullptr ? "" : element.Name();
  node.line = static_cast<std::size_t>(std::max(1, element.GetLineNum()));
  for (const tinyxml2::XMLAttribute *attribute = element.FirstAttribute();
       attribute != nullptr; attribute = attribute->Next()) {
    node.attributes.emplace_back(attribute->Name(), attribute->Value());
  }
  if (const char *text = element.GetText()) {
    node.text = text;
  }
  for (const tinyxml2::XMLElement *child = element.FirstChildElement();
       child != nullptr; child = child->NextSiblingElement()) {
    node.children.push_back(copyElement(*child));
  }
  return node;
}

std::string escaped(std::string_view value) {
  std::string result;
  for (const char character : value) {
    switch (character) {
    case '\\': result += "\\\\"; break;
    case '"': result += "\\\""; break;
    case '\n': result += "\\n"; break;
    case '\r': result += "\\r"; break;
    case '\t': result += "\\t"; break;
    default: result += character; break;
    }
  }
  return result;
}

void appendSnapshot(const XmlNode &node, const std::string &path,
                    std::ostringstream &output) {
  output << "node " << path << " line=" << node.line << '\n';
  std::vector<std::pair<std::string, std::string>> attributes = node.attributes;
  std::sort(attributes.begin(), attributes.end());
  for (const auto &[name, value] : attributes) {
    output << "attr " << path << " @" << name << "=\"" << escaped(value)
           << "\"\n";
  }
  if (!node.text.empty() &&
      node.text.find_first_not_of(" \t\r\n") != std::string::npos) {
    output << "text " << path << "=\"" << escaped(node.text) << "\"\n";
  }
  for (std::size_t index = 0; index < node.children.size(); ++index) {
    const XmlNode &child = node.children[index];
    appendSnapshot(child, path + "/" + child.name + "[" +
                              std::to_string(index) + "]",
                   output);
  }
}

} // namespace

const std::string *XmlNode::attribute(const std::string_view key) const {
  const auto found = std::find_if(
      attributes.begin(), attributes.end(), [key](const auto &attribute) {
        return attribute.first == key;
      });
  return found == attributes.end() ? nullptr : &found->second;
}

const XmlNode *XmlNode::firstChild(const std::string_view childName) const {
  const auto found = std::find_if(children.begin(), children.end(),
                                  [childName](const XmlNode &child) {
                                    return child.name == childName;
                                  });
  return found == children.end() ? nullptr : &*found;
}

XmlParseError::XmlParseError(fs::path source, const XmlSchema schema,
                             const std::size_t line,
                             const std::size_t column, std::string cause)
    : std::runtime_error(errorMessage(source, schema, line, column, cause)),
      source_(std::move(source)), schema_(schema), line_(line), column_(column),
      cause_(std::move(cause)) {}

std::string_view xmlSchemaName(const XmlSchema schema) noexcept {
  switch (schema) {
  case XmlSchema::scene: return "scene";
  case XmlSchema::sequence: return "sequence";
  case XmlSchema::save: return "save";
  case XmlSchema::facialAnimation: return "facial-animation";
  case XmlSchema::configAdjacent: return "config-adjacent";
  }
  return "unknown";
}

XmlDocument parseXml(const std::string_view text, const fs::path &source,
                     const XmlSchema schema) {
  tinyxml2::XMLDocument parsed;
  tinyxml2::XMLError result = parsed.Parse(text.data(), text.size());
  bool normalized = false;
  std::string compatible;
  if (result != tinyxml2::XML_SUCCESS) {
    compatible = normalizeLegacyAttributes(text, normalized);
    if (normalized) {
      parsed.Clear();
      result = parsed.Parse(compatible.data(), compatible.size());
    }
  }
  if (result != tinyxml2::XML_SUCCESS) {
    const std::size_t line =
        static_cast<std::size_t>(std::max(1, parsed.ErrorLineNum()));
    const char *detail = parsed.ErrorStr();
    throw XmlParseError(source, schema, line, 1,
                        detail == nullptr ? "XML parse failed" : detail);
  }
  const tinyxml2::XMLElement *root = parsed.RootElement();
  if (root == nullptr) {
    throw XmlParseError(source, schema, 1, 1, "XML document has no root element");
  }
  const std::string required = expectedRoot(schema);
  if (!required.empty() && required != root->Name()) {
    throw XmlParseError(source, schema,
                        static_cast<std::size_t>(std::max(1, root->GetLineNum())),
                        1, "expected <" + required + "> root, found <" +
                               std::string(root->Name()) + ">");
  }
  return {schema, copyElement(*root), normalized};
}

XmlDocument parseXmlFile(const fs::path &path, const XmlSchema schema) {
  try {
    return parseXml(readText(path), path, schema);
  } catch (const XmlParseError &error) {
    if (error.schema() == schema) {
      throw;
    }
    throw XmlParseError(path, schema, error.line(), error.column(),
                        error.cause());
  }
}

std::string canonicalXmlSnapshot(const XmlDocument &document) {
  std::ostringstream output;
  output << "schema=" << xmlSchemaName(document.schema) << '\n';
  output << "legacy-attribute-normalization="
         << (document.legacyAttributeNormalization ? "true" : "false")
         << '\n';
  appendSnapshot(document.root, "/" + document.root.name, output);
  return output.str();
}

} // namespace run3::content
