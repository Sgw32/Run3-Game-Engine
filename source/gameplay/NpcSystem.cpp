#include <run3/gameplay/NpcSystem.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace run3::gameplay {
namespace {
using content::AuthoredElement;

std::string origin(const AuthoredElement &element) {
  return element.source.file.generic_string() + ":" +
         std::to_string(element.source.line) + ":" +
         std::to_string(element.source.column) + " [npc '" +
         (element.attribute("name") ? *element.attribute("name") : "unnamed") +
         "']";
}
std::string attribute(const AuthoredElement &element, std::string_view name,
                      std::string fallback = {}) {
  const auto *value = element.attribute(name);
  return value ? *value : std::move(fallback);
}
double number(const AuthoredElement &element, std::string_view name,
              double fallback) {
  const auto *text = element.attribute(name);
  if (!text) return fallback;
  try {
    std::size_t used{};
    const double value = std::stod(*text, &used);
    if (used != text->size() || !std::isfinite(value)) throw std::invalid_argument("number");
    return value;
  } catch (const std::exception &) {
    throw std::runtime_error(origin(element) + ": invalid " +
                             std::string(name) + "='" + *text + "'");
  }
}
bool boolean(const AuthoredElement &element, std::string_view name,
             bool fallback) {
  const auto *value = element.attribute(name);
  if (!value) return fallback;
  if (*value == "true" || *value == "1") return true;
  if (*value == "false" || *value == "0") return false;
  throw std::runtime_error(origin(element) + ": invalid boolean " +
                           std::string(name));
}
physics::Vec3 vector(const AuthoredElement &element,
                     physics::Vec3 fallback = {}) {
  return {number(element, "x", fallback.x), number(element, "y", fallback.y),
          number(element, "z", fallback.z)};
}
physics::Vec3 parseVector(const std::string &text) {
  std::string normalized = text;
  std::replace(normalized.begin(), normalized.end(), ',', ' ');
  std::istringstream input(normalized);
  physics::Vec3 result;
  std::string extra;
  if (!(input >> result.x >> result.y >> result.z) || (input >> extra) ||
      !std::isfinite(result.x) || !std::isfinite(result.y) ||
      !std::isfinite(result.z)) {
    throw std::invalid_argument("expected three finite coordinates, got '" + text + "'");
  }
  return result;
}
physics::Quaternion parseQuaternion(const std::string &text) {
  std::string normalized = text;
  std::replace(normalized.begin(), normalized.end(), ',', ' ');
  std::istringstream input(normalized);
  physics::Quaternion result;
  std::string extra;
  if (!(input >> result.w >> result.x >> result.y >> result.z) ||
      (input >> extra) || !std::isfinite(result.w) ||
      !std::isfinite(result.x) || !std::isfinite(result.y) ||
      !std::isfinite(result.z))
    throw std::invalid_argument("expected four finite quaternion components");
  return result;
}
physics::Quaternion inverse(physics::Quaternion q) {
  const double length = q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z;
  if (length <= 1e-12) return {};
  return {q.w / length, -q.x / length, -q.y / length, -q.z / length};
}
physics::Quaternion multiply(physics::Quaternion a, physics::Quaternion b) {
  return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
          a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
          a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
          a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}
physics::Vec3 rotate(physics::Quaternion q, physics::Vec3 v) {
  const auto r = multiply(multiply(q, {0, v.x, v.y, v.z}), inverse(q));
  return {r.x, r.y, r.z};
}
double separation(physics::Vec3 left, physics::Vec3 right) {
  const double x = right.x - left.x, y = right.y - left.y, z = right.z - left.z;
  return std::sqrt(x * x + y * y + z * z);
}
void appendNodes(const AuthoredElement &element,
                 std::vector<air3::PathNode> &nodes) {
  if (element.tag == "npcnode")
    nodes.push_back({static_cast<air3::NodeId>(element.order + 1), vector(element)});
  if (element.tag == "integratedSequence") return;
  for (const auto &child : element.children) appendNodes(child, nodes);
}
} // namespace

std::optional<NpcEvent> npcEventFromLegacy(int code) noexcept {
  if (code < 0 || code > 32) return std::nullopt;
  return static_cast<NpcEvent>(code);
}

