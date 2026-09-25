#include <run3/gameplay/EntityRegistry.hpp>

#include <algorithm>
#include <exception>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace {

std::string locationText(const content::SourceLocation &source) {
  return source.file.generic_string() + ":" + std::to_string(source.line) +
         ":" + std::to_string(source.column);
}

std::string syntheticName(const content::AuthoredElement &element) {
  return "@" + element.tag + ":" + element.source.file.filename().string() +
         ":" + std::to_string(element.source.line) + ":" +
         std::to_string(element.order);
}

EntityOwner ownerFor(const std::string &tag) {
  if (tag == "npc") {
    return EntityOwner::Npc;
  }
  if (tag == "npcnode") {
    return EntityOwner::AiGraph;
  }
  if (tag == "phys" || tag == "breakable" || tag == "pblock" ||
      tag == "blockbox" || tag == "ragdoll") {
    return EntityOwner::DynamicPhysics;
  }
  if (tag == "entity" || tag == "nocollide" || tag == "light" ||
      tag == "camera" || tag == "particleSystem" || tag == "fire" ||
      tag == "tree" || tag == "mirror") {
    return EntityOwner::StaticMap;
  }
  if (tag == "flare" || tag == "darkzone") {
    return EntityOwner::Presentation;
  }
  if (tag == "computer" || tag == "cutscene")
    return EntityOwner::Presentation;
  return EntityOwner::Sequence;
}

const std::string *authoredName(const content::AuthoredElement &element) {
  if (const std::string *name = element.attribute("name")) {
    return name;
  }
  if (const std::string *name = element.attribute("buttonName")) {
    return name;
  }
  return element.attribute("fuzzyName");
}

EntityHandle registerElement(const content::AuthoredElement &element,
                             EntityRegistry &registry) {
  const std::string *name = authoredName(element);
  EntityDescriptor descriptor;
  descriptor.name = name == nullptr || name->empty() ? syntheticName(element)
                                                      : *name;
  descriptor.authoredName = name == nullptr ? std::string{} : *name;
  descriptor.tag = element.tag;
  descriptor.owner = ownerFor(element.tag);
  descriptor.source = element.source;
  descriptor.authoredOrder = element.order;
  return registry.create(std::move(descriptor));
}

void registerScene(const content::AuthoredElement &element,
                   EntityRegistry &registry, RegistryPopulationResult &result,
                   std::unordered_map<std::size_t, EntityHandle> &byOrder) {
  static const std::vector<std::string> objectTags{
      "entity", "phys", "breakable", "pblock", "blockbox", "nocollide",
      "ragdoll", "tree", "mirror", "light", "camera", "particleSystem",
      "fire", "npcnode"};
  if (std::find(objectTags.begin(), objectTags.end(), element.tag) !=
      objectTags.end()) {
    const EntityHandle handle = registerElement(element, registry);
    byOrder.emplace(element.order, handle);
    if (element.tag == "npcnode") {
      ++result.aiNodes;
    } else {
      ++result.sceneObjects;
    }
  }
  if (element.tag == "integratedSequence") {
    return;
  }
  for (const content::AuthoredElement &child : element.children) {
    registerScene(child, registry, result, byOrder);
  }
}

} // namespace

struct EntityRegistry::Slot {
  std::uint32_t generation{1};
  std::optional<EntityRecord> record;
  DestroyCallback onDestroy;
};

EntityRegistry::EntityRegistry() = default;
EntityRegistry::~EntityRegistry() { clear(); }

EntityHandle EntityRegistry::create(EntityDescriptor descriptor,
                                    DestroyCallback onDestroy) {
  if (descriptor.name.empty()) {
    throw EntityRegistryError("entity registry name cannot be empty");
  }
  std::size_t slotIndex{};
  if (freeSlots_.empty()) {
    slotIndex = slots_.size();
    slots_.push_back({});
  } else {
    slotIndex = freeSlots_.back();
    freeSlots_.pop_back();
  }
  Slot &slot = slots_[slotIndex];
  const EntityHandle handle{{static_cast<std::uint64_t>(slotIndex + 1)},
                             slot.generation};
  slot.record = EntityRecord{handle, std::move(descriptor), std::nullopt,
                             std::nullopt};
  slot.onDestroy = std::move(onDestroy);
  constructionOrder_.push_back(handle);
  ++liveCount_;
  return handle;
}

bool EntityRegistry::valid(const EntityHandle handle) const noexcept {
  if (handle.id.value == 0 || handle.id.value > slots_.size()) {
    return false;
  }
  const Slot &slot = slots_[static_cast<std::size_t>(handle.id.value - 1)];
  return slot.generation == handle.generation && slot.record.has_value();
}

EntityRecord &EntityRegistry::get(const EntityHandle handle) {
  if (!valid(handle)) {
    throw EntityRegistryError("stale or unknown entity handle");
  }
  return *slots_[static_cast<std::size_t>(handle.id.value - 1)].record;
}

const EntityRecord &EntityRegistry::get(const EntityHandle handle) const {
  if (!valid(handle)) {
    throw EntityRegistryError("stale or unknown entity handle");
  }
  return *slots_[static_cast<std::size_t>(handle.id.value - 1)].record;
}

std::vector<EntityHandle>
EntityRegistry::findAll(const std::string_view name) const {
  std::vector<EntityHandle> result;
  for (const EntityHandle handle : constructionOrder_) {
    if (valid(handle) && get(handle).descriptor.name == name) {
      result.push_back(handle);
    }
  }
  return result;
}

