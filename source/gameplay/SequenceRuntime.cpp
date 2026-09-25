#include <run3/gameplay/SequenceRuntime.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace run3::gameplay {
namespace {

using content::AuthoredElement;

std::string locationText(const content::SourceLocation &source) {
  return source.file.generic_string() + ":" + std::to_string(source.line) +
         ":" + std::to_string(source.column);
}

std::string attribute(const AuthoredElement &element, std::string_view name,
                      const char *fallback = "") {
  const std::string *value = element.attribute(name);
  return value == nullptr ? std::string(fallback) : *value;
}

double number(const AuthoredElement &element, std::string_view name,
              double fallback) {
  const std::string *text = element.attribute(name);
  if (text == nullptr || text->empty()) {
    return fallback;
  }
  try {
    std::size_t consumed{};
    const double result = std::stod(*text, &consumed);
    if (consumed != text->size() || !std::isfinite(result)) {
      throw std::invalid_argument("not finite");
    }
    return result;
  } catch (const std::exception &) {
    throw SequenceRuntimeError(element.source, "invalid numeric attribute '" +
                                                   std::string(name) + "'='" +
                                                   *text + "'");
  }
}

bool boolean(const AuthoredElement &element, std::string_view name,
             bool fallback) {
  const std::string *text = element.attribute(name);
  if (text == nullptr || text->empty()) {
    return fallback;
  }
  if (*text == "true" || *text == "1") {
    return true;
  }
  if (*text == "false" || *text == "0") {
    return false;
  }
  throw SequenceRuntimeError(element.source, "invalid boolean attribute '" +
                                                 std::string(name) + "'='" +
                                                 *text + "'");
}

physics::Vec3 vector(const AuthoredElement &element,
                     physics::Vec3 fallback = {}) {
  const double multiplier = number(element, "mul", number(element, "m", 1.0));
  return {number(element, "x", fallback.x) * multiplier,
          number(element, "y", fallback.y) * multiplier,
          number(element, "z", fallback.z) * multiplier};
}

physics::Vec3 scale(const AuthoredElement &element) {
  if (const AuthoredElement *child = element.firstChild("scale")) {
    return vector(*child, {1.0, 1.0, 1.0});
  }
  return {number(element, "sX", 1.0), number(element, "sY", 1.0),
          number(element, "sZ", 1.0)};
}

physics::Transform transform(const AuthoredElement &element) {
  physics::Transform result;
  result.position = vector(element);
  if (const AuthoredElement *position = element.firstChild("position")) {
    result.position = vector(*position);
  }
  const AuthoredElement *rotation = element.firstChild("rotation");
  if (rotation == nullptr) {
    rotation = &element;
  }
  result.rotation = {number(*rotation, "qw", 1.0),
                     number(*rotation, "qx", 0.0),
                     number(*rotation, "qy", 0.0),
                     number(*rotation, "qz", 0.0)};
  return result;
}

physics::Vec3 add(physics::Vec3 left, physics::Vec3 right) {
  return {left.x + right.x, left.y + right.y, left.z + right.z};
}
physics::Vec3 subtract(physics::Vec3 left, physics::Vec3 right) {
  return {left.x - right.x, left.y - right.y, left.z - right.z};
}
physics::Vec3 multiply(physics::Vec3 value, double amount) {
  return {value.x * amount, value.y * amount, value.z * amount};
}
double length(physics::Vec3 value) {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}
physics::Vec3 normalized(physics::Vec3 value) {
  const double magnitude = length(value);
  return magnitude <= 1e-12 ? physics::Vec3{} : multiply(value, 1.0 / magnitude);
}
physics::Vec3 approach(physics::Vec3 current, physics::Vec3 target,
                       double amount) {
  const physics::Vec3 difference = subtract(target, current);
  const double remaining = length(difference);
  return remaining <= amount || remaining <= 1e-12
             ? target
             : add(current, multiply(difference, amount / remaining));
}

physics::Quaternion multiply(physics::Quaternion a, physics::Quaternion b) {
  return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
          a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
          a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
          a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

physics::Quaternion axisAngle(physics::Vec3 axis, double radians) {
  axis = normalized(axis);
  const double half = radians * 0.5;
  const double sine = std::sin(half);
  return {std::cos(half), axis.x * sine, axis.y * sine, axis.z * sine};
}

double approach(double current, double target, double amount) {
  if (current < target) return std::min(current + amount, target);
  if (current > target) return std::max(current - amount, target);
  return target;
}

physics::Quaternion applyEulerDegrees(physics::Quaternion initial,
                                      physics::Vec3 degrees) {
  constexpr double radiansPerDegree = 3.14159265358979323846 / 180.0;
  physics::Quaternion result = initial;
  result = multiply(result, axisAngle({1.0, 0.0, 0.0},
                                      degrees.x * radiansPerDegree));
  result = multiply(result, axisAngle({0.0, 1.0, 0.0},
                                      degrees.y * radiansPerDegree));
  result = multiply(result, axisAngle({0.0, 0.0, 1.0},
                                      degrees.z * radiansPerDegree));
  return result;
}

physics::Quaternion normalized(physics::Quaternion value) {
  const double magnitude = std::sqrt(value.w * value.w + value.x * value.x +
                                     value.y * value.y + value.z * value.z);
  if (magnitude <= 1e-12) return {};
  return {value.w / magnitude, value.x / magnitude, value.y / magnitude,
          value.z / magnitude};
}

physics::Quaternion interpolate(physics::Quaternion from,
                                physics::Quaternion to, double amount) {
  const double dot = from.w * to.w + from.x * to.x + from.y * to.y +
                     from.z * to.z;
  if (dot < 0.0) to = {-to.w, -to.x, -to.y, -to.z};
  return normalized({from.w + (to.w - from.w) * amount,
                     from.x + (to.x - from.x) * amount,
                     from.y + (to.y - from.y) * amount,
                     from.z + (to.z - from.z) * amount});
}

std::uint64_t secondsToTicks(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0) {
    throw std::invalid_argument("delay must be finite and non-negative");
  }
  return static_cast<std::uint64_t>(
      std::ceil(seconds / SequenceRuntime::fixedStepSeconds - 1e-12));
}

bool contains(physics::Vec3 centre, physics::Vec3 half, physics::Vec3 point) {
  return std::abs(point.x - centre.x) <= std::abs(half.x) &&
         std::abs(point.y - centre.y) <= std::abs(half.y) &&
         std::abs(point.z - centre.z) <= std::abs(half.z);
}

double argumentNumber(const scripting::ScriptCall &call, std::size_t index) {
  if (index >= call.arguments.size()) {
    throw std::invalid_argument("missing argument " + std::to_string(index + 1));
  }
  std::size_t consumed{};
  const double value = std::stod(call.arguments[index], &consumed);
  if (consumed != call.arguments[index].size() || !std::isfinite(value)) {
    throw std::invalid_argument("argument " + std::to_string(index + 1) +
                                " is not numeric");
  }
  return value;
}

physics::Vec3 argumentVector(const scripting::ScriptCall &call) {
  if (call.arguments.size() >= 3) {
    return {argumentNumber(call, 0), argumentNumber(call, 1),
            argumentNumber(call, 2)};
  }
  if (call.arguments.size() != 1) {
    throw std::invalid_argument("expected one vector string or three numbers");
  }
  std::istringstream stream(call.arguments.front());
  physics::Vec3 value;
  if (!(stream >> value.x >> value.y >> value.z)) {
    throw std::invalid_argument("argument 1 is not an x y z vector");
  }
  std::string trailing;
  if (stream >> trailing) {
    throw std::invalid_argument("argument 1 has trailing vector data");
  }
  return value;
}

void writeElement(std::ostream &output, const AuthoredElement &element) {
  output << std::quoted(element.tag) << ' '
         << std::quoted(element.source.file.generic_string()) << ' '
         << element.source.line << ' ' << element.source.column << ' '
         << element.order << ' ' << std::quoted(element.text) << ' '
         << element.attributes.size() << ' ' << element.children.size() << '\n';
  for (const auto &[name, value] : element.attributes)
    output << std::quoted(name) << ' ' << std::quoted(value) << '\n';
  for (const AuthoredElement &child : element.children)
    writeElement(output, child);
}

bool readElement(std::istream &input, AuthoredElement &element) {
  std::string file;
  std::size_t attributeCount{}, childCount{};
  if (!(input >> std::quoted(element.tag) >> std::quoted(file) >>
        element.source.line >> element.source.column >> element.order >>
        std::quoted(element.text) >> attributeCount >> childCount))
    return false;
  element.source.file = std::move(file);
  element.attributes.clear();
  element.children.clear();
  for (std::size_t index = 0; index < attributeCount; ++index) {
    std::string name, value;
    if (!(input >> std::quoted(name) >> std::quoted(value))) return false;
    element.attributes.emplace_back(std::move(name), std::move(value));
  }
  element.children.resize(childCount);
  for (AuthoredElement &child : element.children)
    if (!readElement(input, child)) return false;
  return true;
}

} // namespace

SequenceRuntimeError::SequenceRuntimeError(content::SourceLocation source,
                                           std::string cause)
    : std::runtime_error(locationText(source) + " [sequence-runtime]: " + cause),
      source_(std::move(source)) {}