struct NpcSystem::Impl {
  struct Record {
    NpcSnapshot publicState;
    content::SourceLocation source;
    std::string mesh, material, parent, nearScript, useScript, goalScript, deathScript;
    std::string handBone{"Hand"};
    physics::Vec3 parentOffset;
    physics::Quaternion parentRotation;
    physics::Vec3 visualOffset{};
    physics::Vec3 collisionScale{1.0, 1.0, 1.0};
    physics::Vec3 visualRotationAxis{0.0, 1.0, 0.0};
    double visualRotationDegrees{};
    double speed{1.0}, stopDistance{1.0}, renderDistance{10000.0};
    double farFind{1000.0}, attackDistance{130.0}, headshotDistance{20.0};
    double movementMultiplier{1.0}, yShift{};
    bool stopAtDistance{}, animated{true}, ragdoll{}, spawned{};
    bool headshotEnabled{}, suspended{}, nearFired{};
    bool flashlight{};
    std::uint64_t perceptionTick{}, attackTick{};
    std::vector<air3::PathNode> path;
    std::size_t waypoint{1};
  };

  Impl(const content::MapDefinition &map, EntityRegistry &entities,
       const physics::IPhysicsQuery &query, IGameServices &game)
      : registry(&entities), services(&game), navigation(query) {
    std::vector<air3::PathNode> nodes;
    appendNodes(map.scene, nodes);
    navigation.setNodes(std::move(nodes));
    for (const auto *element : content::sequenceDeclarations(map)) {
      if (element->tag != "npc") continue;
      Record npc;
      npc.source = element->source;
      npc.publicState.name = attribute(*element, "name", "unnamed");
      const std::string className = attribute(*element, "className", "npc_enemy");
      if (className == "npc_neutral") npc.publicState.npcClass = NpcClass::Neutral;
      else if (className == "npc_enemy") npc.publicState.npcClass = NpcClass::Enemy;
      else throw std::runtime_error(origin(*element) + ": unsupported live NPC class '" + className + "'");
      npc.mesh = attribute(*element, "meshFile", "ninja.mesh");
      npc.material = attribute(*element, "materialFile");
      npc.handBone = attribute(*element, "handBone", "Hand");
      npc.nearScript = attribute(*element, "cNearScript");
      npc.goalScript = attribute(*element, "scriptOnReach");
      npc.deathScript = attribute(*element, "scriptOnDeath");
      npc.speed = number(*element, "velocity", 1.0);
      npc.stopDistance = number(*element, "stopDist", 1.0);
      npc.renderDistance = number(*element, "renderDist", 10000.0);
      npc.attackDistance = number(*element, "attackAnimDist", 130.0);
      npc.headshotDistance = number(*element, "headshotDist", 20.0);
      if (const auto *far = element->firstChild("farFind"))
        npc.farFind = number(*far, "dist", 1000.0);
      npc.yShift = number(*element, "yShift", 0.0);
      npc.publicState.health = number(*element, "health", 30.0);
      npc.stopAtDistance = boolean(*element, "stopAtDist", false);
      npc.animated = boolean(*element, "animated", true);
      npc.ragdoll = boolean(*element, "ragdoll", false);
      npc.headshotEnabled = boolean(*element, "headshot", false);
      if (npc.speed < 0 || npc.stopDistance < 0 || npc.publicState.health <= 0)
        throw std::runtime_error(origin(*element) + ": negative movement or invalid health");
      if (const auto *position = element->firstChild("position"))
        npc.publicState.transform.position = vector(*position);
      if (const auto *rotation = element->firstChild("rotate")) {
        npc.publicState.transform.rotation = {
            number(*rotation, "qw", 1.0), number(*rotation, "qx", 0.0),
            number(*rotation, "qy", 0.0), number(*rotation, "qz", 0.0)};
      }
      physics::Vec3 scale{1, 1, 1};
      if (const auto *authoredScale = element->firstChild("scale"))
        scale = vector(*authoredScale, scale);
      if (const auto *physPosition = element->firstChild("physPosit"))
        npc.visualOffset = vector(*physPosition);
      if (const auto *physSize = element->firstChild("physSize"))
        npc.collisionScale = vector(*physSize, npc.collisionScale);
      if (const auto *axis = element->firstChild("axis"))
        npc.visualRotationAxis = vector(*axis, npc.visualRotationAxis);
      if (const auto *angle = element->firstChild("angle"))
        npc.visualRotationDegrees = number(*angle, "f", 0.0);
      if (npc.collisionScale.x <= 0 || npc.collisionScale.y <= 0 ||
          npc.collisionScale.z <= 0)
        throw std::runtime_error(origin(*element) +
                                 ": physSize must be positive");
      const auto handles = registry->findAll(npc.publicState.name);
      const auto found = std::find_if(handles.begin(), handles.end(),
          [this, element](EntityHandle handle) {
            const auto &record = registry->get(handle);
            return record.descriptor.owner == EntityOwner::Npc &&
                   record.descriptor.authoredOrder == element->order &&
                   record.descriptor.source.file == element->source.file;
          });
      if (found == handles.end())
        throw std::runtime_error(origin(*element) + ": no typed entity handle");
      npc.publicState.handle = *found;
      npc.publicState.animation = npc.animated ? "Idle1" : "";
      scales.push_back(scale);
      records.push_back(std::move(npc));
    }
    refresh();
  }

