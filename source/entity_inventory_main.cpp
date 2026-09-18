#include <run3/app/AppPaths.hpp>
#include <run3/content/MapDefinition.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct TagUse {
  std::size_t count{};
  std::set<std::string> attributes;
};

using TagUses = std::map<std::string, TagUse>;

void add(const run3::content::AuthoredElement &element, TagUses &uses,
         bool recurse) {
  TagUse &use = uses[element.tag];
  ++use.count;
  for (const auto &[name, value] : element.attributes) {
    static_cast<void>(value);
    use.attributes.insert(name);
  }
  if (recurse && element.tag != "integratedSequence") {
    for (const auto &child : element.children) {
      add(child, uses, true);
    }
  }
}

void printUses(const char *kind, const TagUses &uses) {
  for (const auto &[tag, use] : uses) {
    std::cout << kind << '\t' << tag << '\t' << use.count << '\t';
    bool first = true;
    for (const std::string &attribute : use.attributes) {
      if (!first) {
        std::cout << ',';
      }
      first = false;
      std::cout << attribute;
    }
    std::cout << '\n';
  }
}

std::string counts(const std::vector<run3::content::AuthoredElement> &items) {
  std::map<std::string, std::size_t> result;
  for (const auto &item : items) {
    ++result[item.tag];
  }
  std::string text;
  for (const auto &[tag, count] : result) {
    if (!text.empty()) {
      text += ',';
    }
    text += tag + "=" + std::to_string(count);
  }
  return text.empty() ? "-" : text;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "usage: run3_entity_inventory <content-root> [quality]\n";
    return 2;
  }
  try {
    const fs::path contentRoot = fs::absolute(argv[1]).lexically_normal();
    const std::string quality = argc == 3 ? argv[2] : "low";
    const run3::AppPaths paths = run3::AppPaths::resolve(
        run3::AppPaths::executablePath(argv[0]), contentRoot,
        fs::temp_directory_path() / "run3-entity-inventory-user");
    const fs::path mapsRoot =
        paths.contentPath(fs::path("run3") / "maps" / quality);
    std::vector<std::string> maps;
    for (const fs::directory_entry &entry : fs::directory_iterator(mapsRoot)) {
      if (entry.is_directory() &&
          fs::is_regular_file(entry.path() / "scene.cfg")) {
        maps.push_back(entry.path().filename().string());
      }
    }
    std::sort(maps.begin(), maps.end());

    TagUses sceneUses;
    TagUses declarationUses;
    TagUses eventUses;
    for (const std::string &map : maps) {
      const run3::content::MapDefinition definition =
          run3::content::loadMapDefinition(paths, map, quality);
      add(definition.scene, sceneUses, true);
      std::vector<run3::content::AuthoredElement> declarations;
      std::vector<run3::content::AuthoredElement> events;
      for (const auto &sequence : definition.sequences) {
        declarations.insert(declarations.end(), sequence.declarations.begin(),
                            sequence.declarations.end());
        events.insert(events.end(), sequence.events.begin(),
                      sequence.events.end());
        for (const auto &item : sequence.declarations) {
          add(item, declarationUses, true);
        }
        for (const auto &item : sequence.events) {
          add(item, eventUses, true);
        }
      }
      std::cout << "MAP\t" << map << "\t" << counts(declarations) << "\t"
                << counts(events) << "\tissues=" << definition.issues.size()
                << '\n';
    }
    printUses("SCENE", sceneUses);
    printUses("DECL", declarationUses);
    printUses("EVENT", eventUses);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "run3_entity_inventory: " << error.what() << '\n';
    return 1;
  }
}