class SequenceRuntime::Impl final {
public:
  struct Record {
    const AuthoredElement *definition{};
    SequenceEntityState publicState;
    RuntimeEntityKind kind{RuntimeEntityKind::Trigger};
    physics::Transform initial;
    physics::Vec3 scale{1.0, 1.0, 1.0};
    physics::Vec3 halfExtents{1.0, 1.0, 1.0};
    physics::Vec3 direction{0.0, 0.0, 1.0};
    physics::Vec3 rotationTarget{};
    physics::Vec3 rotationProgress{};
    std::vector<physics::Vec3> keyPoints;
    std::vector<const AuthoredElement *> authoredKeyPoints;
    std::size_t keyPoint{};
    double speed{};
    double distance{};
    double angle{};
    double phase{};
    std::uint64_t periodTicks{60};
    std::uint64_t nextTick{};
    bool useInteract{};
    bool multiple{};
    bool oneShotFired{};
    bool completionFired{};
    bool reverse{};
    bool infinite{};
    bool rotational{};
    std::string callback;
    std::string enterCallback;
    std::string leaveCallback;
    std::string openSound;
    std::string closeSound;
    std::string movingSound;
    std::string initScript;
    std::string nearScript;
    std::string shutdownScript;
    std::string displayMaterial;
    bool allowVirtualDisplay{true};
    bool nearFired{};
    std::vector<std::pair<std::string, bool>> previousLights;
  };

  struct QueuedAction {
    std::uint64_t dueTick{};
    std::uint64_t order{};
    AuthoredElement action;
  };

  struct CutsceneFrame {
    std::uint64_t tick{};
    physics::Transform transform;
    std::string lookTarget;
  };
  struct CutsceneRun {
    std::uint64_t tick{};
    content::SourceLocation source;
    std::string script;
    bool fired{};
  };
  struct Cutscene {
    const AuthoredElement *definition{};
    std::string name;
    std::vector<CutsceneFrame> frames;
    std::vector<CutsceneRun> runs;
    std::uint64_t lengthTicks{};
    std::uint64_t waitTicks{};
    std::uint64_t elapsedTicks{};
    double skipMultiplier{10.0};
    bool freeze{true};
    bool hideHud{true};
    bool infinite{};
    bool scheduled{};
    bool active{};
    bool skipping{};
    std::string music;
  };

  Impl(const content::MapDefinition &mapDefinition, EntityRegistry &entityRegistry,
       IGameServices &gameServices)
      : definition(&mapDefinition), registry(&entityRegistry),
        services(&gameServices) {
    build();
  }

  EntityHandle handleFor(const AuthoredElement &element) const {
    std::string name = element.attribute("name") != nullptr
                           ? *element.attribute("name")
                           : attribute(element, "buttonName", "");
    if (name.empty()) {
      name = "@" + element.tag + ":" +
             element.source.file.filename().string() + ":" +
             std::to_string(element.source.line) + ":" +
             std::to_string(element.order);
    }
    for (EntityHandle handle : registry->findAll(name)) {
      const EntityRecord &candidate = registry->get(handle);
      if (candidate.descriptor.authoredOrder == element.order &&
          candidate.descriptor.source.file == element.source.file) {
        return handle;
      }
    }
    throw SequenceRuntimeError(element.source,
                               "registry has no matching declaration for <" +
                                   element.tag + "> '" + name + "'");
  }

  void addRecord(const AuthoredElement &element, RuntimeEntityKind kind) {
    Record record;
    record.definition = &element;
    record.kind = kind;
    record.publicState.handle = handleFor(element);
    record.publicState.name = registry->get(record.publicState.handle).descriptor.name;
    record.publicState.tag = element.tag;
    record.initial = transform(element);
    record.publicState.transform = record.initial;
    record.scale = scale(element);
    record.useInteract = boolean(element, "useInteract", false);
    record.publicState.enabled = boolean(element, "enabled", true);
    record.publicState.visible = boolean(element, "show", true);
    record.speed = number(element, "speed", kind == RuntimeEntityKind::Train
                                                ? 30.0 : 10.0);
    record.distance = number(element, "distance", 90.0);
    record.angle = number(element, "angle", 0.0);
    record.direction = {number(element, "dirX", 0.0),
                        number(element, "dirY", 0.0),
                        number(element, "dirZ", 1.0)};
    record.openSound = attribute(element, "openSound", "nosound");
    record.closeSound = attribute(element, "closeSound", "nosound");
    record.movingSound = attribute(element, "movingSound", "none");
    // The authored rest position is not a completed open/close transition.
    // Calling lOnClosed on the initial tick can run scripts for another map.
    record.completionFired = true;

    if (kind == RuntimeEntityKind::Trigger || kind == RuntimeEntityKind::DarkZone) {
      if (const AuthoredElement *position = element.firstChild("position")) {
        record.publicState.transform.position = vector(*position);
      }
      if (const AuthoredElement *extent = element.firstChild("scale")) {
        record.halfExtents = vector(*extent, {1.0, 1.0, 1.0});
        if (kind == RuntimeEntityKind::Trigger) {
          record.halfExtents = multiply(record.halfExtents, 0.5);
        }
      } else {
        const physics::Vec3 minimum{number(element, "x1", 0.0),
                                   number(element, "y1", 0.0),
                                   number(element, "z1", 10.0)};
        const physics::Vec3 maximum{number(element, "x2", 0.0),
                                   number(element, "y2", 10.0),
                                   number(element, "z2", 10.0)};
        record.publicState.transform.position = multiply(add(minimum, maximum), 0.5);
        record.halfExtents = multiply(subtract(maximum, minimum), 0.5);
      }
      record.multiple = boolean(element, "multiple", false);
      record.enterCallback = attribute(element, "luaOnEnter", "");
      record.leaveCallback = attribute(element, "luaOnLeave", "");
    }
    if (kind == RuntimeEntityKind::Train) {
      record.infinite = boolean(element, "inf", false);
      for (const AuthoredElement &child : element.children) {
        if (child.tag == "keyPoint") {
          record.keyPoints.push_back(vector(child));
          record.authoredKeyPoints.push_back(&child);
        }
      }
      if (record.keyPoints.empty()) {
        record.keyPoints.push_back(record.publicState.transform.position);
      }
      record.publicState.active = boolean(element, "start", false);
    }
    if (kind == RuntimeEntityKind::Computer) {
      record.useInteract = true;
      record.initScript = attribute(element, "script", "run3/lua/c64.lua");
      record.nearScript = attribute(element, "cNearScript", "");
      record.shutdownScript = attribute(element, "cShutScript", "");
      record.displayMaterial = attribute(element, "dispMat", "BLACK");
      record.allowVirtualDisplay = boolean(element, "allowVirtualDisplay", true);
    }
    if (kind == RuntimeEntityKind::Door || kind == RuntimeEntityKind::Rotator ||
        kind == RuntimeEntityKind::Pendulum) {
      record.rotational = boolean(element, "rotating", false);
      record.speed = number(element, "rotspeed", record.speed);
      record.rotationTarget = {number(element, "pitch", 0.0),
                               number(element, "yaw", 0.0),
                               number(element, "roll", 0.0)};
      if (kind == RuntimeEntityKind::Pendulum) {
        record.publicState.active = true;
        record.direction = record.rotationTarget;
      } else if (kind == RuntimeEntityKind::Rotator) {
        // In the legacy func_door implementation `rotating` selected angular
        // motion; it did not call Fire(). Rotators therefore start stopped and
        // are toggled by use/script commands.
        record.publicState.active = false;
        record.direction = record.rotationTarget;
      }
      if (kind == RuntimeEntityKind::Pendulum &&
          length(record.direction) <= 1e-12) {
        record.direction = {1.0, 0.0, 0.0};
      }
    }
    records.push_back(std::move(record));
  }

  void build() {
    for (const AuthoredElement *declaration :
         content::sequenceDeclarations(*definition)) {
      const std::string &tag = declaration->tag;
      if (tag == "button") addRecord(*declaration, RuntimeEntityKind::Button);
      else if (tag == "door") addRecord(*declaration, RuntimeEntityKind::Door);
      else if (tag == "rot") addRecord(*declaration, RuntimeEntityKind::Rotator);
      else if (tag == "pendulum") addRecord(*declaration, RuntimeEntityKind::Pendulum);
      else if (tag == "train") addRecord(*declaration, RuntimeEntityKind::Train);
      else if (tag == "trigger") addRecord(*declaration, RuntimeEntityKind::Trigger);
      else if (tag == "ladder") addRecord(*declaration, RuntimeEntityKind::Ladder);
      else if (tag == "pickup") addRecord(*declaration, RuntimeEntityKind::Pickup);
      else if (tag == "darkzone") addRecord(*declaration, RuntimeEntityKind::DarkZone);
      else if (tag == "computer") addRecord(*declaration, RuntimeEntityKind::Computer);
      else if (tag == "cutscene") addCutscene(*declaration);
      else if (tag == "timer") {
        Record timer;
        timer.definition = declaration;
        timer.publicState.handle = handleFor(*declaration);
        timer.publicState.name = registry->get(timer.publicState.handle).descriptor.name;
        timer.publicState.tag = tag;
        timer.kind = RuntimeEntityKind::Trigger;
        timer.periodTicks = std::max<std::uint64_t>(
            1, secondsToTicks(number(*declaration, "period", 1.0)));
        timer.publicState.enabled = boolean(*declaration, "start", false);
        timer.callback = attribute(*declaration, "lua", "");
        records.push_back(std::move(timer));
      } else if (tag == "lua") {
        startupScripts.push_back({declaration->source,
                                  attribute(*declaration, "script", "")});
      } else if (tag == "onexit") {
        exitScripts.push_back({declaration->source,
                               attribute(*declaration, "script", "")});
      }
    }
    for (const AuthoredElement *event : content::sequenceEvents(*definition)) {
      if (event->tag == "trigger") {
        triggerEvents[attribute(*event, "name", "")].push_back(event);
      } else if (event->tag == "cutscene") {
        Cutscene *scene = findCutscene(attribute(*event, "name", ""));
        if (scene == nullptr) {
          services->submit(RuntimeLog{"warning: cutscene event skipped missing cutscene '" +
                                      attribute(*event, "name", "") + "'"});
          continue;
        }
        const double wait = number(*event, "wait", 0.0);
        scene->scheduled = wait >= 0.0;
        scene->waitTicks = wait >= 0.0 ? secondsToTicks(wait) : 0;
        for (const AuthoredElement &run : event->children) {
          if (run.tag != "run") continue;
          scene->runs.push_back({secondsToTicks(number(run, "sec", 0.0)),
                                 run.source, attribute(run, "script", ""),
                                 false});
        }
      }
    }
    refreshPublicStates();
  }