  void refresh() {
    publicStates.clear();
    for (const auto &npc : records) publicStates.push_back(npc.publicState);
  }
  Record *find(std::string_view name) {
    auto it = std::find_if(records.begin(), records.end(), [name](const Record &record) {
      return record.publicState.name == name;
    });
    return it == records.end() ? nullptr : &*it;
  }
  Record *find(EntityHandle handle) {
    auto it = std::find_if(records.begin(), records.end(), [handle](const Record &record) {
      return record.publicState.handle == handle;
    });
    return it == records.end() ? nullptr : &*it;
  }
  void setAnimation(Record &npc, std::string name) {
    if (!npc.animated || npc.publicState.animation == name) return;
    npc.publicState.animation = std::move(name);
    services->submit(PlayRuntimeAnimation{npc.publicState.handle,
                                           npc.publicState.animation, true});
  }
  void setGoal(Record &npc, physics::Vec3 goal) {
    npc.suspended = false;
    npc.publicState.goal = goal;
    npc.path = navigation.search(
        {0, npc.publicState.transform.position}, {0, goal});
    npc.waypoint = 1;
    npc.publicState.state = npc.path.empty() ? NpcState::Blocked : NpcState::Navigating;
    if (npc.publicState.state == NpcState::Blocked)
      services->submit(RuntimeLog{"NPC '" + npc.publicState.name +
                                   "' has no path to requested goal"});
    else setAnimation(npc, "Walk");
  }
  void kill(Record &npc) {
    if (npc.publicState.state == NpcState::Dead) return;
    npc.publicState.state = NpcState::Dead;
    npc.path.clear();
    if (!npc.deathScript.empty()) services->submit(RunRuntimeScript{npc.deathScript});
    services->submit(StopRuntimeSound{npc.publicState.handle});
    if (npc.ragdoll) services->submit(SpawnRuntimeRagdoll{
        npc.publicState.handle, npc.publicState.transform});
    services->submit(SetRuntimeVisible{npc.publicState.handle, false});
    services->submit(SetRuntimeCollision{npc.publicState.handle, false});
  }

  EntityRegistry *registry{};
  IGameServices *services{};
  air3::AirPathFind navigation;
  std::vector<Record> records;
  std::vector<physics::Vec3> scales;
  std::vector<NpcSnapshot> publicStates;
  bool started{}, unloaded{};
};

NpcSystem::NpcSystem(const content::MapDefinition &definition,
                     EntityRegistry &registry, const physics::IPhysicsQuery &query,
                     IGameServices &services)
    : impl_(std::make_unique<Impl>(definition, registry, query, services)) {}
NpcSystem::~NpcSystem() {
  try { unload(); } catch (...) {}
}

void NpcSystem::start() {
  if (impl_->unloaded) throw std::logic_error("NPC map already unloaded");
  if (impl_->started) return;
  impl_->started = true;
  for (std::size_t i = 0; i < impl_->records.size(); ++i) {
    auto &npc = impl_->records[i];
    RuntimeEntitySpec spec;
    spec.handle = npc.publicState.handle;
    spec.kind = RuntimeEntityKind::Npc;
    spec.name = npc.publicState.name;
    spec.mesh = npc.mesh;
    spec.material = npc.material;
    spec.transform = npc.publicState.transform;
    spec.scale = impl_->scales[i];
    spec.visualOffset = npc.visualOffset;
    spec.collisionScale = npc.collisionScale;
    spec.visualRotationAxis = npc.visualRotationAxis;
    spec.visualRotationDegrees = npc.visualRotationDegrees;
    spec.visualYawDegrees = npc.yShift;
    spec.renderDistance = npc.renderDistance;
    spec.handBone = npc.handBone;
    // Ogre presentation derives the exact legacy box from the loaded mesh
    // AABB * authored scale * physSize. This fallback is used only if no mesh
    // presentation can be created.
    spec.halfExtents = {20, 90, 20};
    impl_->services->submit(SpawnRuntimeEntity{std::move(spec)});
    npc.spawned = true;
    if (npc.animated) impl_->services->submit(PlayRuntimeAnimation{
        npc.publicState.handle, npc.publicState.animation, true});
  }
  impl_->refresh();
}

