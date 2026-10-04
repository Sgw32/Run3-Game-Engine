#include "OgreCompositorEffects.hpp"

#include <OgreCamera.h>
#include <OgreCompositor.h>
#include <OgreCompositorManager.h>
#include <OgreDataStream.h>
#include <OgreGpuProgramParams.h>
#include <OgreLogManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreRenderSystem.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreScriptCompiler.h>
#include <OgreShaderGenerator.h>
#include <OgreTechnique.h>
#include <OgreViewport.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace run3::rendering {
namespace {
namespace fs = std::filesystem;
constexpr const char *resourceGroup = "Run3LegacyCompositors";

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return value;
}

std::string readText(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot read legacy compositor script: " +
                             path.string());
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

// The shipped effects were authored in Cg/HLSL syntax. Ogre's current
// Direct3D renderer can compile the same programs through its HLSL factory;
// this overlay translates only language syntax and obsolete profile names.
std::string splatConstructors(std::string source) {
  for (const int width : {2, 3, 4}) {
    const std::string constructor = "float" + std::to_string(width);
    std::size_t cursor{};
    while ((cursor = source.find(constructor, cursor)) != std::string::npos) {
      const bool wordStart = cursor == 0 ||
          (!std::isalnum(static_cast<unsigned char>(source[cursor - 1])) &&
           source[cursor - 1] != '_');
      std::size_t open = cursor + constructor.size();
      while (open < source.size() &&
             std::isspace(static_cast<unsigned char>(source[open]))) ++open;
      if (!wordStart || open >= source.size() || source[open] != '(') {
        cursor += constructor.size();
        continue;
      }
      int depth = 0;
      std::size_t commas{};
      std::size_t close = open;
      for (; close < source.size(); ++close) {
        if (source[close] == '(') ++depth;
        else if (source[close] == ')') {
          --depth;
          if (depth == 0) break;
        } else if (source[close] == ',' && depth == 1) {
          ++commas;
        }
      }
      if (close == source.size()) break;
      if (commas == 0) {
        source.replace(cursor, constructor.size(),
                       "run3_splat" + std::to_string(width));
        cursor = close + 6;
      } else {
        cursor = close + 1;
      }
    }
  }
  return source;
}

std::string addFragmentPositionInputs(std::string source) {
  // D3D11 requires the pixel input signature to consume the POSITION emitted
  // by Ogre's compositor fullscreen vertex program. Cg accepted entry points
  // that omitted it; HLSL compiles those programs but binds the remaining
  // interpolants incorrectly. Preserve the authored function and parameters,
  // adding only the missing input semantic in the generated overlay.
  static const std::regex functionStart(
      R"(\b(?:float4|half4)[ \t\r\n]+[A-Za-z_][A-Za-z0-9_]*[ \t\r\n]*\()",
      std::regex::icase);
  static const std::regex outputSemantic(
      R"(^[ \t\r\n]*:[ \t\r\n]*(?:COLOR[0-9]*|SV_Target[0-9]*)\b)",
      std::regex::icase);
  static const std::regex positionSemantic(
      R"(:[ \t\r\n]*(?:POSITION[0-9]*|SV_Position)\b)",
      std::regex::icase);

  std::vector<std::size_t> insertions;
  for (std::sregex_iterator match(source.begin(), source.end(), functionStart),
       end;
       match != end; ++match) {
    const std::size_t open = static_cast<std::size_t>(match->position()) +
                             static_cast<std::size_t>(match->length()) - 1;
    int depth{};
    std::size_t close = open;
    for (; close < source.size(); ++close) {
      if (source[close] == '(') ++depth;
      else if (source[close] == ')' && --depth == 0) break;
    }
    if (close == source.size()) continue;
    if (!std::regex_search(source.cbegin() +
                               static_cast<std::ptrdiff_t>(close + 1),
                           source.cend(), outputSemantic,
                           std::regex_constants::match_continuous))
      continue;
    const std::string parameters = source.substr(open + 1, close - open - 1);
    if (std::regex_search(parameters, positionSemantic)) continue;
    insertions.push_back(open + 1);
  }
  for (auto insertion = insertions.rbegin(); insertion != insertions.rend();
       ++insertion) {
    const std::size_t next = source.find_first_not_of(" \t\r\n", *insertion);
    const bool hasParameters = next != std::string::npos &&
                               next < source.find(')', *insertion);
    source.insert(*insertion,
                  std::string("float4 run3Position : POSITION") +
                      (hasParameters ? ", " : ""));
  }
  return source;
}