  void addCutscene(const AuthoredElement &element) {
    Cutscene scene;
    scene.definition = &element;
    scene.name = attribute(element, "name", "undefined");
    scene.lengthTicks = std::max<std::uint64_t>(
        1, secondsToTicks(number(element, "length", 20.0)));
    scene.skipMultiplier = number(element, "skipAnimSpeed", 10.0);
    scene.freeze = boolean(element, "freezeb", true);
    scene.hideHud = boolean(element, "hideHUD", true);
    scene.infinite = boolean(element, "inf", false);
    if (boolean(element, "music", false))
      scene.music = attribute(element, "musicFile", "");
    for (const AuthoredElement &frame : element.children) {
      if (frame.tag != "frame") continue;
      physics::Transform pose;
      if (boolean(frame, "p", false)) {
        std::istringstream values(attribute(frame, "pos", "0 0 0"));
        if (!(values >> pose.position.x >> pose.position.y >> pose.position.z))
          throw SequenceRuntimeError(frame.source, "invalid cutscene frame pos");
      } else {
        pose.position = vector(frame);
      }
      if (boolean(frame, "or", false)) {
        std::istringstream values(attribute(frame, "orient", "1 0 0 0"));
        if (!(values >> pose.rotation.w >> pose.rotation.x >> pose.rotation.y >>
              pose.rotation.z))
          throw SequenceRuntimeError(frame.source,
                                     "invalid cutscene frame orient");
      } else {
        pose.rotation = transform(frame).rotation;
      }
      scene.frames.push_back({secondsToTicks(number(frame, "second", 1.0)),
                              pose, attribute(frame, "lookTarget", "")});
    }
    if (scene.frames.empty()) {
      scene.frames.push_back({0, transform(element), {}});
    }
    std::stable_sort(scene.frames.begin(), scene.frames.end(),
                     [](const CutsceneFrame &a, const CutsceneFrame &b) {
                       return a.tick < b.tick;
                     });
    cutscenes.push_back(std::move(scene));
  }

  Cutscene *findCutscene(std::string_view name) {
    const auto found = std::find_if(cutscenes.begin(), cutscenes.end(),
                                    [name](const Cutscene &value) {
                                      return value.name == name;
                                    });
    return found == cutscenes.end() ? nullptr : &*found;
  }

  bool startCutscene(std::string_view name) {
    Cutscene *scene = findCutscene(name);
    if (scene == nullptr) return false;
    finishCutscene();
    for (CutsceneRun &run : scene->runs) run.fired = false;
    scene->elapsedTicks = 0;
    scene->active = true;
    scene->scheduled = false;
    activeCutscene = scene;
    presentationState.activeCutscene = scene->name;
    presentationState.playerFrozen = scene->freeze;
    presentationState.hudVisible = !scene->hideHud;
    services->submit(SetRuntimeHudVisible{presentationState.hudVisible});
    try {
      if (!scene->music.empty())
        services->submit(PlayRuntimeSound{{}, scene->music, {}, false, 1.0F});
    } catch (...) {
      finishCutscene();
      throw;
    }
    updateCutsceneCamera(*scene);
    return true;
  }

  void finishCutscene() {
    if (activeCutscene == nullptr) return;
    activeCutscene->active = false;
    activeCutscene->skipping = false;
    activeCutscene = nullptr;
    presentationState.activeCutscene.clear();
    presentationState.camera.reset();
    presentationState.playerFrozen = activeComputer != nullptr;
    presentationState.hudVisible = true;
    services->submit(SetRuntimeHudVisible{true});
  }

  void updateCutsceneCamera(Cutscene &scene) {
    const CutsceneFrame *before = &scene.frames.front();
    const CutsceneFrame *after = before;
    for (const CutsceneFrame &frame : scene.frames) {
      if (frame.tick <= scene.elapsedTicks) before = &frame;
      if (frame.tick >= scene.elapsedTicks) { after = &frame; break; }
      after = &frame;
    }
    double amount{};
    if (after->tick > before->tick)
      amount = static_cast<double>(scene.elapsedTicks - before->tick) /
               static_cast<double>(after->tick - before->tick);
    physics::Transform pose;
    pose.position = add(before->transform.position,
                        multiply(subtract(after->transform.position,
                                          before->transform.position), amount));
    pose.rotation = interpolate(before->transform.rotation,
                                after->transform.rotation, amount);
    const std::string &lookTarget = after->lookTarget.empty()
                                        ? before->lookTarget
                                        : after->lookTarget;
    if (!lookTarget.empty()) {
      if (const auto target = services->runtimeTransform(lookTarget)) {
        const physics::Vec3 direction = normalized(
            subtract(target->position, pose.position));
        const double yaw = std::atan2(-direction.x, -direction.z);
        const double pitch = std::asin(std::clamp(direction.y, -1.0, 1.0));
        pose.rotation = multiply(axisAngle({0, 1, 0}, yaw),
                                 axisAngle({1, 0, 0}, pitch));
      } else {
        services->submit(RuntimeLog{"warning: cutscene '" + scene.name +
                                    "' look target '" + lookTarget +
                                    "' is missing"});
      }
    }
    presentationState.camera = pose;
  }

  void refreshPublicStates() {
    publicStates.clear();
    publicStates.reserve(records.size());
    for (const Record &record : records) {
      publicStates.push_back(record.publicState);
    }
  }

  Record *find(std::string_view name) {
    const auto found = std::find_if(records.begin(), records.end(),
                                    [name](const Record &record) {
                                      return record.publicState.name == name;
                                    });
    return found == records.end() ? nullptr : &*found;
  }
  Record *findKind(std::string_view name, RuntimeEntityKind kind) {
    const auto found = std::find_if(records.begin(), records.end(),
                                    [name, kind](const Record &record) {
      return record.publicState.name == name && record.kind == kind &&
             record.publicState.tag != "timer";
    });
    return found == records.end() ? nullptr : &*found;
  }
  Record *findTag(std::string_view name, std::string_view tag) {
    const auto found = std::find_if(records.begin(), records.end(),
                                    [name, tag](const Record &record) {
      return record.publicState.name == name && record.publicState.tag == tag;
    });
    return found == records.end() ? nullptr : &*found;
  }
  Record *findPresented(std::string_view name) {
    const auto found = std::find_if(records.begin(), records.end(),
                                    [name](const Record &record) {
      return record.publicState.name == name &&
             record.publicState.tag != "timer" &&
             record.kind != RuntimeEntityKind::DarkZone &&
             record.kind != RuntimeEntityKind::Trigger;
    });
    return found == records.end() ? nullptr : &*found;
  }
  const Record *find(std::string_view name) const {
    const auto found = std::find_if(records.begin(), records.end(),
                                    [name](const Record &record) {
                                      return record.publicState.name == name;
                                    });
    return found == records.end() ? nullptr : &*found;
  }

  static bool elementContainsName(const AuthoredElement &element,
                                  std::string_view name) {
    const std::string *authored = element.attribute("name");
    if (authored != nullptr && *authored == name) return true;
    return std::any_of(element.children.begin(), element.children.end(),
                       [name](const AuthoredElement &child) {
                         return elementContainsName(child, name);
                       });
  }

  bool authoredTargetExists(std::string_view name) const {
    if (elementContainsName(definition->scene, name)) return true;
    for (const content::SequenceDefinition &sequence : definition->sequences) {
      for (const AuthoredElement &declaration : sequence.declarations) {
        if (elementContainsName(declaration, name)) return true;
      }
    }
    return false;
  }

  void submitScript(const content::SourceLocation &source,
                    const std::string &script) {
    if (script.empty() || script == "none") {
      return;
    }
    try {
      services->submit(RunRuntimeScript{script});
    } catch (const std::exception &error) {
      throw SequenceRuntimeError(source, "script '" + script + "' failed: " +
                                             error.what());
    }
  }