void NpcSystem::fixedUpdate(double seconds) {
  if (!impl_->started || impl_->unloaded) return;
  if (!std::isfinite(seconds) || seconds < 0 || seconds > 1.0)
    throw std::invalid_argument("invalid NPC fixed step");
  impl_->services->submit(TickRuntimeNpcPhysics{seconds});
  for (auto &npc : impl_->records) {
    if (!npc.nearFired && !npc.nearScript.empty() &&
        separation(npc.publicState.transform.position,
                   impl_->services->playerPosition()) < 200.0) {
      npc.nearFired = true;
      impl_->services->submit(RunRuntimeScript{npc.nearScript});
    }
    if (npc.publicState.npcClass == NpcClass::Enemy && !npc.suspended &&
        npc.publicState.state != NpcState::Dead &&
        ++npc.perceptionTick % 30 == 0) {
      const auto player = impl_->services->playerPosition();
      const double gap = separation(npc.publicState.transform.position, player);
      if (gap <= npc.farFind &&
          (npc.publicState.state == NpcState::Idle ||
           npc.publicState.state == NpcState::Reached ||
           npc.publicState.state == NpcState::Blocked))
        impl_->setGoal(npc, player);
      if (gap <= npc.attackDistance && npc.perceptionTick - npc.attackTick >= 60) {
        npc.attackTick = npc.perceptionTick;
        impl_->services->submit(DamageRuntimePlayer{10.0});
      }
    }
    if (!npc.parent.empty()) {
      const auto parent = impl_->services->runtimeTransform(npc.parent);
      if (parent) {
        const physics::Vec3 offset = rotate(parent->rotation, npc.parentOffset);
        npc.publicState.transform.position = {
            parent->position.x + offset.x,
            parent->position.y + offset.y,
            parent->position.z + offset.z};
        npc.publicState.transform.rotation =
            multiply(parent->rotation, npc.parentRotation);
        impl_->services->submit(SetRuntimeTransform{npc.publicState.handle,
                                                     npc.publicState.transform});
      }
      continue;
    }
    if (npc.publicState.state != NpcState::Navigating) continue;
    if (npc.waypoint >= npc.path.size()) {
      npc.publicState.state = NpcState::Reached;
    } else {
      const physics::Vec3 target = npc.path[npc.waypoint].position;
      auto &position = npc.publicState.transform.position;
      const double gap = separation(position, target);
      const double move = npc.speed * npc.movementMultiplier * seconds;
      const double arrival = npc.waypoint + 1 == npc.path.size() &&
                             npc.stopAtDistance ? npc.stopDistance : 0.0;
      if (gap <= arrival + move) {
        if (gap > arrival && gap > 0) {
          const double fraction = (gap - arrival) / gap;
          position = {position.x + (target.x - position.x) * fraction,
                      position.y + (target.y - position.y) * fraction,
                      position.z + (target.z - position.z) * fraction};
        }
        ++npc.waypoint;
        if (npc.waypoint >= npc.path.size()) npc.publicState.state = NpcState::Reached;
      } else if (gap > 0) {
        const double fraction = move / gap;
        position = {position.x + (target.x - position.x) * fraction,
                    position.y + (target.y - position.y) * fraction,
                    position.z + (target.z - position.z) * fraction};
      }
      const double dx = target.x - position.x;
      const double dz = target.z - position.z;
      if (std::abs(dx) + std::abs(dz) > 1e-8) {
        const double yaw = std::atan2(-dx, -dz) * 0.5;
        npc.publicState.transform.rotation = {std::cos(yaw), 0,
                                               std::sin(yaw), 0};
      }
      impl_->services->submit(SetRuntimeTransform{npc.publicState.handle,
                                                   npc.publicState.transform});
    }
    if (npc.publicState.state == NpcState::Reached) {
      impl_->setAnimation(npc, "Idle1");
      if (!npc.goalScript.empty())
        impl_->services->submit(RunRuntimeScript{npc.goalScript});
    }
  }
  impl_->refresh();
}