std::string translateCgSource(std::string source) {
  const std::array<std::pair<const char *, const char *>, 6> words{{
      {"fract", "frac"}, {"mix", "lerp"}, {"mod", "fmod"},
      {"vec2", "float2"}, {"vec3", "float3"}, {"vec4", "float4"}}};
  for (const auto &[from, to] : words) {
    source = std::regex_replace(
        source, std::regex("\\b" + std::string(from) + "\\b"), to);
  }
  source = std::regex_replace(source, std::regex(R"(\batan\s*\()"),
                              "atan2(");
  source = std::regex_replace(source,
                              std::regex(R"((^|\n)[ \t]*ps_3_0[ \t]+)"),
                              "$1");
  source = std::regex_replace(source,
                              std::regex(R"(if\s*\(colour\.xyz\s*!=\s*float3\(0\)\))"),
                              "if (any(colour.xyz != float3(0, 0, 0)))");
  source = std::regex_replace(source,
                              std::regex(R"((^|\n)[ \t]*float2[ \t]+add[ \t]*=)"),
                              "$1 static const float2 add =");
  source = std::regex_replace(
      source,
      std::regex(R"((void[ \t]+tPlane\s*\([^\{]+\{))"),
      "$1\n    float run3HitDistance = hit.dist;\n"
      "    hit = (ITSC)0;\n    hit.dist = run3HitDistance;");
  source = std::regex_replace(source, std::regex(R"(\bITSC[ \t]+i[ \t]*;)"),
                              "ITSC i = (ITSC)0;");
  // Cg selected one of these overloads by profile. The overlay targets SM4,
  // for which the tex2Dlod implementation is the exact selected path.
  source = std::regex_replace(
      source,
      std::regex(R"(\nfloat4[ \t]+TEX2DLOD\(sampler2D map, float2 uv\)\s*\{\s*return tex2D\(map, uv\);\s*\})"),
      "");
  source = addFragmentPositionInputs(std::move(source));
  source = splatConstructors(std::move(source));
  static constexpr const char *helpers = R"(
float2 run3_splat2(float v) { return float2(v, v); }
float2 run3_splat2(float2 v) { return v; }
float3 run3_splat3(float v) { return float3(v, v, v); }
float3 run3_splat3(float3 v) { return v; }
float4 run3_splat4(float v) { return float4(v, v, v, v); }
float4 run3_splat4(float4 v) { return v; }
)";
  return std::string(helpers) + source;
}

void writeCgOverlay(const fs::path &sourceRoot, const fs::path &programCache,
                    const std::string &sourceName,
                    const std::string &overlayName) {
  fs::create_directories(programCache);
  const fs::path output = programCache / overlayName;
  std::string translated = translateCgSource(readText(sourceRoot /
                                                       sourceName));
  if (fs::is_regular_file(output) && readText(output) == translated) return;
  std::ofstream stream(output, std::ios::binary | std::ios::trunc);
  if (!stream)
    throw std::runtime_error("cannot create compositor program overlay: " +
                             output.string());
  stream.write(translated.data(), static_cast<std::streamsize>(translated.size()));
  if (!stream)
    throw std::runtime_error("cannot write compositor program overlay: " +
                             output.string());
}

std::string hlslOverlay(std::string source, const fs::path &sourceRoot,
                        const fs::path &programCache) {
  static const std::regex declaration(
      R"(^([ \t]*(vertex_program|fragment_program)[ \t]+[^ \t\r\n]+[ \t]+)cg([ \t]*(?://.*)?\r?)$)");
  static const std::regex profiles(
      R"(^([ \t]*)profiles[ \t]+([^\r\n]+)(\r?)$)");
  static const std::regex sourceFile(
      R"(^([ \t]*)source[ \t]+([^ \t\r\n]+)([ \t]*(?://.*)?\r?)$)");

  std::istringstream lines(source);
  std::ostringstream output;
  std::string line;
  bool inConvertedProgram = false;
  bool fragment = false;
  int braces = 0;
  while (std::getline(lines, line)) {
    std::smatch match;
    if (!inConvertedProgram && std::regex_match(line, match, declaration)) {
      inConvertedProgram = true;
      fragment = match[2].str() == "fragment_program";
      braces = 0;
      line = match[1].str() + "hlsl" + match[3].str();
    } else if (inConvertedProgram && std::regex_match(line, match, profiles)) {
      const std::string profile = fragment ? "ps_4_0" : "vs_3_0";
      line = match[1].str() + "target " + profile + match[3].str();
    } else if (inConvertedProgram &&
               std::regex_match(line, match, sourceFile)) {
      const std::string original = match[2].str();
      const std::string overlay = "Run3Legacy_" +
          fs::path(original).stem().string() + ".hlsl";
      writeCgOverlay(sourceRoot, programCache, original, overlay);
      line = match[1].str() + "source " + overlay + match[3].str();
    }

    if (inConvertedProgram) {
      braces += static_cast<int>(std::count(line.begin(), line.end(), '{'));
      braces -= static_cast<int>(std::count(line.begin(), line.end(), '}'));
      if (braces == 0 && line.find('}') != std::string::npos)
        inConvertedProgram = false;
    }
    output << line;
    if (!lines.eof()) output << '\n';
  }
  return output.str();
}

std::string modernizeHlslProfiles(std::string source) {
  source = std::regex_replace(source,
                              std::regex(R"((target[ \t]+)vs_1_1\b)"),
                              "$1vs_3_0");
  return source;
}

std::string maskScriptLineComments(std::string source) {
  bool quoted = false;
  char quote{};
  for (std::size_t cursor{}; cursor + 1 < source.size(); ++cursor) {
    if ((source[cursor] == '\'' || source[cursor] == '"') &&
        (cursor == 0 || source[cursor - 1] != '\\')) {
      if (!quoted) {
        quoted = true;
        quote = source[cursor];
      } else if (quote == source[cursor]) {
        quoted = false;
      }
    }
    if (!quoted && source[cursor] == '/' && source[cursor + 1] == '/') {
      while (cursor < source.size() && source[cursor] != '\n')
        source[cursor++] = ' ';
    }
  }
  return source;
}