  RuntimeEntitySpec spec(const Record &record) const {
    RuntimeEntitySpec result;
    result.handle = record.publicState.handle;
    result.kind = record.kind;
    result.name = record.publicState.name;
    result.transform = record.publicState.transform;
    result.scale = record.scale;
    result.halfExtents = record.halfExtents;
    result.visible = record.publicState.visible;
    result.parent = attribute(*record.definition, "parent", "");
    if (record.kind == RuntimeEntityKind::Button ||
        record.kind == RuntimeEntityKind::Ladder) {
      result.mesh = attribute(*record.definition, "meshName", "box.mesh");
    } else if (record.kind == RuntimeEntityKind::Computer) {
      result.mesh = attribute(*record.definition, "meshFile",
                              "pcomputer_01.mesh");
    } else if (record.kind == RuntimeEntityKind::Train) {
      if (const AuthoredElement *entity = record.definition->firstChild("entity")) {
        result.mesh = attribute(*entity, "meshFile", "box.mesh");
      } else {
        result.mesh = "box.mesh";
      }
    } else if (record.kind != RuntimeEntityKind::Trigger &&
               record.kind != RuntimeEntityKind::DarkZone) {
      result.mesh = attribute(*record.definition, "mesh", "box.mesh");
    }
    result.material = attribute(*record.definition, "materialFile", "");
    result.collision = record.kind != RuntimeEntityKind::DarkZone;
    return result;
  }

  void start() {
    if (isUnloaded) {
      throw std::logic_error("cannot restart an unloaded SequenceRuntime");
    }
    if (isStarted) return;
    isStarted = true;
    for (Record &record : records) {
      if (record.publicState.tag == "timer") {
        record.nextTick = tickNumber + record.periodTicks;
        continue;
      }
      services->submit(SpawnRuntimeEntity{spec(record)});
    }
    for (const auto &[source, script] : startupScripts) {
      submitScript(source, script);
    }
    for (Cutscene &scene : cutscenes) {
      if (scene.scheduled && scene.waitTicks == 0) {
        static_cast<void>(startCutscene(scene.name));
        break;
      }
    }
    refreshPublicStates();
  }

  void queueEvent(const Record &trigger) {
    const auto events = triggerEvents.find(trigger.publicState.name);
    if (events == triggerEvents.end()) return;
    for (const AuthoredElement *event : events->second) {
      const std::uint64_t base = secondsToTicks(number(*event, "sec", 0.0));
      for (const AuthoredElement &action : event->children) {
        const double extra = number(action, "secs", 0.0);
        queue.push_back({tickNumber + base + secondsToTicks(extra),
                         nextQueueOrder++, action});
      }
    }
    std::stable_sort(queue.begin(), queue.end(),
                     [](const QueuedAction &left, const QueuedAction &right) {
                       return left.dueTick < right.dueTick ||
                              (left.dueTick == right.dueTick &&
                               left.order < right.order);
                     });
  }

  bool queueStandaloneEvent(std::string_view name) {
    const auto declarations = content::sequenceDeclarations(*definition);
    const auto events = std::find_if(
        declarations.begin(), declarations.end(),
        [name](const AuthoredElement *declaration) {
          return declaration->tag == "event" &&
                 attribute(*declaration, "name", "") == name;
        });
    if (events == declarations.end()) return false;
    for (const AuthoredElement &action : (*events)->children) {
      queue.push_back({tickNumber + secondsToTicks(number(action, "secs", 0.0)),
                       nextQueueOrder++, action});
    }
    std::stable_sort(queue.begin(), queue.end(),
                     [](const QueuedAction &left, const QueuedAction &right) {
                       return left.dueTick < right.dueTick ||
                              (left.dueTick == right.dueTick &&
                               left.order < right.order);
                     });
    return true;
  }

  void fire(Record &record, bool entering = true) {
    if (!record.publicState.enabled) return;
    if (record.kind == RuntimeEntityKind::Trigger) {
      if (record.multiple) {
        ++record.publicState.activationCount;
        if (entering) {
          record.previousLights.clear();
          for (const AuthoredElement &child : record.definition->children) {
            if (child.tag != "lighton" && child.tag != "lightoff") continue;
            const std::string light = attribute(child, "name", "");
            if (const auto previous = services->lightVisible(light)) {
              record.previousLights.emplace_back(light, *previous);
              services->submit(SetRuntimeLightVisible{
                  light, child.tag == "lighton"});
            }
          }
        } else {
          for (const auto &[light, previous] : record.previousLights) {
            services->submit(SetRuntimeLightVisible{light, previous});
          }
          record.previousLights.clear();
        }
        submitScript(record.definition->source,
                     entering ? record.enterCallback : record.leaveCallback);
      } else if (entering && !record.oneShotFired) {
        record.oneShotFired = true;
        ++record.publicState.activationCount;
        queueEvent(record);
      }
      return;
    }
    ++record.publicState.activationCount;
    if (record.kind == RuntimeEntityKind::Button) {
      submitScript(record.definition->source,
                   attribute(*record.definition, "luaScript", ""));
    } else if (record.kind == RuntimeEntityKind::Door ||
               record.kind == RuntimeEntityKind::Rotator) {
      setDoor(record, !record.publicState.active);
    } else if (record.kind == RuntimeEntityKind::Pickup) {
      record.publicState.enabled = false;
      services->submit(SetRuntimeVisible{record.publicState.handle, false});
      const std::string event = attribute(*record.definition, "event", "");
      if (!event.empty()) {
        services->submit(DeferredLegacyCommand{"pickup-event", event});
      }
    } else if (record.kind == RuntimeEntityKind::Computer) {
      enterComputer(record);
    }
  }

  void enterComputer(Record &record) {
    if (activeComputer == &record) return;
    exitComputer();
    activeComputer = &record;
    presentationState.activeComputer = record.publicState.name;
    presentationState.computerFocused = true;
    presentationState.playerFrozen = true;
    presentationState.hudVisible = false;
    services->submit(SetRuntimeHudVisible{false});
    services->submit(SetComputerPresentation{record.publicState.handle,
                                              record.displayMaterial, true,
                                              record.allowVirtualDisplay});
    try {
      submitScript(record.definition->source, record.initScript);
    } catch (...) {
      static_cast<void>(exitComputer());
      throw;
    }
  }

  bool exitComputer() {
    if (activeComputer == nullptr) return false;
    Record *record = activeComputer;
    activeComputer = nullptr;
    std::exception_ptr failure;
    try {
      services->submit(SetComputerPresentation{record->publicState.handle,
                                                record->displayMaterial, false,
                                                record->allowVirtualDisplay});
    } catch (...) { failure = std::current_exception(); }
    try { submitScript(record->definition->source, record->shutdownScript); }
    catch (...) { if (!failure) failure = std::current_exception(); }
    presentationState.activeComputer.clear();
    presentationState.computerFocused = false;
    presentationState.playerFrozen = activeCutscene != nullptr &&
                                     activeCutscene->freeze;
    presentationState.hudVisible = activeCutscene == nullptr;
    services->submit(SetRuntimeHudVisible{presentationState.hudVisible});
    if (failure) std::rethrow_exception(failure);
    return true;
  }

  void setDoor(Record &record, bool open) {
    if (record.publicState.active == open) return;
    record.publicState.active = open;
    record.completionFired = false;
    const std::string &sound = open ? record.openSound : record.closeSound;
    if (!sound.empty() && sound != "none" && sound != "nosound") {
      services->submit(PlayRuntimeSound{record.publicState.handle, sound,
                                        record.publicState.transform.position,
                                        false, 1.0F});
    }
    if (open) {
      submitScript(record.definition->source,
                   attribute(*record.definition, "lOnStarted", ""));
    }
  }

  void executeAction(const QueuedAction &queued) {
    const AuthoredElement &action = queued.action;
    if (action.tag == "lua") {
      submitScript(action.source, attribute(action, "script", ""));
    } else if (action.tag == "door") {
      const std::string name = attribute(action, "name", "");
      if (name.empty()) {
        throw SequenceRuntimeError(action.source, "door action has no name");
      }
      Record *door = findKind(name, RuntimeEntityKind::Door);
      if (door == nullptr) door = findKind(name, RuntimeEntityKind::Rotator);
      if (door == nullptr) {
        services->submit(RuntimeLog{"warning: door event at " +
                                    locationText(action.source) +
                                    " skipped missing door '" + name + "'"});
        return;
      }
      const std::string event = attribute(action, "event", "open");
      setDoor(*door, event == "open" ? true
                     : event == "close" ? false
                     : !door->publicState.active);
    } else if (action.tag == "changelevel") {
      queue.clear();
      finishCutscene();
      static_cast<void>(exitComputer());
      services->submit(ChangeRuntimeMap{attribute(action, "map", "")});
    } else if (action.tag == "hurt") {
      services->submit(DamageRuntimePlayer{number(action, "damage", 1.0)});
    } else if (action.tag == "player") {
      const AuthoredElement *position = action.firstChild("pos");
      if (position == nullptr) position = action.firstChild("position");
      if (position == nullptr) {
        throw SequenceRuntimeError(action.source,
                                   "player teleport action has no position");
      }
      services->submit(TeleportRuntimePlayer{vector(*position)});
    } else if (action.tag == "entc") {
      services->submit(DeferredLegacyCommand{"entc", attribute(action, "name", "")});
    } else {
      throw SequenceRuntimeError(action.source,
                                 "unsupported required event action <" +
                                     action.tag + ">");
    }
  }