std::optional<EntityHandle>
EntityRegistry::findFirst(const std::string_view name) const {
  const std::vector<EntityHandle> matches = findAll(name);
  return matches.empty() ? std::nullopt
                         : std::optional<EntityHandle>{matches.front()};
}

void EntityRegistry::bindPresentation(const EntityHandle handle,
                                      const std::uint64_t key) {
  get(handle).presentationKey = key;
}

void EntityRegistry::bindPhysics(const EntityHandle handle,
                                 const std::uint64_t key) {
  get(handle).physicsKey = key;
}

void EntityRegistry::destroy(const EntityHandle handle) {
  if (!valid(handle)) {
    return;
  }
  const std::size_t slotIndex = static_cast<std::size_t>(handle.id.value - 1);
  Slot &slot = slots_[slotIndex];
  std::exception_ptr callbackFailure;
  if (slot.onDestroy) {
    try {
      slot.onDestroy();
    } catch (...) {
      callbackFailure = std::current_exception();
    }
  }
  slot.onDestroy = {};
  slot.record.reset();
  ++slot.generation;
  if (slot.generation == 0) {
    slot.generation = 1;
  }
  freeSlots_.push_back(slotIndex);
  --liveCount_;
  if (callbackFailure) {
    std::rethrow_exception(callbackFailure);
  }
}

void EntityRegistry::clear() noexcept {
  for (auto it = constructionOrder_.rbegin(); it != constructionOrder_.rend();
       ++it) {
    try {
      destroy(*it);
    } catch (...) {
      // Cleanup is noexcept; a failing owner cannot prevent later owners from
      // releasing their map-scoped state.
    }
  }
  constructionOrder_.clear();
  references_.clear();
}

void EntityRegistry::defer(EntityReference reference) {
  if (!valid(reference.source)) {
    throw EntityRegistryError("reference source handle is stale");
  }
  if (reference.targetName.empty()) {
    throw EntityRegistryError("deferred entity reference has an empty target");
  }
  references_.push_back(std::move(reference));
}

std::vector<content::DefinitionIssue>
EntityRegistry::resolveReferences(const bool rejectMissing) {
  std::vector<content::DefinitionIssue> issues;
  for (EntityReference &reference : references_) {
    reference.target.reset();
    const std::vector<EntityHandle> matches = findAll(reference.targetName);
    if (matches.empty()) {
      if (reference.required) {
        content::DefinitionIssue issue{
            content::DefinitionIssueKind::MissingReference,
            reference.location,
            reference.role,
            "missing required target '" + reference.targetName + "'"};
        issues.push_back(issue);
        if (rejectMissing) {
          throw EntityRegistryError(locationText(reference.location) + ": " +
                                    issue.message + " for " + reference.role);
        }
      }
      continue;
    }
    reference.target = matches.front();
    if (matches.size() > 1) {
      issues.push_back(
          {content::DefinitionIssueKind::AmbiguousReference,
           reference.location,
           reference.role,
           "target '" + reference.targetName + "' has " +
               std::to_string(matches.size()) +
               " declarations; legacy first declaration selected"});
    }
  }
  return issues;
}

RegistryPopulationResult
populateEntityRegistry(const content::MapDefinition &definition,
                       EntityRegistry &registry,
                       const bool rejectMissingReferences) {
  RegistryPopulationResult result;
  std::unordered_map<std::size_t, EntityHandle> byOrder;
  registerScene(definition.scene, registry, result, byOrder);

  for (const content::SequenceDefinition &sequence : definition.sequences) {
    for (const content::AuthoredElement &declaration : sequence.declarations) {
      const EntityHandle handle = registerElement(declaration, registry);
      byOrder.emplace(declaration.order, handle);
      ++result.sequenceDeclarations;
    }
  }

  for (const content::SequenceDefinition &sequence : definition.sequences) {
    for (const content::AuthoredElement &event : sequence.events) {
      EntityDescriptor descriptor;
      descriptor.name = syntheticName(event);
      descriptor.tag = event.tag;
      descriptor.owner = EntityOwner::Sequence;
      descriptor.source = event.source;
      descriptor.authoredOrder = event.order;
      const EntityHandle eventHandle = registry.create(std::move(descriptor));
      ++result.eventBindings;
      if (const std::string *target = event.attribute("name");
          target != nullptr && !target->empty()) {
        registry.defer({eventHandle, "<events>/" + event.tag, *target,
                        event.source, true, std::nullopt});
      }
    }
  }

  std::unordered_map<std::string, std::vector<EntityHandle>> names;
  for (const auto &[order, handle] : byOrder) {
    static_cast<void>(order);
    const EntityRecord &record = registry.get(handle);
    if (!record.descriptor.authoredName.empty()) {
      names[record.descriptor.authoredName].push_back(handle);
    }
  }
  for (const auto &[name, handles] : names) {
    if (handles.size() > 1) {
      const EntityRecord &first = registry.get(handles.front());
      result.issues.push_back(
          {content::DefinitionIssueKind::DuplicateName, first.descriptor.source,
           name,
           "preserved " + std::to_string(handles.size()) +
               " declarations; name lookup selects authored first"});
    }
  }
  std::vector<content::DefinitionIssue> referenceIssues =
      registry.resolveReferences(rejectMissingReferences);
  result.issues.insert(result.issues.end(), referenceIssues.begin(),
                       referenceIssues.end());
  return result;
}

} // namespace run3::gameplay