std::vector<std::string> scriptedCompositorNames(const fs::path &luaRoot) {
  std::unordered_set<std::string> unique;
  if (!fs::is_directory(luaRoot)) return {};
  static const std::regex call(
      R"RUN3(setCompositorEnabled[ \t\r\n]*\([ \t\r\n]*["'](?:true|false)["'][ \t\r\n]*,[ \t\r\n]*["']([^"']+)["'])RUN3",
      std::regex::icase);
  std::error_code error;
  for (fs::recursive_directory_iterator iterator(
           luaRoot, fs::directory_options::skip_permission_denied, error),
       end;
       iterator != end; iterator.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }
    if (!iterator->is_regular_file() ||
        lower(iterator->path().extension().string()) != ".lua")
      continue;
    const std::string source = maskScriptLineComments(readText(iterator->path()));
    for (std::sregex_iterator match(source.begin(), source.end(), call), last;
         match != last; ++match)
      unique.insert((*match)[1].str());
  }
  std::vector<std::string> names(unique.begin(), unique.end());
  std::sort(names.begin(), names.end());
  return names;
}

std::string ensureFullscreenVertexPrograms(std::string source) {
  const std::string structural = maskScriptLineComments(source);
  std::vector<std::size_t> inserts;
  std::size_t cursor{};
  while ((cursor = structural.find("pass", cursor)) != std::string::npos) {
    const bool boundaryBefore = cursor == 0 ||
        (!std::isalnum(static_cast<unsigned char>(structural[cursor - 1])) &&
         structural[cursor - 1] != '_');
    const std::size_t afterWord = cursor + 4;
    const bool boundaryAfter = afterWord >= structural.size() ||
        (!std::isalnum(static_cast<unsigned char>(structural[afterWord])) &&
         structural[afterWord] != '_');
    std::size_t open = structural.find('{', afterWord);
    if (!boundaryBefore || !boundaryAfter || open == std::string::npos ||
        open >= structural.size()) {
      cursor = afterWord;
      continue;
    }
    int depth{};
    std::size_t close = open;
    for (; close < structural.size(); ++close) {
      if (structural[close] == '{') ++depth;
      else if (structural[close] == '}' && --depth == 0) break;
    }
    if (close == structural.size()) break;
    const std::string block = structural.substr(open, close - open + 1);
    const std::regex fragmentLine(R"((^|\n)[ \t]*fragment_program_ref\b)");
    const std::regex vertexLine(R"((^|\n)[ \t]*vertex_program_ref\b)");
    std::smatch fragment;
    if (std::regex_search(block, fragment, fragmentLine) &&
        !std::regex_search(block, vertexLine)) {
      const std::size_t fragmentOffset =
          static_cast<std::size_t>(fragment.position()) +
          (fragment.str().front() == '\n' ? 1U : 0U);
      inserts.push_back(open + fragmentOffset);
    }
    cursor = close + 1;
  }
  for (auto found = inserts.rbegin(); found != inserts.rend(); ++found) {
    const std::size_t lineStart = *found;
    const std::size_t firstText = source.find_first_not_of(" \t", lineStart);
    const std::string indent = source.substr(lineStart, firstText - lineStart);
    source.insert(lineStart,
                  indent + "vertex_program_ref Ogre/Compositor/StdQuad_vp\n" +
                  indent + "{\n" + indent + "}\n");
  }
  return source;
}

std::string namedObject(const std::string &source, const std::string &kind,
                        const std::string &name) {
  std::istringstream lines(source);
  std::ostringstream output;
  std::string line;
  bool copying = false;
  int braces = 0;
  while (std::getline(lines, line)) {
    if (!copying) {
      std::istringstream words(line);
      std::string foundKind;
      std::string foundName;
      words >> foundKind >> foundName;
      if (foundKind != kind || foundName != name) continue;
      copying = true;
    }
    output << line << '\n';
    braces += static_cast<int>(std::count(line.begin(), line.end(), '{'));
    braces -= static_cast<int>(std::count(line.begin(), line.end(), '}'));
    if (copying && braces == 0 && line.find('}') != std::string::npos)
      return output.str();
  }
  throw std::runtime_error("cannot find authored " + kind + " '" + name +
                           "'");
}

// NewHDR contains its own Ogre/Compositor/StdQuad_vp definition. The common
// StdQuad script declares the same unified name, so omit that declaration
// while retaining all authored vertex variants used by runFX.
std::string omitProgram(std::string source, const std::string &programName) {
  std::istringstream lines(source);
  std::ostringstream output;
  std::string line;
  bool skipping = false;
  int braces = 0;
  while (std::getline(lines, line)) {
    if (!skipping) {
      std::istringstream words(line);
      std::string kind;
      std::string name;
      words >> kind >> name;
      if ((kind == "vertex_program" || kind == "fragment_program") &&
          name == programName) {
        skipping = true;
        braces = 0;
      }
    }
    if (skipping) {
      braces += static_cast<int>(std::count(line.begin(), line.end(), '{'));
      braces -= static_cast<int>(std::count(line.begin(), line.end(), '}'));
      if (braces == 0 && line.find('}') != std::string::npos) skipping = false;
      continue;
    }
    output << line;
    if (!lines.eof()) output << '\n';
  }
  return output.str();
}