  void updateTrigger(Record &record, physics::Vec3 player) {
    const physics::Vec3 playerHalf = services->playerHalfExtents();
    const bool nowInside = contains(
        record.publicState.transform.position,
        add(record.halfExtents, playerHalf), player);
    if (nowInside != record.publicState.inside) {
      record.publicState.inside = nowInside;
      fire(record, nowInside);
    }
  }

  void updateDoor(Record &record) {
    if (record.rotational) {
      const physics::Vec3 target = record.publicState.active
          ? record.rotationTarget : physics::Vec3{};
      const physics::Vec3 before = record.rotationProgress;
      const double step = std::abs(record.speed) * 5.0 * fixedStepSeconds;
      record.rotationProgress = {
          approach(before.x, target.x, step),
          approach(before.y, target.y, step),
          approach(before.z, target.z, step)};
      if (!(before == record.rotationProgress)) {
        record.publicState.transform.rotation = applyEulerDegrees(
            record.initial.rotation, record.rotationProgress);
        services->submit(SetRuntimeTransform{record.publicState.handle,
                                              record.publicState.transform});
      }
      if (record.rotationProgress == target && !record.completionFired) {
        record.completionFired = true;
        submitScript(record.definition->source,
                     attribute(*record.definition,
                               record.publicState.active ? "lOnOpen"
                                                         : "lOnClosed", ""));
      }
      return;
    }
    physics::Vec3 target = record.initial.position;
    if (record.publicState.active) {
      target = add(target, multiply(record.direction, record.distance));
    }
    const physics::Vec3 before = record.publicState.transform.position;
    record.publicState.transform.position =
        approach(before, target, std::abs(record.speed) * 5.0 * fixedStepSeconds);
    if (!(before == record.publicState.transform.position)) {
      services->submit(SetRuntimeTransform{record.publicState.handle,
                                            record.publicState.transform});
    }
    if (record.publicState.transform.position == target &&
        !record.completionFired) {
      record.completionFired = true;
      submitScript(record.definition->source,
                   attribute(*record.definition,
                             record.publicState.active ? "lOnOpen"
                                                       : "lOnClosed", ""));
    }
  }

  void updateRotator(Record &record) {
    if (!record.rotational || !record.publicState.active) return;
    // Legacy func_door applied each authored Euler channel independently at
    // -rotspeed * dt * 5 * sign(channel), without normalising the vector.
    const double degrees = -record.speed * fixedStepSeconds * 5.0;
    const auto signedStep = [degrees](double channel) {
      return channel < 0.0 ? -degrees : channel > 0.0 ? degrees : 0.0;
    };
    record.publicState.transform.rotation = multiply(
        record.publicState.transform.rotation,
        applyEulerDegrees({}, {signedStep(record.direction.x),
                               signedStep(record.direction.y),
                               signedStep(record.direction.z)}));
    record.publicState.transform.rotation = normalized(
        record.publicState.transform.rotation);
    services->submit(SetRuntimeTransform{record.publicState.handle,
                                          record.publicState.transform});
  }

  void updatePendulum(Record &record) {
    if (!record.publicState.active) return;
    record.phase += record.speed * fixedStepSeconds;
    const double amplitude = record.angle == 0.0 ? 30.0 : record.angle;
    const double radians = std::sin(record.phase) * amplitude *
                           3.14159265358979323846 / 180.0;
    record.publicState.transform.rotation =
        multiply(record.initial.rotation, axisAngle(record.direction, radians));
    services->submit(SetRuntimeTransform{record.publicState.handle,
                                          record.publicState.transform});
  }

  void updateTrain(Record &record) {
    if (!record.publicState.active || record.keyPoints.empty()) return;
    if (record.keyPoints.size() > 1 && record.keyPoint == 0 &&
        record.publicState.transform.position == record.keyPoints.front()) {
      record.keyPoint = record.reverse ? record.keyPoints.size() - 1 : 1;
    }
    const physics::Vec3 before = record.publicState.transform.position;
    const bool carriesPlayer = playerParent != record.publicState.name &&
                               services->playerStandingOn(record.publicState.handle);
    const physics::Vec3 target = record.keyPoints[record.keyPoint];
    record.publicState.transform.position =
        approach(before, target, std::abs(record.speed) * fixedStepSeconds);
    const physics::Vec3 delta = subtract(record.publicState.transform.position,
                                         before);
    services->submit(SetRuntimeTransform{record.publicState.handle,
                                          record.publicState.transform});
    if (carriesPlayer) {
      services->submit(ApplyRuntimeParentMotion{delta});
    }
    if (record.publicState.transform.position == target) {
      if (record.keyPoint < record.authoredKeyPoints.size()) {
        const AuthoredElement &point = *record.authoredKeyPoints[record.keyPoint];
        submitScript(point.source, attribute(point, "script", ""));
      }
      if (record.keyPoints.size() == 1) {
        record.publicState.active = false;
        services->submit(StopRuntimeSound{record.publicState.handle});
      } else {
        const bool atEnd = record.reverse ? record.keyPoint == 0
                                          : record.keyPoint + 1 == record.keyPoints.size();
        if (atEnd && !record.infinite) {
          record.publicState.active = false;
          record.keyPoint = 1;
          services->submit(StopRuntimeSound{record.publicState.handle});
        } else if (atEnd) {
          record.keyPoint = record.reverse ? record.keyPoints.size() - 1 : 0;
          record.publicState.transform.position = record.keyPoints[record.keyPoint];
          services->submit(SetRuntimeTransform{record.publicState.handle,
                                                record.publicState.transform});
        } else {
          record.keyPoint = record.reverse ? record.keyPoint - 1
                                           : record.keyPoint + 1;
        }
      }
    }
  }

  void fixedUpdate() {
    if (!isStarted || isUnloaded) {
      throw std::logic_error("SequenceRuntime fixedUpdate outside active lifecycle");
    }
    ++tickNumber;
    for (Cutscene &scene : cutscenes) {
      if (scene.scheduled && tickNumber >= scene.waitTicks) {
        static_cast<void>(startCutscene(scene.name));
        break;
      }
    }
    if (activeCutscene != nullptr) {
      Cutscene &scene = *activeCutscene;
      const std::uint64_t advance = scene.skipping
          ? std::max<std::uint64_t>(1, static_cast<std::uint64_t>(
                std::floor(scene.skipMultiplier)))
          : 1;
      scene.elapsedTicks = std::min(scene.lengthTicks,
                                    scene.elapsedTicks + advance);
      for (CutsceneRun &run : scene.runs) {
        if (!run.fired && run.tick <= scene.elapsedTicks) {
          run.fired = true;
          try {
            submitScript(run.source, run.script);
          } catch (...) {
            finishCutscene();
            throw;
          }
        }
      }
      updateCutsceneCamera(scene);
      if (scene.elapsedTicks >= scene.lengthTicks) {
        if (scene.infinite) scene.elapsedTicks = 0;
        else finishCutscene();
      }
    }
    const physics::Vec3 player = services->playerPosition();
    double darkness = 1.0;
    for (Record &record : records) {
      if (!record.publicState.enabled) continue;
      if (record.publicState.tag == "timer") {
        if (tickNumber >= record.nextTick) {
          submitScript(record.definition->source, record.callback);
          ++record.publicState.activationCount;
          do { record.nextTick += record.periodTicks; }
          while (record.nextTick <= tickNumber);
        }
      } else if (record.kind == RuntimeEntityKind::Trigger) {
        updateTrigger(record, player);
      } else if (record.kind == RuntimeEntityKind::Door) {
        updateDoor(record);
      } else if (record.kind == RuntimeEntityKind::Rotator) {
        updateRotator(record);
      } else if (record.kind == RuntimeEntityKind::Pendulum) {
        updatePendulum(record);
      } else if (record.kind == RuntimeEntityKind::Train) {
        updateTrain(record);
      } else if (record.kind == RuntimeEntityKind::Pickup &&
                 contains(record.publicState.transform.position,
                          {25.0, 25.0, 25.0}, player)) {
        fire(record);
      } else if (record.kind == RuntimeEntityKind::Computer &&
                 !record.nearFired && !record.nearScript.empty() &&
                 length(subtract(player, record.publicState.transform.position)) <
                     200.0) {
        record.nearFired = true;
        submitScript(record.definition->source, record.nearScript);
      } else if (record.kind == RuntimeEntityKind::DarkZone) {
        const physics::Vec3 displacement = subtract(
            player, record.publicState.transform.position);
        const double x = displacement.x /
                         std::max(1e-6, std::abs(record.halfExtents.x));
        const double y = displacement.y /
                         std::max(1e-6, std::abs(record.halfExtents.y));
        const double z = displacement.z /
                         std::max(1e-6, std::abs(record.halfExtents.z));
        const double exponent = std::max(1e-6, number(
            *record.definition, "exp", 0.1));
        const double factor = 1.0 -
            number(*record.definition, "darken", 0.5) *
                std::exp(-(x * x + y * y + z * z) / exponent);
        darkness *= std::clamp(factor, 0.0, 4.0);
      }
    }
    // Script-authored train parenting is evaluated after train transforms so
    // the player receives the same fixed-tick delta without a frame of lag.
    if (!playerParent.empty()) {
      const auto parent = services->runtimeTransform(playerParent);
      if (parent) {
        if (lastPlayerParentTransform) {
          services->submit(ApplyRuntimeParentMotion{
              subtract(parent->position, lastPlayerParentTransform->position)});
        }
        lastPlayerParentTransform = parent;
      } else {
        services->submit(RuntimeLog{"warning: player parent '" + playerParent +
                                    "' disappeared; binding released"});
        playerParent.clear();
        lastPlayerParentTransform.reset();
      }
    }
    services->submit(SetRuntimeDarkness{darkness});
    while (!queue.empty() && queue.front().dueTick <= tickNumber) {
      const QueuedAction action = queue.front();
      queue.erase(queue.begin());
      executeAction(action);
    }
    refreshPublicStates();
  }

