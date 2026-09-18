#pragma once

#include <run3/content/MapDefinition.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace run3::gameplay {

struct EntityId {
  std::uint64_t value{};

  friend constexpr bool operator==(EntityId left, EntityId right) noexcept {
    return left.value == right.value;
  }
};

struct EntityHandle {
  EntityId id;
  std::uint32_t generation{};

  friend constexpr bool operator==(EntityHandle left,
                                   EntityHandle right) noexcept {
    return left.id == right.id && left.generation == right.generation;
  }
};

enum class EntityOwner {
  StaticMap,
  DynamicPhysics,
  Sequence,
  Npc,
  AiGraph,
  Presentation,
  Deferred
};

struct EntityDescriptor {
  std::string name;
  std::string authoredName;
  std::string tag;
  EntityOwner owner{EntityOwner::Deferred};
  content::SourceLocation source;
  std::size_t authoredOrder{};
};

struct EntityRecord {
  EntityHandle handle;
  EntityDescriptor descriptor;
  std::optional<std::uint64_t> presentationKey;
  std::optional<std::uint64_t> physicsKey;
};

struct EntityReference {
  EntityHandle source;
  std::string role;
  std::string targetName;
  content::SourceLocation location;
  bool required{true};
  std::optional<EntityHandle> target;
};

class EntityRegistryError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class EntityRegistry final {
public:
  using DestroyCallback = std::function<void()>;

  EntityRegistry();
  ~EntityRegistry();
  EntityRegistry(const EntityRegistry &) = delete;
  EntityRegistry &operator=(const EntityRegistry &) = delete;

  [[nodiscard]] EntityHandle create(EntityDescriptor descriptor,
                                    DestroyCallback onDestroy = {});
  [[nodiscard]] bool valid(EntityHandle handle) const noexcept;
  [[nodiscard]] EntityRecord &get(EntityHandle handle);
  [[nodiscard]] const EntityRecord &get(EntityHandle handle) const;
  [[nodiscard]] std::optional<EntityHandle> findFirst(std::string_view name) const;
  [[nodiscard]] std::vector<EntityHandle> findAll(std::string_view name) const;

  void bindPresentation(EntityHandle handle, std::uint64_t key);
  void bindPhysics(EntityHandle handle, std::uint64_t key);
  void destroy(EntityHandle handle);
  void clear() noexcept;

  void defer(EntityReference reference);
  [[nodiscard]] std::vector<content::DefinitionIssue>
  resolveReferences(bool rejectMissing = true);
  [[nodiscard]] const std::vector<EntityReference> &references() const noexcept {
    return references_;
  }
  [[nodiscard]] std::size_t size() const noexcept { return liveCount_; }

private:
  struct Slot;
  std::vector<Slot> slots_;
  std::vector<std::size_t> freeSlots_;
  std::vector<EntityHandle> constructionOrder_;
  std::vector<EntityReference> references_;
  std::size_t liveCount_{};
};

struct RegistryPopulationResult {
  std::vector<content::DefinitionIssue> issues;
  std::size_t sceneObjects{};
  std::size_t aiNodes{};
  std::size_t sequenceDeclarations{};
  std::size_t eventBindings{};
};

[[nodiscard]] RegistryPopulationResult
populateEntityRegistry(const content::MapDefinition &definition,
                       EntityRegistry &registry,
                       bool rejectMissingReferences = false);

} // namespace run3::gameplay