void parseScript(const fs::path &path, const bool convertCg,
                 const fs::path &programCache, const std::string &omit = {},
                 const std::string &singleCompositor = {},
                 const fs::path &cgSourceRoot = {},
                 const bool addFullscreenVertexPrograms = false) {
  std::string text = readText(path);
  if (!omit.empty()) text = omitProgram(std::move(text), omit);
  if (!singleCompositor.empty())
    text = namedObject(text, "compositor", singleCompositor);
  if (convertCg)
    text = hlslOverlay(std::move(text),
                       cgSourceRoot.empty() ? path.parent_path() : cgSourceRoot,
                       programCache);
  // Legacy game materials name the Cg-only variant directly. The stock Ogre
  // script exposes a unified program for this exact fullscreen vertex shader,
  // allowing D3D11 to select its authored HLSL implementation.
  text = std::regex_replace(
      text,
      std::regex(R"((vertex_program_ref[ \t]+)Ogre/Compositor/StdQuad_Cg_vp\b)"),
      "$1Ogre/Compositor/StdQuad_vp");
  text = std::regex_replace(
      text,
      std::regex(R"((vertex_program_ref[ \t]+)Ogre/Compositor/StdQuad_Tex2a_Cg_vp\b)"),
      "$1Ogre/Compositor/StdQuad_Tex2a_vp");
  if (addFullscreenVertexPrograms)
    text = ensureFullscreenVertexPrograms(std::move(text));
  text = modernizeHlslProfiles(std::move(text));
  Ogre::DataStreamPtr stream(OGRE_NEW Ogre::MemoryDataStream(
      path.filename().string(), text.data(), text.size(), false, true));
  Ogre::ScriptCompilerManager::getSingleton().parseScript(stream,
                                                           resourceGroup);
}

struct SchemeMaterialSource {
  std::string name;
  std::string parent;
  std::string directives;
  std::string techniques;
  std::string templateName;
};

std::size_t closingBrace(const std::string &source, const std::size_t open) {
  int depth{};
  for (std::size_t cursor = open; cursor < source.size(); ++cursor) {
    if (source[cursor] == '{') ++depth;
    else if (source[cursor] == '}' && --depth == 0) return cursor;
  }
  return std::string::npos;
}

std::string trimmed(std::string value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

void collectSchemeMaterials(
    const fs::path &path,
    std::map<std::string, SchemeMaterialSource> &materials) {
  const std::string original = readText(path);
  const std::string structural = maskScriptLineComments(original);
  static const std::regex declaration(
      R"((^|\n)[ \t]*material[ \t]+([^\s:{]+)(?:[ \t]*:[ \t]*([^\s{]+))?)");
  std::size_t search{};
  while (search < structural.size()) {
    std::smatch match;
    const auto first = structural.cbegin() +
        static_cast<std::ptrdiff_t>(search);
    if (!std::regex_search(first, structural.cend(), match, declaration)) break;
    const std::size_t declarationAt = search +
        static_cast<std::size_t>(match.position());
    const std::size_t open = structural.find(
        '{', declarationAt + static_cast<std::size_t>(match.length()));
    if (open == std::string::npos) break;
    const std::size_t close = closingBrace(structural, open);
    if (close == std::string::npos) break;

    SchemeMaterialSource material;
    material.name = match[2].str();
    material.parent = match[3].matched ? match[3].str() : std::string{};
    std::size_t cursor = open + 1;
    while (cursor < close) {
      const std::size_t lineEnd = std::min(
          structural.find('\n', cursor), close);
      const std::string line = trimmed(
          structural.substr(cursor, lineEnd - cursor));
      if (line.rfind("set ", 0) == 0 ||
          line.rfind("set_texture_alias ", 0) == 0) {
        material.directives += original.substr(cursor, lineEnd - cursor) +
                               "\n";
      } else if (line.rfind("technique", 0) == 0) {
        const std::size_t techniqueOpen = structural.find('{', cursor);
        if (techniqueOpen != std::string::npos && techniqueOpen < close) {
          const std::size_t techniqueClose =
              closingBrace(structural, techniqueOpen);
          if (techniqueClose != std::string::npos && techniqueClose <= close) {
            const std::string block = structural.substr(
                cursor, techniqueClose - cursor + 1);
            static const std::regex relevantScheme(
                R"(\bscheme[ \t]+(glow|GlassObj|geom)\b)",
                std::regex::icase);
            if (std::regex_search(block, relevantScheme))
              material.techniques += original.substr(
                  cursor, techniqueClose - cursor + 1) + "\n";
            cursor = techniqueClose + 1;
            continue;
          }
        }
      }
      cursor = lineEnd == close ? close : lineEnd + 1;
    }
    materials[lower(material.name)] = std::move(material);
    search = close + 1;
  }
}

bool inheritsScheme(
    const std::string &key,
    const std::map<std::string, SchemeMaterialSource> &materials,
    std::unordered_map<std::string, bool> &memo,
    std::unordered_set<std::string> &visiting) {
  if (const auto found = memo.find(key); found != memo.end())
    return found->second;
  const auto found = materials.find(key);
  if (found == materials.end() || !visiting.insert(key).second) return false;
  bool relevant = !found->second.techniques.empty();
  if (!relevant && !found->second.parent.empty())
    relevant = inheritsScheme(lower(found->second.parent), materials, memo,
                              visiting);
  visiting.erase(key);
  memo[key] = relevant;
  return relevant;
}

std::unordered_map<std::string, std::string> parseSchemeMaterials(
    const fs::path &contentRoot, const fs::path &preferredRoot) {
  std::map<std::string, SchemeMaterialSource> materials;
  const auto scan = [&](const fs::path &root) {
    if (!fs::is_directory(root)) return;
    std::vector<fs::path> paths;
    std::error_code error;
    for (fs::recursive_directory_iterator iterator(
             root, fs::directory_options::skip_permission_denied, error), end;
         iterator != end; iterator.increment(error)) {
      if (error) {
        error.clear();
        continue;
      }
      if (iterator->is_regular_file(error) &&
          lower(iterator->path().extension().string()) == ".material")
        paths.push_back(iterator->path());
    }
    std::sort(paths.begin(), paths.end());
    for (const fs::path &path : paths) collectSchemeMaterials(path, materials);
  };
  scan(contentRoot / "run3");
  // Match the compatibility material catalogue's selected-quality priority.
  scan(preferredRoot);

  std::unordered_map<std::string, bool> memo;
  std::unordered_set<std::string> visiting;
  std::size_t index{};
  for (auto &[key, material] : materials)
    if (inheritsScheme(key, materials, memo, visiting))
      material.templateName = "Run3/LegacyScheme/" +
                              std::to_string(index++);

  std::ostringstream script;
  std::unordered_set<std::string> emitted;
  const auto emit = [&](const auto &self, const std::string &key) -> void {
    auto found = materials.find(key);
    if (found == materials.end() || found->second.templateName.empty() ||
        !emitted.insert(key).second)
      return;
    SchemeMaterialSource &material = found->second;
    const std::string parentKey = lower(material.parent);
    const auto parent = materials.find(parentKey);
    if (parent != materials.end() && !parent->second.templateName.empty())
      self(self, parentKey);
    script << "material " << material.templateName;
    if (parent != materials.end() && !parent->second.templateName.empty())
      script << " : " << parent->second.templateName;
    script << "\n{\n" << material.directives << material.techniques << "}\n";
  };
  for (const auto &[key, material] : materials) {
    static_cast<void>(material);
    emit(emit, key);
  }
  std::string text = script.str();
  if (!text.empty()) {
    Ogre::DataStreamPtr stream(OGRE_NEW Ogre::MemoryDataStream(
        "Run3LegacySchemes.material", text.data(), text.size(), false, true));
    Ogre::ScriptCompilerManager::getSingleton().parseScript(stream,
                                                             resourceGroup);
  }

  std::unordered_map<std::string, std::string> result;
  for (const auto &[key, material] : materials)
    if (!material.templateName.empty()) result.emplace(key, material.templateName);
  return result;
}

float finiteFloat(const std::string_view text) {
  std::size_t consumed{};
  float value{};
  try {
    value = std::stof(std::string(text), &consumed);
  } catch (const std::exception &) {
    throw std::invalid_argument("invalid compositor parameter value '" +
                                std::string(text) + "'");
  }
  if (consumed == 0 || !std::isfinite(value) ||
      std::any_of(text.begin() + static_cast<std::ptrdiff_t>(consumed),
                  text.end(), [](const unsigned char character) {
                    return !std::isspace(character);
                  }))
    throw std::invalid_argument("invalid compositor parameter value '" +
                                std::string(text) + "'");
  return value;
}

void addLocationIfPresent(Ogre::ResourceGroupManager &resources,
                          const fs::path &path, const bool recursive = false) {
  if (fs::is_directory(path))
    resources.addResourceLocation(path.string(), "FileSystem", resourceGroup,
                                  recursive, true);
}
} // namespace