  void unload(bool runOnExit) {
    if (isUnloaded) return;
    queue.clear();
    finishCutscene();
    static_cast<void>(exitComputer());
    playerParent.clear();
    lastPlayerParentTransform.reset();
    std::exception_ptr failure;
    if (runOnExit && isStarted) {
      for (const auto &[source, script] : exitScripts) {
        try {
          submitScript(source, script);
        } catch (...) {
          if (!failure) failure = std::current_exception();
        }
      }
    }
    try {
      services->submit(DestroyRuntimeEntities{});
    } catch (...) {
      if (!failure) failure = std::current_exception();
    }
    isUnloaded = true;
    isStarted = false;
    if (failure) std::rethrow_exception(failure);
  }

  const content::MapDefinition *definition{};
  EntityRegistry *registry{};
  IGameServices *services{};
  std::vector<Record> records;
  std::vector<SequenceEntityState> publicStates;
  std::unordered_map<std::string, std::vector<const AuthoredElement *>> triggerEvents;
  std::vector<std::pair<content::SourceLocation, std::string>> startupScripts;
  std::vector<std::pair<content::SourceLocation, std::string>> exitScripts;
  std::vector<QueuedAction> queue;
  std::vector<Cutscene> cutscenes;
  Cutscene *activeCutscene{};
  Record *activeComputer{};
  SequencePresentationState presentationState;
  std::string playerParent;
  std::optional<physics::Transform> lastPlayerParentTransform;
  std::uint64_t tickNumber{};
  std::uint64_t nextQueueOrder{};
  bool isStarted{};
  bool isUnloaded{};
};

SequenceRuntime::SequenceRuntime(const content::MapDefinition &definition,
                                 EntityRegistry &registry,
                                 IGameServices &services)
    : impl_(std::make_unique<Impl>(definition, registry, services)) {}
SequenceRuntime::~SequenceRuntime() = default;
void SequenceRuntime::start() { impl_->start(); }
void SequenceRuntime::fixedUpdate() { impl_->fixedUpdate(); }
void SequenceRuntime::unload(bool runOnExit) { impl_->unload(runOnExit); }

bool SequenceRuntime::handleInput(const InputEvent &event) {
  if (impl_->activeComputer != nullptr) {
    if (event.type == InputEventType::KeyPressed && event.key == Key::Escape)
      return impl_->exitComputer();
    if (event.type == InputEventType::KeyPressed ||
        event.type == InputEventType::KeyReleased ||
        event.type == InputEventType::TextEntered) {
      impl_->services->submit(SendComputerInput{
          impl_->activeComputer->publicState.handle, event.text,
          static_cast<int>(event.key),
          event.type != InputEventType::KeyReleased});
      return true;
    }
    return event.type == InputEventType::MouseMoved ||
           event.type == InputEventType::MousePressed ||
           event.type == InputEventType::MouseReleased;
  }
  if (impl_->activeCutscene != nullptr &&
      event.type == InputEventType::KeyPressed && !event.repeated &&
      (event.key == Key::Escape || event.key == Key::Space)) {
    impl_->activeCutscene->skipping = true;
    return true;
  }
  return false;
}

bool SequenceRuntime::startCutscene(std::string_view name) {
  return impl_->startCutscene(name);
}
bool SequenceRuntime::skipCutscene() {
  if (impl_->activeCutscene == nullptr) return false;
  impl_->activeCutscene->skipping = true;
  return true;
}
bool SequenceRuntime::exitComputer() { return impl_->exitComputer(); }

bool SequenceRuntime::interact(EntityHandle handle) {
  const auto found = std::find_if(
      impl_->records.begin(), impl_->records.end(),
      [handle](const Impl::Record &record) {
        return record.publicState.handle == handle;
      });
  if (found == impl_->records.end() || !found->publicState.enabled ||
      (!found->useInteract && found->kind != RuntimeEntityKind::Button &&
       found->kind != RuntimeEntityKind::Pickup)) {
    return false;
  }
  impl_->fire(*found);
  impl_->refreshPublicStates();
  return true;
}

bool SequenceRuntime::interactByEntityId(EntityId id) {
  const auto found = std::find_if(
      impl_->records.begin(), impl_->records.end(),
      [id](const Impl::Record &record) { return record.publicState.handle.id == id; });
  return found != impl_->records.end() && interact(found->publicState.handle);
}

bool SequenceRuntime::setTriggerEnabled(std::string_view name, bool enabled) {
  Impl::Record *record = impl_->findKind(name, RuntimeEntityKind::Trigger);
  if (record == nullptr) return false;
  record->publicState.enabled = enabled;
  if (!enabled) record->publicState.inside = false;
  impl_->refreshPublicStates();
  return true;
}

bool SequenceRuntime::setTimerEnabled(std::string_view name, bool enabled) {
  Impl::Record *record = impl_->findTag(name, "timer");
  if (record == nullptr) return false;
  record->publicState.enabled = enabled;
  record->nextTick = impl_->tickNumber + record->periodTicks;
  impl_->refreshPublicStates();
  return true;
}

bool SequenceRuntime::setDoorOpen(std::string_view name, bool open) {
  Impl::Record *record = impl_->findKind(name, RuntimeEntityKind::Door);
  if (record == nullptr) record = impl_->findKind(name, RuntimeEntityKind::Rotator);
  if (record == nullptr) return false;
  impl_->setDoor(*record, open);
  impl_->refreshPublicStates();
  return true;
}

bool SequenceRuntime::setTrainRunning(std::string_view name, bool running) {
  Impl::Record *record = impl_->findKind(name, RuntimeEntityKind::Train);
  if (record == nullptr) return false;
  record->publicState.active = running;
  if (!record->movingSound.empty() && record->movingSound != "none") {
    if (running) {
      impl_->services->submit(PlayRuntimeSound{record->publicState.handle,
                                               record->movingSound,
                                               record->publicState.transform.position,
                                               true, 1.0F});
    } else {
      impl_->services->submit(StopRuntimeSound{record->publicState.handle});
    }
  }
  impl_->refreshPublicStates();
  return true;
}