void NpcSystem::dispatch(const NpcRuntimeCommand &command) {
  if (!impl_->started || impl_->unloaded)
    throw std::logic_error("NPC command outside active map");
  const auto event = npcEventFromLegacy(command.legacyEvent);
  if (!event) throw std::invalid_argument("unknown NPC event " +
                                          std::to_string(command.legacyEvent));
  auto apply = [this, &command, event](Impl::Record &npc) {
    if (npc.publicState.state == NpcState::Dead && *event != NpcEvent::Spawn) return;
    switch (*event) {
    case NpcEvent::GoTo: impl_->setGoal(npc, parseVector(command.argument)); break;
    case NpcEvent::RunTo: impl_->setGoal(npc, impl_->services->playerPosition()); break;
    case NpcEvent::Stop: npc.publicState.state = NpcState::Idle; npc.path.clear();
      npc.suspended = true;
      impl_->setAnimation(npc, "Idle1"); break;
    case NpcEvent::Resume:
      npc.suspended = false;
      if (npc.publicState.state == NpcState::Idle) {
        impl_->setGoal(npc, npc.publicState.goal);
      }
      break;
    case NpcEvent::Spawn: npc.publicState.state = NpcState::Idle;
      npc.publicState.health = std::max(1.0, npc.publicState.health);
      impl_->services->submit(SetRuntimeVisible{npc.publicState.handle, true});
      impl_->services->submit(SetRuntimeCollision{npc.publicState.handle, true});
      break;
    case NpcEvent::Kill: impl_->kill(npc); break;
    case NpcEvent::SetAnimation:
      npc.publicState.state = NpcState::Idle;
      npc.path.clear();
      npc.suspended = true;
      impl_->setAnimation(npc, command.argument);
      break;
    case NpcEvent::TransitAnimation:
      impl_->setAnimation(npc, command.argument); break;
    case NpcEvent::Alert: npc.movementMultiplier = 2.0; break;
    case NpcEvent::Fear:
    case NpcEvent::Crazy: npc.movementMultiplier = 0.3; break;
    case NpcEvent::SetMoveActivity: {
      std::size_t used{};
      const double value = std::stod(command.argument, &used);
      if (used != command.argument.size() || !std::isfinite(value) || value < 0)
        throw std::invalid_argument("invalid NPC movement activity");
      npc.movementMultiplier = value;
      break;
    }
    case NpcEvent::Teleport: npc.publicState.transform.position = parseVector(command.argument);
      impl_->services->submit(SetRuntimeTransform{npc.publicState.handle,
                                                   npc.publicState.transform}); break;
    case NpcEvent::SetGoalScript: npc.goalScript = command.argument; break;
    case NpcEvent::SetUseScript: npc.useScript = command.argument; break;
    case NpcEvent::SetParent: {
      const auto parent = impl_->services->runtimeTransform(command.argument);
      if (!parent) {
        impl_->services->submit(RuntimeLog{"warning: NPC '" + npc.publicState.name +
          "' cannot attach to missing parent '" + command.argument + "'"});
        break;
      }
      npc.parent = command.argument;
      const physics::Vec3 delta{
          npc.publicState.transform.position.x - parent->position.x,
          npc.publicState.transform.position.y - parent->position.y,
          npc.publicState.transform.position.z - parent->position.z};
      npc.parentOffset = rotate(inverse(parent->rotation), delta);
      npc.parentRotation = multiply(inverse(parent->rotation),
                                    npc.publicState.transform.rotation);
      break;
    }
    case NpcEvent::ResetParent: npc.parent.clear(); break;
    case NpcEvent::TeleportParent: {
      if (npc.parent.empty())
        throw std::invalid_argument("NPC has no parent for relative teleport");
      const auto parent = impl_->services->runtimeTransform(npc.parent);
      if (!parent) throw std::invalid_argument("NPC parent is missing");
      npc.parentOffset = parseVector(command.argument);
      const auto offset = rotate(parent->rotation, npc.parentOffset);
      npc.publicState.transform.position = {
          parent->position.x + offset.x, parent->position.y + offset.y,
          parent->position.z + offset.z};
      impl_->services->submit(SetRuntimeTransform{npc.publicState.handle,
                                                   npc.publicState.transform});
      break;
    }
    case NpcEvent::RotateOverride:
      npc.publicState.transform.rotation = parseQuaternion(command.argument);
      impl_->services->submit(SetRuntimeTransform{npc.publicState.handle,
                                                   npc.publicState.transform});
      break;
    case NpcEvent::AttachPhysicsObject:
    case NpcEvent::AttachPhysicsObject2:
    case NpcEvent::DetachPhysicsObject:
    case NpcEvent::DetachPhysicsObject2:
      impl_->services->submit(SetNpcAttachment{
          npc.publicState.handle, command.argument, npc.handBone,
          *event == NpcEvent::AttachPhysicsObject ||
          *event == NpcEvent::AttachPhysicsObject2});
      break;
    case NpcEvent::FacialActivity:
      impl_->services->submit(PlayRuntimeFacial{npc.publicState.handle,
        command.argument, npc.publicState.transform.position}); break;
    default: throw std::invalid_argument("NPC event " +
                std::to_string(command.legacyEvent) + " is not implemented for '" +
                npc.publicState.name + "'");
    }
  };
  if (command.broadcast) {
    for (auto &npc : impl_->records) apply(npc);
  } else if (auto *npc = impl_->find(command.name)) {
    apply(*npc);
  } else {
    impl_->services->submit(RuntimeLog{"warning: missing NPC '" + command.name +
                                      "' for event " +
                                      std::to_string(command.legacyEvent)});
  }
  impl_->refresh();
}