class OgreCompositorEffects::Impl final
    : public Ogre::CompositorInstance::Listener,
      public Ogre::MaterialManager::Listener {
public:
  explicit Impl(Ogre::Viewport &target) : viewport(target) {}

  ~Impl() override {
    clear();
    try {
      if (materialListenerRegistered)
        Ogre::MaterialManager::getSingleton().removeListener(this);
      if (auto *generator = Ogre::RTShader::ShaderGenerator::getSingletonPtr())
        for (const Ogre::MaterialPtr &material : generatedSchemeMaterials)
          if (material) generator->removeAllShaderBasedTechniques(*material);
      generatedSchemeMaterials.clear();
      generatedSchemeMaterialNames.clear();
      blackMaterial.reset();
      auto &resources = Ogre::ResourceGroupManager::getSingleton();
      if (resources.resourceGroupExists(resourceGroup))
        resources.destroyResourceGroup(resourceGroup);
    } catch (...) {
    }
  }

  void configure(const fs::path &contentRoot, const fs::path &supportAssets,
                 const fs::path &programCache,
                 const std::string_view textureQuality) {
    if (configured) return;
    const fs::path shaders = contentRoot / "run3" / "shaders";
    const fs::path sampleScripts =
        contentRoot / "media" / "materials" / "scripts";
    const fs::path stockCompositors = supportAssets;
    const fs::path shadowPrograms = contentRoot / "run3" / "run3shadow";
    if (!fs::is_directory(shaders) || !fs::is_directory(sampleScripts)) {
      Ogre::LogManager::getSingleton().logMessage(
          "Legacy compositors unavailable: authored shader directories are "
          "absent under " + contentRoot.string());
      return;
    }

    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    if (resources.resourceGroupExists(resourceGroup))
      resources.destroyResourceGroup(resourceGroup);
    resources.createResourceGroup(resourceGroup);
    fs::create_directories(programCache);
    // Stock Ogre programs must win name collisions with the older copies
    // bundled by the game (notably StdQuad_vp.cg).
    addLocationIfPresent(resources, supportAssets);
    addLocationIfPresent(resources, shaders);
    addLocationIfPresent(resources, shadowPrograms);
    addLocationIfPresent(resources, programCache);
    addLocationIfPresent(resources,
                         contentRoot / "media" / "materials" / "textures");
    addLocationIfPresent(resources, contentRoot / "run3" / "mats" /
                                        std::string(textureQuality),
                         true);
    addLocationIfPresent(resources, contentRoot / "run3" / "core", true);
    addLocationIfPresent(resources, contentRoot / "run3" / "models", true);
    addLocationIfPresent(resources, contentRoot / "run3" / "game", true);

    const std::string renderer =
        Ogre::Root::getSingleton().getRenderSystem()->getName();
    const bool direct3D = renderer.find("Direct3D") != std::string::npos;

    if (direct3D) {
      // Parse the original compositor graphs unchanged. Only Cg program
      // declarations are overlaid onto Ogre's HLSL factory; shader sources,
      // materials, passes, inputs, formats and defaults remain authored data.
      // Bloom is the stock Ogre sample, including its original compositor
      // graph, materials, HLSL programs and fullscreen vertex programs.  Do
      // not source these files from a game's legacy media copy: The Long Way
      // ships an older, locally modified version of the Ogre sample.
      parseScript(stockCompositors / "StdQuad_vp.program", false,
                  programCache);
      parseScript(stockCompositors / "Bloom2.material", false, programCache);
      parseScript(stockCompositors / "Examples.compositor", false,
                  programCache, {}, "Bloom");
      parseScript(sampleScripts / "Invert.material", true, programCache,
                  {}, {}, shaders);
      parseScript(sampleScripts / "Examples.compositor", false,
                  programCache, {}, "Invert");
      parseScript(shaders / "runFX.material", true, programCache, {}, {}, {},
                  true);
      parseScript(shaders / "NewHDR.material", true, programCache,
                  "Ogre/Compositor/StdQuad_vp", {}, {}, true);
      for (const char *script : {"glow.program", "ssao.material",
                                 "blur.material"})
        parseScript(shaders / script, true, programCache, {}, {}, {}, true);

      // SSAO's geometry pass is the exact authored geom_vs/geom_ps pair. Only
      // those two declarations are selected so obsolete unrelated lighting
      // programs in diffuse.program are never loaded.
      const std::string shadowDeclarations =
          namedObject(readText(shadowPrograms / "diffuse.program"),
                      "vertex_program", "geom_vs") +
          namedObject(readText(shadowPrograms / "diffuse.program"),
                      "fragment_program", "geom_ps");
      std::string translatedShadowDeclarations = modernizeHlslProfiles(
          hlslOverlay(shadowDeclarations, shadowPrograms, programCache));
      Ogre::DataStreamPtr shadowStream(OGRE_NEW Ogre::MemoryDataStream(
          "Run3LegacyGeom.program", translatedShadowDeclarations.data(),
          translatedShadowDeclarations.size(), false, true));
      Ogre::ScriptCompilerManager::getSingleton().parseScript(shadowStream,
                                                               resourceGroup);

      parseScript(shaders / "NewHDR.compositor", false, programCache, {},
                  "NewHDR");
      for (const char *script : {"runEffects.compositor", "glow.compositor",
                                 "ssao.compositor", "blur.compositor"})
        parseScript(shaders / script, false, programCache);

      const std::vector<std::string> scriptedNames =
          scriptedCompositorNames(contentRoot / "run3" / "lua");
      for (const std::string &name : scriptedNames)
        if (!Ogre::CompositorManager::getSingleton().getByName(
                name, resourceGroup))
          throw std::runtime_error("Lua references compositor '" + name +
                                   "' but no authored definition was loaded");

      schemeMaterials = parseSchemeMaterials(
          contentRoot, contentRoot / "run3" / "mats" /
                           std::string(textureQuality));
      blackMaterial = Ogre::MaterialManager::getSingleton().create(
          "Run3/LegacyScheme/Black", resourceGroup);
      Ogre::Technique *blackTechnique = blackMaterial->getNumTechniques() != 0
          ? blackMaterial->getTechnique(0) : blackMaterial->createTechnique();
      Ogre::Pass *blackPass = blackTechnique->getNumPasses() != 0
          ? blackTechnique->getPass(0) : blackTechnique->createPass();
      blackPass->setLightingEnabled(false);
      blackPass->setAmbient(0, 0, 0);
      blackPass->setDiffuse(0, 0, 0, 0);
      blackPass->setSpecular(0, 0, 0, 0);
      blackPass->setSelfIllumination(0, 0, 0);
      // The legacy GlowMaterialListener used an unlit black fallback for
      // objects without a glow technique.  Suppressing colour writes is the
      // exact, shader-independent equivalent on D3D11 and prevents a generated
      // RTSS fallback from contaminating the glow extraction target.
      blackPass->setColourWriteEnabled(false);
      Ogre::MaterialManager::getSingleton().addListener(this);
      materialListenerRegistered = true;
      Ogre::LogManager::getSingleton().logMessage(
          "Legacy compositor catalogue loaded from authored Ogre scripts and " +
          std::to_string(schemeMaterials.size()) +
          " material-scheme overlays; verified " +
          std::to_string(scriptedNames.size()) +
          " Lua compositor names (" + renderer + ")");
    } else {
      Ogre::LogManager::getSingleton().logMessage(
          "Legacy compositor catalogue: exact custom Cg effects require the "
          "Direct3D/HLSL renderer and were not approximated on " +
          renderer);
    }
    configured = true;
  }

  void setEnabled(const std::string_view requestedName, const bool enabled) {
    if (requestedName.empty()) throw std::invalid_argument("empty compositor name");

    const std::string name(requestedName);
    auto active = instances.find(name);
    if (!enabled) {
      if (active != instances.end()) {
        active->second->setEnabled(false);
        Ogre::CompositorManager::getSingleton().removeCompositor(&viewport,
                                                                 name);
        instances.erase(active);
      }
      return;
    }
    if (!configured)
      throw std::runtime_error("legacy compositor catalogue is not configured");
    const Ogre::CompositorPtr definition =
        Ogre::CompositorManager::getSingleton().getByName(name, resourceGroup);
    if (!definition)
      throw std::runtime_error("authored compositor '" + name +
                               "' is unavailable for the active renderer");
    if (active == instances.end()) {
      Ogre::CompositorInstance *instance =
          Ogre::CompositorManager::getSingleton().addCompositor(
              &viewport, name);
      if (instance == nullptr)
        throw std::runtime_error("cannot attach authored compositor '" + name +
                                 "'");
      instance->addListener(this);
      active = instances.emplace(name, instance).first;
    }
    active->second->setEnabled(true);
  }

  void setParameter(const std::string_view requestedMaterial,
                    const std::string_view parameter,
                    const std::string_view text) {
    if (!configured)
      throw std::runtime_error("legacy compositor catalogue is not configured");
    const float value = finiteFloat(text);
    const std::string materialName(requestedMaterial);
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(
        materialName, resourceGroup);
    if (!material)
      throw std::runtime_error("authored compositor material '" + materialName +
                               "' was not found");
    apply(*material, std::string(parameter), value, true);
    overrides[lower(materialName)][std::string(parameter)] = value;
  }

  void notifyMaterialSetup(const Ogre::uint32,
                           Ogre::MaterialPtr &material) override {
    applyOverrides(material, true);
  }

  void notifyMaterialRender(const Ogre::uint32 passId,
                            Ogre::MaterialPtr &material) override {
    applyOverrides(material, false);
    if (passId != 42) return;

    // Exact camera bindings from the legacy SSAOLogic listener. The authored
    // SSAO compositor assigns identifier 42 to its occlusion pass.
    Ogre::Camera *camera = viewport.getCamera();
    if (camera == nullptr) return;
    const Ogre::Vector3 farCorner =
        camera->getViewMatrix(true) * camera->getWorldSpaceCorners()[4];
    Ogre::Technique *technique = material->getBestTechnique();
    if (technique == nullptr || technique->getNumPasses() == 0) return;
    Ogre::Pass *pass = technique->getPass(0);

    if (pass->hasVertexProgram()) {
      Ogre::GpuProgramParametersSharedPtr parameters =
          pass->getVertexProgramParameters();
      if (parameters->_findNamedConstantDefinition("farCorner", false))
        parameters->setNamedConstant("farCorner", farCorner);
    }
    if (!pass->hasFragmentProgram()) return;
    Ogre::GpuProgramParametersSharedPtr parameters =
        pass->getFragmentProgramParameters();
    static const Ogre::Matrix4 clipSpaceToImageSpace(
        0.5, 0, 0, 0.5,
        0, -0.5, 0, 0.5,
        0, 0, 1, 0,
        0, 0, 0, 1);
    if (parameters->_findNamedConstantDefinition("ptMat", false))
      parameters->setNamedConstant(
          "ptMat", clipSpaceToImageSpace *
                       camera->getProjectionMatrixWithRSDepth());
    if (parameters->_findNamedConstantDefinition("far", false))
      parameters->setNamedConstant("far", camera->getFarClipDistance());
  }

  Ogre::Technique *handleSchemeNotFound(
      const unsigned short, const Ogre::String &scheme,
      Ogre::Material *material, const unsigned short,
      const Ogre::Renderable *) override {
    if (scheme != "glow" && scheme != "GlassObj" && scheme != "geom")
      return nullptr;
    std::string sourceName = material->getName();
    static constexpr std::string_view compatibilityPrefix =
        "Run3/CompatTexture/";
    if (sourceName.rfind(compatibilityPrefix, 0) == 0) {
      const std::size_t slash = sourceName.find('/', compatibilityPrefix.size());
      if (slash != std::string::npos) sourceName.erase(0, slash + 1);
    }
    if (const auto found = schemeMaterials.find(lower(sourceName));
        found != schemeMaterials.end()) {
      Ogre::MaterialPtr source = Ogre::MaterialManager::getSingleton().getByName(
          found->second, resourceGroup);
      if (source) {
        source->load();
        for (Ogre::Technique *technique : source->getTechniques())
          if (technique->getSchemeName() == scheme) {
            if (hasCompletePrograms(technique)) return technique;
            if (Ogre::Technique *generated =
                    shaderBackedTechnique(source, technique, scheme))
              return generated;
          }
      }
    }
    // Exact legacy GlowMaterialListener behavior: objects without an authored
    // glow/glass technique contribute black to those extraction passes. Ogre's
    // D3D11 renderer has no fixed-function pipeline, so RTSS translates this
    // original unlit black technique to its shader equivalent.
    if ((scheme == "glow" || scheme == "GlassObj") && blackMaterial)
      return shaderBackedTechnique(blackMaterial, blackMaterial->getTechnique(0),
                                   scheme);
    return nullptr;
  }

  void clear() noexcept {
    for (const auto &[name, instance] : instances) {
      try {
        instance->setEnabled(false);
        Ogre::CompositorManager::getSingleton().removeCompositor(&viewport,
                                                                 name);
      } catch (...) {
      }
    }
    instances.clear();
    overrides.clear();
  }

private:
  static bool hasCompletePrograms(Ogre::Technique *technique) {
    if (technique == nullptr || technique->getNumPasses() == 0) return false;
    for (Ogre::Pass *pass : technique->getPasses())
      if (!pass->hasVertexProgram() || !pass->hasFragmentProgram()) return false;
    return true;
  }

  Ogre::Technique *shaderBackedTechnique(
      const Ogre::MaterialPtr &material, Ogre::Technique *source,
      const Ogre::String &requestedScheme) {
    if (!material || source == nullptr) return nullptr;
    for (Ogre::Pass *pass : source->getPasses())
      if (pass->hasVertexProgram() || pass->hasFragmentProgram()) return nullptr;

    const Ogre::String generatedScheme =
        "Run3/LegacyRTSS/" + lower(requestedScheme);
    for (Ogre::Technique *technique : material->getTechniques())
      if (technique->getSchemeName() == generatedScheme &&
          hasCompletePrograms(technique))
        return technique;

    auto &generator = Ogre::RTShader::ShaderGenerator::getSingleton();
    if (!generator.createShaderBasedTechnique(source, generatedScheme) ||
        !generator.validateMaterial(generatedScheme, material->getName(),
                                    material->getGroup()))
      return nullptr;
    if (generatedSchemeMaterialNames.insert(material->getName()).second)
      generatedSchemeMaterials.push_back(material);
    for (Ogre::Technique *technique : material->getTechniques())
      if (technique->getSchemeName() == generatedScheme &&
          hasCompletePrograms(technique))
        return technique;
    return nullptr;
  }

  static bool apply(Ogre::Material &material, const std::string &parameter,
                    const float value, const bool required,
                    const bool load = true) {
    if (load) material.load();
    Ogre::Technique *technique = material.getSupportedTechnique(0);
    if (technique == nullptr || technique->getNumPasses() == 0) {
      if (required)
        throw std::runtime_error("compositor material '" + material.getName() +
                                 "' has no supported technique");
      return false;
    }
    Ogre::Pass *pass = technique->getPass(0);
    if (!pass->hasFragmentProgram()) {
      if (required)
        throw std::runtime_error("compositor material '" + material.getName() +
                                 "' has no fragment program");
      return false;
    }
    Ogre::GpuProgramParametersSharedPtr parameters =
        pass->getFragmentProgramParameters();
    if (parameters->_findNamedConstantDefinition(parameter, false) == nullptr) {
      if (required)
        throw std::runtime_error("fragment parameter '" + parameter +
                                 "' does not exist on authored material '" +
                                 material.getName() + "'");
      return false;
    }
    parameters->setNamedConstant(parameter, value);
    return true;
  }

  void applyOverrides(Ogre::MaterialPtr &material, const bool load) {
    std::string sourceName = material->getName();
    const std::size_t slash = sourceName.find('/');
    if (sourceName.size() > 1 && sourceName.front() == 'c' &&
        slash != std::string::npos &&
        std::all_of(sourceName.begin() + 1,
                    sourceName.begin() + static_cast<std::ptrdiff_t>(slash),
                    [](const unsigned char value) { return std::isdigit(value); }))
      sourceName.erase(0, slash + 1);
    const auto found = overrides.find(lower(sourceName));
    if (found == overrides.end()) return;
    for (const auto &[name, value] : found->second)
      static_cast<void>(apply(*material, name, value, false, load));
  }

  Ogre::Viewport &viewport;
  bool configured{};
  bool materialListenerRegistered{};
  Ogre::MaterialPtr blackMaterial;
  std::vector<Ogre::MaterialPtr> generatedSchemeMaterials;
  std::unordered_set<std::string> generatedSchemeMaterialNames;
  std::unordered_map<std::string, std::string> schemeMaterials;
  std::unordered_map<std::string, Ogre::CompositorInstance *> instances;
  std::unordered_map<std::string, std::unordered_map<std::string, float>>
      overrides;
};

OgreCompositorEffects::OgreCompositorEffects(Ogre::Viewport &viewport)
    : impl_(std::make_unique<Impl>(viewport)) {}
OgreCompositorEffects::~OgreCompositorEffects() = default;

void OgreCompositorEffects::configure(const fs::path &contentRoot,
                                      const fs::path &supportAssets,
                                      const fs::path &programCache,
                                      const std::string_view textureQuality) {
  impl_->configure(contentRoot, supportAssets, programCache, textureQuality);
}
void OgreCompositorEffects::setEnabled(const std::string_view name,
                                       const bool enabled) {
  impl_->setEnabled(name, enabled);
}
void OgreCompositorEffects::setShaderParameter(
    const std::string_view material, const std::string_view parameter,
    const std::string_view value) {
  impl_->setParameter(material, parameter, value);
}
void OgreCompositorEffects::update(const double) noexcept {}
void OgreCompositorEffects::clear() noexcept { impl_->clear(); }

} // namespace run3::rendering