scripting::ScriptValue
SequenceRuntime::dispatchScriptCall(const scripting::ScriptCall &call) {
  const auto requireName = [&call]() -> const std::string & {
    if (call.arguments.empty() || call.arguments.front().empty()) {
      throw std::invalid_argument(call.name + " requires an entity name");
    }
    return call.arguments.front();
  };
  const auto warnMissing = [this, &call](std::string_view kind,
                                         std::string_view name) {
    impl_->services->submit(RuntimeLog{
        "warning: Lua command " + call.name + " skipped missing " +
        std::string(kind) + " '" + std::string(name) + "'"});
  };
  try {
    if (call.name == "openDoor" || call.name == "closeDoor" ||
        call.name == "toggleDoor") {
      const std::string &name = requireName();
      Impl::Record *door = impl_->findKind(name, RuntimeEntityKind::Door);
      if (door == nullptr) door = impl_->findKind(name, RuntimeEntityKind::Rotator);
      if (door == nullptr) {
        warnMissing("door", name);
      } else {
        const bool open = call.name == "openDoor" ? true
                          : call.name == "closeDoor" ? false
                          : !door->publicState.active;
        setDoorOpen(name, open);
      }
    } else if (call.name == "startTrain" || call.name == "stopTrain" ||
               call.name == "reverseTrain") {
      const std::string &name = requireName();
      Impl::Record *train = impl_->findKind(name, RuntimeEntityKind::Train);
      if (train == nullptr) warnMissing("train", name);
      else if (call.name == "reverseTrain") train->reverse = !train->reverse;
      else setTrainRunning(name, call.name == "startTrain");
    } else if (call.name == "enableTimer" || call.name == "disableTimer" ||
               call.name == "toggleTimer") {
      const std::string &name = requireName();
      Impl::Record *timer = impl_->findTag(name, "timer");
      if (timer == nullptr) warnMissing("timer", name);
      else {
        const bool enabled = call.name == "enableTimer" ? true
                             : call.name == "disableTimer" ? false
                             : !timer->publicState.enabled;
        setTimerEnabled(name, enabled);
      }
    } else if (call.name == "enableTrigger" || call.name == "disableTrigger") {
      const std::string &name = requireName();
      if (!setTriggerEnabled(name, call.name == "enableTrigger"))
        warnMissing("trigger", name);
    } else if (call.name == "teleport" || call.name == "teleport_rel") {
      physics::Vec3 position = argumentVector(call);
      if (call.name == "teleport_rel") position = add(impl_->services->playerPosition(), position);
      impl_->services->submit(TeleportRuntimePlayer{position});
    } else if (call.name == "logMessage") {
      impl_->services->submit(RuntimeLog{call.arguments.empty() ? std::string{} : call.arguments.front()});
    } else if (call.name == "runScript") {
      impl_->services->submit(RunRuntimeScript{requireName()});
    } else if (call.name == "startEvent") {
      const std::string &name = requireName();
      if (!impl_->queueStandaloneEvent(name)) warnMissing("event", name);
    } else if (call.name == "startCutScene" ||
               call.name == "runCutScene") {
      const std::string &name = requireName();
      if (!startCutscene(name)) warnMissing("cutscene", name);
    } else if (call.name == "stopCutScene" ||
               call.name == "removeCutScene") {
      impl_->finishCutscene();
    } else if (call.name == "powerComputer") {
      const std::string &name = requireName();
      Impl::Record *computer = impl_->findKind(name, RuntimeEntityKind::Computer);
      if (computer == nullptr) warnMissing("computer", name);
      else impl_->enterComputer(*computer);
    } else if (call.name == "logoffComputer" || call.name == "logoff") {
      static_cast<void>(exitComputer());
    } else if (call.name == "setCameraParent") {
      const std::string &name = requireName();
      const auto transform = impl_->services->runtimeTransform(name);
      if (!transform) warnMissing("player parent", name);
      else {
        impl_->playerParent = name;
        impl_->lastPlayerParentTransform = transform;
      }
    } else if (call.name == "resetCameraParent") {
      impl_->playerParent.clear();
      impl_->lastPlayerParentTransform.reset();
    } else if (call.name == "HUDHide" || call.name == "HUDDisable") {
      impl_->presentationState.hudVisible = false;
      impl_->services->submit(SetRuntimeHudVisible{false});
    } else if (call.name == "HUDShow" || call.name == "HUDEnable") {
      impl_->presentationState.hudVisible = true;
      impl_->services->submit(SetRuntimeHudVisible{true});
    } else if (call.name == "gameText") {
      const std::string text = requireName();
      const double seconds = call.arguments.size() >= 3
          ? argumentNumber(call, 2) : 3.0;
      impl_->services->submit(SetRuntimeSubtitle{text, seconds});
    } else if (call.name == "enableInventory" ||
               call.name == "disableInventory") {
      impl_->services->submit(SetRuntimeInventoryEnabled{
          call.name == "enableInventory"});
    } else if (call.name == "player__allowFlash") {
      const std::string value = requireName();
      impl_->services->submit(SetRuntimeFlashlightAllowed{
          value == "true" || value == "1"});
    } else if (call.name == "getFov") {
      return impl_->services->runtimeFovDegrees();
    } else if (call.name == "setFov") {
      impl_->services->submit(SetRuntimeFov{argumentNumber(call, 0)});
    } else if (call.name == "resetFov") {
      impl_->services->submit(SetRuntimeFov{std::nullopt});
    } else if (call.name == "setCompositorEnabled") {
      if (call.arguments.size() < 2)
        throw std::invalid_argument("expected enabled and compositor name");
      impl_->services->submit(SetRuntimeCompositor{
          call.arguments[1], call.arguments[0] == "true" ||
                                 call.arguments[0] == "1"});
    } else if (call.name == "fragmentGPUProgramParams") {
      if (call.arguments.size() < 3)
        throw std::invalid_argument("expected program, parameter and value");
      impl_->services->submit(SetRuntimeShaderParameter{
          call.arguments[0], call.arguments[1], call.arguments[2]});
    } else if (call.name == "dMaterialSet") {
      if (impl_->activeComputer == nullptr) {
        impl_->services->submit(RuntimeLog{
            "warning: dMaterialSet ignored outside computer focus"});
      } else {
        impl_->activeComputer->displayMaterial = requireName();
        impl_->services->submit(SetComputerPresentation{
            impl_->activeComputer->publicState.handle,
            impl_->activeComputer->displayMaterial, true,
            impl_->activeComputer->allowVirtualDisplay});
      }
    } else if (call.name == "exitAllComputers") {
      static_cast<void>(exitComputer());
    } else if (call.name == "fireFire" ||
               call.name == "fireExtinguish" ||
               call.name == "fireToggle") {
      impl_->services->submit(SetRuntimeEffectEnabled{
          requireName(), call.name == "fireToggle"
                             ? std::nullopt
                             : std::optional<bool>{call.name == "fireFire"}});
    } else if (call.name == "playMusic") {
      const bool loop = call.arguments.size() < 2 ||
                        call.arguments[1] == "true" ||
                        call.arguments[1] == "1";
      impl_->services->submit(PlayRuntimeSound{{}, requireName(), {}, loop, 1.0F});
    } else if (call.name == "toggleMusic") {
      impl_->services->submit(PlayRuntimeSound{{}, requireName(), {}, true, 1.0F});
    } else if (call.name == "stopMusic" || call.name == "fadeoutMusic") {
      impl_->services->submit(StopRuntimeSound{{}});
    } else if (call.name == "setMusicVolume") {
      impl_->services->submit(SetRuntimeMusicGain{
          static_cast<float>(std::max(0.0, argumentNumber(call, 0)))});
    } else if (call.name == "enableAmbientSound" ||
               call.name == "disableAmbientSound") {
      impl_->services->submit(SetRuntimeAmbientEnabled{
          requireName(), call.name == "enableAmbientSound"});
    } else if (call.name == "emitSound" || call.name == "emitSound2") {
      const bool spatial = call.name == "emitSound2";
      if (call.arguments.size() < (spatial ? 5U : 2U)) {
        throw std::invalid_argument("missing effect duration/position arguments");
      }
      const physics::Vec3 position = spatial
          ? physics::Vec3{argumentNumber(call, 2), argumentNumber(call, 3),
                          argumentNumber(call, 4)}
          : physics::Vec3{};
      impl_->services->submit(PlayRuntimeEffect{
          requireName(), position, static_cast<float>(argumentNumber(call, 1)),
          spatial});
    } else if (call.name == "npcEvent" || call.name == "npcEvent2" ||
               call.name == "__all_npcEvent") {
      const std::size_t expected = call.name == "npcEvent2" ? 4U : 3U;
      if (call.arguments.size() != expected)
        throw std::invalid_argument("expected " + std::to_string(expected) +
                                    " NPC event arguments");
      std::size_t used{};
      const int code = std::stoi(call.arguments[1], &used);
      if (used != call.arguments[1].size())
        throw std::invalid_argument("NPC event code is not an integer");
      impl_->services->submit(NpcRuntimeCommand{
          call.arguments[0], code, call.arguments[2],
          expected == 4 ? call.arguments[3] : std::string{},
          call.name == "__all_npcEvent"});
    } else if (call.name == "destroyNPC") {
      impl_->services->submit(DestroyNpcRuntimeCommand{requireName()});
    } else if (call.name == "setNPCManagerStep") {
      impl_->services->submit(SetNpcUpdateInterval{
          std::max(0.0, argumentNumber(call, 0))});
    } else if (call.name == "setSpeedTrain" ||
               call.name == "setRotSpeed") {
      const std::string &name = requireName();
      const double speed = argumentNumber(call, 1);
      Impl::Record *record = call.name == "setSpeedTrain"
          ? impl_->findKind(name, RuntimeEntityKind::Train)
          : impl_->findKind(name, RuntimeEntityKind::Rotator);
      if (record == nullptr && call.name == "setRotSpeed")
        record = impl_->findKind(name, RuntimeEntityKind::Door);
      if (record == nullptr && call.name == "setRotSpeed")
        record = impl_->findKind(name, RuntimeEntityKind::Pendulum);
      if (record == nullptr) warnMissing("movement target", name);
      else record->speed = speed;
    } else if (call.name == "showEntity" || call.name == "hideEntity" ||
               call.name == "toggleEntity") {
      const std::optional<bool> requested =
          call.name == "toggleEntity" ? std::nullopt
          : std::optional<bool>{call.name == "showEntity"};
      Impl::Record *record = impl_->findPresented(requireName());
      if (record != nullptr) {
        record->publicState.visible = requested.value_or(
            !record->publicState.visible);
        impl_->services->submit(SetRuntimeVisible{record->publicState.handle,
                                                   record->publicState.visible});
      } else if (impl_->registry->findFirst(requireName()) ||
                 impl_->authoredTargetExists(requireName())) {
        impl_->services->submit(SetRuntimeNamedVisible{requireName(), requested});
      } else {
        warnMissing("entity", requireName());
      }
    } else if (call.name == "toggleLight" || call.name == "enableLight" ||
               call.name == "disableLight") {
      const std::optional<bool> requested =
          call.name == "toggleLight" ? std::nullopt
          : std::optional<bool>{call.name == "enableLight"};
      impl_->services->submit(SetRuntimeLightVisible{requireName(), requested});
    } else {
      impl_->services->submit(DeferredLegacyCommand{call.name, call.group});
    }
  } catch (const std::exception &error) {
    throw std::runtime_error("Lua command " + call.name + ": " + error.what());
  }
  impl_->refreshPublicStates();
  return {};
}