bool NpcSystem::damage(EntityHandle handle, double amount, bool headshot) {
  if (!impl_->registry->valid(handle) || amount < 0 || !std::isfinite(amount)) return false;
  auto *npc = impl_->find(handle);
  if (!npc || npc->publicState.state == NpcState::Dead) return false;
  npc->publicState.health -= amount;
  if (headshot && npc->headshotEnabled)
    impl_->services->submit(RuntimeLog{"NPC headshot: '" +
                                      npc->publicState.name + "'"});
  if (npc->publicState.health <= 0) impl_->kill(*npc);
  impl_->refresh();
  return true;
}
bool NpcSystem::damage(EntityHandle handle, double amount,
                       physics::Vec3 hitPosition) {
  auto *npc = impl_->find(handle);
  if (!npc) return false;
  return damage(handle, amount, npc->headshotEnabled &&
      hitPosition.y - npc->publicState.transform.position.y >
          npc->headshotDistance);
}
bool NpcSystem::use(EntityHandle handle) {
  if (!impl_->registry->valid(handle)) return false;
  auto *npc = impl_->find(handle);
  if (!npc || npc->publicState.state == NpcState::Dead || npc->useScript.empty()) return false;
  impl_->services->submit(RunRuntimeScript{npc->useScript});
  return true;
}
bool NpcSystem::destroy(std::string_view name) {
  const auto found = std::find_if(impl_->records.begin(), impl_->records.end(),
      [name](const Impl::Record &record) {
        return record.publicState.name == name;
      });
  if (found == impl_->records.end()) return false;
  const std::size_t index = static_cast<std::size_t>(found - impl_->records.begin());
  if (found->spawned)
    impl_->services->submit(DestroyRuntimeEntity{found->publicState.handle});
  impl_->registry->destroy(found->publicState.handle);
  impl_->records.erase(found);
  impl_->scales.erase(impl_->scales.begin() + static_cast<std::ptrdiff_t>(index));
  impl_->refresh();
  return true;
}
void NpcSystem::unload() {
  if (!impl_ || impl_->unloaded) return;
  impl_->unloaded = true;
  for (auto it = impl_->records.rbegin(); it != impl_->records.rend(); ++it) {
    if (it->spawned) impl_->services->submit(DestroyRuntimeEntity{it->publicState.handle});
  }
  impl_->records.clear();
  impl_->scales.clear();
  impl_->refresh();
}
std::optional<NpcSnapshot> NpcSystem::state(std::string_view name) const {
  const auto *npc = impl_->find(name);
  return npc ? std::optional<NpcSnapshot>{npc->publicState} : std::nullopt;
}
const std::vector<NpcSnapshot> &NpcSystem::states() const noexcept {
  return impl_->publicStates;
}
std::size_t NpcSystem::size() const noexcept { return impl_->records.size(); }

} // namespace run3::gameplay
