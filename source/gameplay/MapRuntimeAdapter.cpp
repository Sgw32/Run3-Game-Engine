#include <run3/gameplay/MapRuntimeAdapter.hpp>

namespace run3::gameplay {
namespace {

void collect(const content::AuthoredElement &element, bool insideNode,
             std::vector<const content::AuthoredElement *> &result) {
  if (element.tag == "integratedSequence") {
    return;
  }
  if (element.tag == "nodev") {
    return;
  }
  if (element.tag == "node") {
    insideNode = true;
  }
  if (insideNode && isMapRenderableTag(element.tag)) {
    result.push_back(&element);
    return;
  }
  for (const content::AuthoredElement &child : element.children) {
    collect(child, insideNode, result);
  }
}

} // namespace

bool isMapRenderableTag(const std::string_view tag) noexcept {
  return tag == "entity" || tag == "nocollide" || tag == "phys" ||
         tag == "breakable" || tag == "pblock" || tag == "blockbox";
}

std::vector<const content::AuthoredElement *>
activeMapRenderables(const content::MapDefinition &definition) {
  std::vector<const content::AuthoredElement *> result;
  collect(definition.scene, false, result);
  return result;
}

} // namespace run3::gameplay