PersistentSequenceState SequenceRuntime::saveState() const {
  PersistentSequenceState saved;
  saved.mapName = impl_->definition->mapName;
  saved.tick = impl_->tickNumber;
  saved.entities = impl_->publicStates;
  saved.nextQueueOrder = impl_->nextQueueOrder;
  saved.playerParent = impl_->playerParent;
  if (impl_->activeCutscene != nullptr) {
    saved.activeCutscene = impl_->activeCutscene->name;
    saved.cutsceneTick = impl_->activeCutscene->elapsedTicks;
  }
  if (impl_->activeComputer != nullptr)
    saved.activeComputer = impl_->activeComputer->publicState.name;
  for (const Impl::Record &record : impl_->records) {
    saved.internals.push_back({record.nextTick, record.keyPoint, record.phase,
                               record.rotationProgress,
                               record.oneShotFired, record.completionFired,
                               record.reverse});
  }
  for (const Impl::QueuedAction &action : impl_->queue) {
    saved.pending.push_back({action.dueTick, action.order, action.action});
  }
  return saved;
}

void SequenceRuntime::restoreState(const PersistentSequenceState &state) {
  if (!impl_->isStarted || impl_->isUnloaded) {
    throw std::logic_error("restoreState requires an active SequenceRuntime");
  }
  if (state.mapName != impl_->definition->mapName ||
      state.entities.size() != impl_->records.size() ||
      state.internals.size() != impl_->records.size()) {
    throw std::invalid_argument("sequence save does not match loaded map");
  }
  for (std::size_t index = 0; index < state.entities.size(); ++index) {
    Impl::Record &record = impl_->records[index];
    const SequenceEntityState &saved = state.entities[index];
    if (record.publicState.name != saved.name ||
        record.publicState.tag != saved.tag) {
      throw std::invalid_argument("sequence save declaration order does not match map");
    }
  }
  impl_->tickNumber = state.tick;
  impl_->nextQueueOrder = state.nextQueueOrder;
  impl_->playerParent = state.playerParent;
  impl_->lastPlayerParentTransform = state.playerParent.empty()
      ? std::nullopt : impl_->services->runtimeTransform(state.playerParent);
  impl_->queue.clear();
  for (const PersistentSequenceState::PendingAction &action : state.pending) {
    impl_->queue.push_back({action.dueTick, action.order, action.action});
  }
  for (std::size_t index = 0; index < state.entities.size(); ++index) {
    Impl::Record &record = impl_->records[index];
    const EntityHandle current = record.publicState.handle;
    record.publicState = state.entities[index];
    record.publicState.handle = current;
    const auto &internals = state.internals[index];
    record.nextTick = internals.nextTick;
    record.keyPoint = internals.keyPoint;
    record.phase = internals.phase;
    record.rotationProgress = internals.rotationProgress;
    record.oneShotFired = internals.oneShotFired;
    record.completionFired = internals.completionFired;
    record.reverse = internals.reverse;
    if (record.publicState.tag != "timer") {
      impl_->services->submit(SetRuntimeTransform{current,
                                                  record.publicState.transform});
      impl_->services->submit(SetRuntimeVisible{current,
                                                record.publicState.visible});
    }
  }
  impl_->refreshPublicStates();
  impl_->finishCutscene();
  static_cast<void>(impl_->exitComputer());
  if (!state.activeCutscene.empty()) {
    if (!impl_->startCutscene(state.activeCutscene))
      throw std::invalid_argument("sequence save references missing cutscene");
    impl_->activeCutscene->elapsedTicks = state.cutsceneTick;
    impl_->updateCutsceneCamera(*impl_->activeCutscene);
  }
  if (!state.activeComputer.empty()) {
    Impl::Record *computer = impl_->findKind(state.activeComputer,
                                             RuntimeEntityKind::Computer);
    if (computer == nullptr)
      throw std::invalid_argument("sequence save references missing computer");
    impl_->enterComputer(*computer);
  }
}

std::uint64_t SequenceRuntime::tick() const noexcept { return impl_->tickNumber; }
bool SequenceRuntime::started() const noexcept { return impl_->isStarted; }
bool SequenceRuntime::unloaded() const noexcept { return impl_->isUnloaded; }
const std::vector<SequenceEntityState> &SequenceRuntime::states() const noexcept {
  return impl_->publicStates;
}
std::optional<SequenceEntityState> SequenceRuntime::state(std::string_view name) const {
  const Impl::Record *record = impl_->find(name);
  return record == nullptr ? std::nullopt
                           : std::optional<SequenceEntityState>{record->publicState};
}
bool SequenceRuntime::playerOnLadder() const {
  const physics::Vec3 player = impl_->services->playerPosition();
  return std::any_of(impl_->records.begin(), impl_->records.end(),
                     [player](const Impl::Record &record) {
                       return record.kind == RuntimeEntityKind::Ladder &&
                              record.publicState.enabled &&
                              contains(record.publicState.transform.position,
                                       {std::max(25.0, std::abs(record.scale.x)),
                                        std::max(50.0, std::abs(record.scale.y)),
                                        std::max(25.0, std::abs(record.scale.z))},
                                       player);
                     });
}

const SequencePresentationState &
SequenceRuntime::presentation() const noexcept {
  return impl_->presentationState;
}

std::string SequenceRuntime::serializeState() const {
  const PersistentSequenceState saved = saveState();
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "RUN3_SEQUENCE_STATE 2\n" << std::quoted(saved.mapName) << ' '
         << saved.tick << ' ' << saved.nextQueueOrder << '\n'
         << std::quoted(saved.playerParent) << ' '
         << std::quoted(saved.activeCutscene) << ' ' << saved.cutsceneTick << ' '
         << std::quoted(saved.activeComputer) << '\n'
         << saved.entities.size() << '\n';
  for (const auto &entity : saved.entities) {
    output << std::quoted(entity.name) << ' ' << std::quoted(entity.tag) << ' '
           << entity.transform.position.x << ' ' << entity.transform.position.y
           << ' ' << entity.transform.position.z << ' '
           << entity.transform.rotation.w << ' ' << entity.transform.rotation.x
           << ' ' << entity.transform.rotation.y << ' '
           << entity.transform.rotation.z << ' ' << entity.enabled << ' '
           << entity.visible << ' ' << entity.active << ' ' << entity.inside
           << ' ' << entity.activationCount << '\n';
  }
  output << saved.internals.size() << '\n';
  for (const auto &internal : saved.internals)
    output << internal.nextTick << ' ' << internal.keyPoint << ' '
           << internal.phase << ' ' << internal.rotationProgress.x << ' '
           << internal.rotationProgress.y << ' '
           << internal.rotationProgress.z << ' ' << internal.oneShotFired << ' '
           << internal.completionFired << ' ' << internal.reverse << '\n';
  output << saved.pending.size() << '\n';
  for (const auto &pending : saved.pending) {
    output << pending.dueTick << ' ' << pending.order << '\n';
    writeElement(output, pending.action);
  }
  return output.str();
}

void SequenceRuntime::restoreSerializedState(std::string_view state) {
  std::istringstream input{std::string(state)};
  input.imbue(std::locale::classic());
  std::string magic;
  int version{};
  if (!(input >> magic >> version) || magic != "RUN3_SEQUENCE_STATE" ||
      (version != 1 && version != 2))
    throw std::invalid_argument("unsupported sequence state format");
  PersistentSequenceState saved = saveState();
  std::size_t count{};
  if (!(input >> std::quoted(saved.mapName) >> saved.tick >>
        saved.nextQueueOrder >> std::quoted(saved.playerParent) >>
        std::quoted(saved.activeCutscene) >> saved.cutsceneTick >>
        std::quoted(saved.activeComputer) >> count) ||
      count != saved.entities.size())
    throw std::invalid_argument("invalid sequence state header");
  for (std::size_t index = 0; index < count; ++index) {
    auto &entity = saved.entities[index];
    std::string name, tag;
    if (!(input >> std::quoted(name) >> std::quoted(tag) >>
          entity.transform.position.x >> entity.transform.position.y >>
          entity.transform.position.z >> entity.transform.rotation.w >>
          entity.transform.rotation.x >> entity.transform.rotation.y >>
          entity.transform.rotation.z >> entity.enabled >> entity.visible >>
          entity.active >> entity.inside >> entity.activationCount) ||
        name != entity.name || tag != entity.tag)
      throw std::invalid_argument("invalid sequence entity state");
  }
  std::size_t internalCount{};
  if (!(input >> internalCount) || internalCount != saved.internals.size())
    throw std::invalid_argument("invalid sequence internals count");
  for (auto &internal : saved.internals) {
    if (!(input >> internal.nextTick >> internal.keyPoint >> internal.phase))
      throw std::invalid_argument("invalid sequence internals record");
    if (version == 2) {
      if (!(input >> internal.rotationProgress.x >>
            internal.rotationProgress.y >> internal.rotationProgress.z))
        throw std::invalid_argument("invalid sequence rotation state");
    } else {
      internal.rotationProgress = {};
    }
    if (!(input >> internal.oneShotFired >> internal.completionFired >>
          internal.reverse))
      throw std::invalid_argument("invalid sequence internals record");
  }
  std::size_t pendingCount{};
  if (!(input >> pendingCount))
    throw std::invalid_argument("invalid sequence pending count");
  saved.pending.clear();
  for (std::size_t index = 0; index < pendingCount; ++index) {
    PersistentSequenceState::PendingAction pending;
    if (!(input >> pending.dueTick >> pending.order) ||
        !readElement(input, pending.action))
      throw std::invalid_argument("invalid sequence pending action");
    saved.pending.push_back(std::move(pending));
  }
  restoreState(saved);
}

} // namespace run3::gameplay
