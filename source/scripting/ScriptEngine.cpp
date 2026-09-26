#include <run3/scripting/ScriptEngine.hpp>

#include <run3/ui/Ui.hpp>

#include <lua.hpp>
#include <sol/sol.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <utility>
#include <unordered_map>

namespace run3::scripting {
namespace fs = std::filesystem;

namespace {

std::string errorMessage(const fs::path &path, const std::string &cause) {
  return path.generic_string() + ": " + cause;
}

std::string readText(const fs::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw ScriptError(path, "cannot open script");
  }
  return {std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>()};
}

fs::path normalizeRoot(const fs::path &root, const char *name) {
  if (root.empty()) {
    throw std::invalid_argument(std::string(name) + " must not be empty");
  }
  std::error_code error;
  fs::path normalized = fs::weakly_canonical(fs::absolute(root), error);
  if (error) {
    normalized = fs::absolute(root).lexically_normal();
  }
  return normalized;
}

bool within(const fs::path &path, const fs::path &root) {
  const fs::path relative = path.lexically_relative(root);
  if (relative.empty()) {
    return path == root;
  }
  return !relative.is_absolute() && *relative.begin() != "..";
}

std::string applyUnknownEscapeSCompatibility(const std::string &source,
                                             bool &changed) {
  std::string result;
  result.reserve(source.size());
  for (std::size_t index = 0; index < source.size();) {
    if (source[index] != '\\') {
      result += source[index++];
      continue;
    }
    const std::size_t runStart = index;
    while (index < source.size() && source[index] == '\\') {
      ++index;
    }
    const std::size_t slashCount = index - runStart;
    if (index < source.size() && source[index] == 's' &&
        slashCount % 2 == 1) {
      result.append(slashCount - 1, '\\');
      result += 's';
      ++index;
      changed = true;
      continue;
    }
    result.append(slashCount, '\\');
  }
  return result;
}

std::string argumentString(const sol::object &value) {
  switch (value.get_type()) {
  case sol::type::nil: return "nil";
  case sol::type::boolean: return value.as<bool>() ? "true" : "false";
  case sol::type::number: {
    std::ostringstream output;
    output.precision(17);
    output << value.as<double>();
    return output.str();
  }
  case sol::type::string: return value.as<std::string>();
  case sol::type::table: return "<table>";
  case sol::type::function: return "<function>";
  case sol::type::userdata: return "<userdata>";
  case sol::type::lightuserdata: return "<lightuserdata>";
  case sol::type::thread: return "<thread>";
  case sol::type::poly: return "<value>";
  case sol::type::none: return "<none>";
  }
  return "<value>";
}

sol::object toObject(sol::this_state state, const ScriptValue &value) {
  return std::visit(
      [state](const auto &item) -> sol::object {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return sol::make_object(state, sol::nil);
        } else {
          return sol::make_object(state, item);
        }
      },
      value);
}

void instructionHook(lua_State *state, lua_Debug *) {
  constexpr const char *budgetKey = "run3.instruction-budget";
  lua_getfield(state, LUA_REGISTRYINDEX, budgetKey);
  lua_Integer remaining = lua_tointeger(state, -1);
  lua_pop(state, 1);
  remaining -= lua_gethookcount(state);
  lua_pushinteger(state, remaining);
  lua_setfield(state, LUA_REGISTRYINDEX, budgetKey);
  if (remaining <= 0) {
    luaL_error(state, "Run3 script instruction budget exceeded");
  }
}

} // namespace

ScriptError::ScriptError(fs::path path, std::string cause)
    : std::runtime_error(errorMessage(path, cause)), path_(std::move(path)),
      cause_(std::move(cause)) {}

const std::vector<BindingSpec> &legacyBindingCatalog() {
  static const std::vector<BindingSpec> bindings{
#define RUN3_SCRIPT_BINDING(group, name, callback, source, line)              \
  BindingSpec{group, name, "int(lua_State*)", callback, source, line},
#include "LegacyBindings.inc"
#undef RUN3_SCRIPT_BINDING
  };
  return bindings;
}

std::string exportedApiSnapshot() {
  std::ostringstream output;
  const auto &bindings = legacyBindingCatalog();
  output << "# Run3 Lua exported API v1\ncount=" << bindings.size() << '\n';
  for (const BindingSpec &binding : bindings) {
    output << binding.group << " | " << binding.name << " | "
           << binding.signature << " | " << binding.legacyCallback << '\n';
  }
  return output.str();
}

std::string myGuiApiSnapshot() {
  static constexpr std::pair<std::string_view, std::string_view> functions[]{
      {"load_layout", "(scope:string, key:string) -> WidgetHandle"},
      {"create", "(scope:string, parent:WidgetHandle|nil, type:string, name:string, x:number, y:number, width:number, height:number) -> WidgetHandle"},
      {"find", "(scope:string, name:string) -> WidgetHandle|nil"},
      {"destroy", "(handle:WidgetHandle)"},
      {"set_text", "(handle:WidgetHandle, text:string)"},
      {"set_visible", "(handle:WidgetHandle, visible:boolean)"},
      {"set_enabled", "(handle:WidgetHandle, enabled:boolean)"},
      {"set_property", "(handle:WidgetHandle, property:string, value:string)"},
      {"focus", "(handle:WidgetHandle)"},
      {"on", "(handle:WidgetHandle, event:click|change|submit, callback:function) -> CallbackHandle"},
      {"clear_callback", "(callback:CallbackHandle)"}};
  std::ostringstream output;
  output << "# Run3 typed MyGUI facade v1\ncount=" << std::size(functions) << '\n';
  for (const auto &[name, signature] : functions)
    output << "mygui." << name << " | " << signature << '\n';
  return output.str();
}

std::string luaRuntimeVersion() {
  return std::string(LUA_VERSION_MAJOR) + "." + LUA_VERSION_MINOR + "." +
         LUA_VERSION_RELEASE;
}

class ScriptEngine::Impl final {
public:
  Impl(ScriptEngineConfig config, ScriptDispatch dispatch)
      : config_(std::move(config)), dispatch_(std::move(dispatch)) {
    config_.contentRoot = normalizeRoot(config_.contentRoot, "contentRoot");
    config_.userRoot = normalizeRoot(config_.userRoot, "userRoot");
    if (config_.instructionBudget == 0) {
      throw std::invalid_argument("instructionBudget must be greater than zero");
    }

    lua_.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string,
                        sol::lib::math, sol::lib::table, sol::lib::utf8,
                        sol::lib::debug);
    const sol::protected_function_result tracebackResult = lua_.safe_script(R"lua(
      local traceback = debug.traceback
      return function(message)
        return traceback(tostring(message), 2)
      end
    )lua", sol::script_pass_on_error);
    if (!tracebackResult.valid()) {
      const sol::error error = tracebackResult;
      throw std::runtime_error("Could not initialize Lua traceback: " +
                               std::string(error.what()));
    }
    traceback_ = tracebackResult.get<sol::protected_function>();
    lua_["debug"] = sol::nil;
    lua_["io"] = sol::nil;
    lua_["os"] = sol::nil;
    lua_["package"] = sol::nil;
    lua_["dofile"] = sol::nil;
    lua_["loadfile"] = sol::nil;

    for (const BindingSpec &binding : legacyBindingCatalog()) {
      lua_.set_function(
          binding.name,
          [this, binding](sol::variadic_args arguments,
                          sol::this_state state) -> sol::object {
            ScriptCall call{binding.group, binding.name, {}};
            call.arguments.reserve(arguments.size());
            for (const sol::object argument : arguments) {
              call.arguments.push_back(argumentString(argument));
            }
            calls_.push_back(call);
            if (config_.uiFacade != nullptr) {
              if (const auto result = dispatchButtonGuiCompatibility(calls_.back()))
                return toObject(state, *result);
            }
            if (dispatch_) {
              return toObject(state, dispatch_(calls_.back()));
            }
            return sol::make_object(state, sol::nil);
          });
    }
    registerMyGuiFacade();
  }

  ~Impl() {
    if (config_.uiFacade != nullptr) {
      for (const auto &[scriptId, facadeId] : uiCallbackIds_) {
        static_cast<void>(scriptId);
        config_.uiFacade->clearCallback(facadeId);
      }
      for (const auto &[handle, callback] : legacyButtons_) {
        static_cast<void>(handle);
        if (callback != 0) config_.uiFacade->clearCallback(callback);
      }
    }
  }

  ui::IScriptUiFacade &uiFacade() const {
    if (config_.uiFacade == nullptr)
      throw std::runtime_error("MyGUI facade is unavailable in this runtime");
    return *config_.uiFacade;
  }

  static ui::Context uiContext(const std::string &name) {
    const auto value = ui::parseContext(name);
    if (!value) throw std::invalid_argument("unknown UI scope '" + name + "'");
    return *value;
  }

  static ui::WidgetHandle uiHandle(const std::string &token) {
    const auto value = ui::WidgetHandle::parse(token);
    if (!value) throw std::invalid_argument("invalid UI handle '" + token + "'");
    return *value;
  }

  void registerMyGuiFacade() {
    sol::table facade = lua_.create_named_table("mygui");
    facade.set_function("load_layout", [this](const std::string &scope,
                                               const std::string &key) {
      return uiFacade().loadLayout(uiContext(scope), key).token();
    });
    facade.set_function(
        "create", [this](const std::string &scope, const sol::object &parent,
                          const std::string &type, const std::string &name,
                          double x, double y, double width, double height) {
          const auto widgetType = ui::parseWidgetType(type);
          if (!widgetType)
            throw std::invalid_argument("unknown UI widget type '" + type + "'");
          ui::WidgetHandle parentHandle;
          if (parent.valid() && parent.get_type() != sol::type::nil)
            parentHandle = uiHandle(parent.as<std::string>());
          return uiFacade()
              .createWidget({uiContext(scope), parentHandle, *widgetType, name,
                             {static_cast<float>(x), static_cast<float>(y),
                              static_cast<float>(width), static_cast<float>(height)},
                             {}})
              .token();
        });
    facade.set_function("find", [this](const std::string &scope,
                                        const std::string &name,
                                        sol::this_state state) -> sol::object {
      const auto handle = uiFacade().findWidget(uiContext(scope), name);
      return handle ? sol::make_object(state, handle->token())
                    : sol::make_object(state, sol::nil);
    });
    facade.set_function("destroy", [this](const std::string &handle) {
      uiFacade().destroyWidget(uiHandle(handle));
    });
    facade.set_function("set_text", [this](const std::string &handle,
                                            std::string text) {
      uiFacade().setText(uiHandle(handle), std::move(text));
    });
    facade.set_function("set_visible", [this](const std::string &handle,
                                               bool visible) {
      uiFacade().setVisible(uiHandle(handle), visible);
    });
    facade.set_function("set_enabled", [this](const std::string &handle,
                                               bool enabled) {
      uiFacade().setEnabled(uiHandle(handle), enabled);
    });
    facade.set_function("set_property", [this](const std::string &handle,
                                                const std::string &property,
                                                std::string value) {
      uiFacade().setProperty(uiHandle(handle), property, std::move(value));
    });
    facade.set_function("focus", [this](const std::string &handle) {
      uiFacade().focus(uiHandle(handle));
    });
    facade.set_function("on", [this](const std::string &handle,
                                      const std::string &event,
                                      sol::protected_function callback) {
      const auto parsedEvent = ui::parseUiEvent(event);
      if (!parsedEvent)
        throw std::invalid_argument("unknown UI callback event '" + event + "'");
      const std::uint64_t scriptId = nextUiCallbackId_++;
      callback.set_error_handler(traceback_);
      uiCallbacks_.emplace(scriptId, std::move(callback));
      const std::uint64_t facadeId = uiFacade().setCallback(
          uiHandle(handle), *parsedEvent,
          [this, scriptId](std::string value) {
            invokeUiCallback(scriptId, std::move(value));
          });
      uiCallbackIds_.emplace(scriptId, facadeId);
      return scriptId;
    });
    facade.set_function("clear_callback", [this](std::uint64_t scriptId) {
      const auto found = uiCallbackIds_.find(scriptId);
      if (found == uiCallbackIds_.end()) return;
      uiFacade().clearCallback(found->second);
      uiCallbackIds_.erase(found);
      uiCallbacks_.erase(scriptId);
    });
  }

  static std::pair<float, float> legacyPair(const std::string &value) {
    std::istringstream input(value);
    float first{};
    float second{};
    if (!(input >> first >> second))
      throw std::invalid_argument("buttonGUI expected a numeric pair, got '" +
                                  value + "'");
    return {first, second};
  }

  std::optional<ScriptValue>
  dispatchButtonGuiCompatibility(const ScriptCall &call) {
    if (call.name.rfind("buttonGUI_", 0) != 0) return std::nullopt;
    auto &facade = uiFacade();
    if (call.name == "buttonGUI_activateCenter640") {
      legacyButtonTransform_ = 1;
      return ScriptValue{};
    }
    if (call.name == "buttonGUI_activateTopLeft640") {
      legacyButtonTransform_ = 2;
      return ScriptValue{};
    }
    if (call.name == "buttonGUI_activateTopLeftComp640") {
      legacyButtonTransform_ = 3;
      return ScriptValue{};
    }
    if (call.name == "buttonGUI_deactivateCenter640" ||
        call.name == "buttonGUI_deactivate640") {
      legacyButtonTransform_ = 0;
      return ScriptValue{};
    }
    if (call.name == "buttonGUI_deleteAllButtons") {
      for (const auto &[handle, callback] : legacyButtons_) {
        facade.clearCallback(callback);
        try { facade.destroyWidget(handle); } catch (...) {}
      }
      legacyButtons_.clear();
      return ScriptValue{};
    }
    if (call.name == "buttonGUI_hideCursor" ||
        call.name == "buttonGUI_showCursor") return ScriptValue{};
    if (call.name == "buttonGUI_getCursX" ||
        call.name == "buttonGUI_getCursY" || call.name == "buttonGUI_getX" ||
        call.name == "buttonGUI_getY") return ScriptValue{0.0};
    if (call.name == "buttonGUI_setPos") {
      if (call.arguments.size() != 3) return ScriptValue{};
      const auto handle = facade.findWidget(ui::Context::Computer,
                                             "buttonGUI." + call.arguments[0]);
      if (handle)
        facade.setProperty(*handle, "Position",
                           call.arguments[1] + " " + call.arguments[2]);
      return ScriptValue{};
    }
    if (call.name != "buttonGUI_createButton" &&
        call.name != "buttonGUI_createButtonS" &&
        call.name != "buttonGUI_createDummy") return std::nullopt;
    if (call.arguments.size() != 5)
      throw std::invalid_argument(call.name + " expects five string arguments");
    const std::string safeName = "buttonGUI." + call.arguments[0];
    if (const auto existing = facade.findWidget(ui::Context::Computer, safeName)) {
      facade.setVisible(*existing, true);
      return ScriptValue{existing->token()};
    }
    auto [x, y] = legacyPair(call.arguments[2]);
    auto [width, height] = legacyPair(call.arguments[3]);
    if (legacyButtonTransform_ == 3) {
      x *= 1024.0F / 640.0F;
      y *= 768.0F / 480.0F;
      width *= 1024.0F / 640.0F;
      height *= 768.0F / 480.0F;
    } else if (legacyButtonTransform_ == 1) {
      x += (1024.0F - 640.0F) * 0.5F;
      y += (768.0F - 480.0F) * 0.5F;
    }
    const ui::WidgetHandle root =
        facade.loadLayout(ui::Context::Computer, "default");
    const ui::WidgetType type = call.name == "buttonGUI_createDummy"
                                    ? ui::WidgetType::Panel
                                    : ui::WidgetType::Button;
    const ui::WidgetHandle handle = facade.createWidget(
        {ui::Context::Computer, root, type, safeName,
         {x, y, width, height}, call.arguments[0]});
    std::uint64_t callback{};
    if (type == ui::WidgetType::Button && !call.arguments[4].empty()) {
      const fs::path script = call.arguments[4];
      callback = facade.setCallback(
          handle, ui::UiEvent::Click,
          [this, script](std::string) {
            const fs::path approved = approvedPath(script);
            auto [loaded, shims] =
                load(readText(approved), approved.generic_string());
            static_cast<void>(shims);
            executeLoaded(std::move(loaded), approved);
          });
    }
    legacyButtons_.emplace_back(handle, callback);
    return ScriptValue{handle.token()};
  }

  void invokeUiCallback(const std::uint64_t id, std::string value) {
    const auto found = uiCallbacks_.find(id);
    if (found == uiCallbacks_.end()) return;
    lua_State *state = lua_.lua_state();
    const lua_Integer capped = static_cast<lua_Integer>(std::min<std::size_t>(
        config_.instructionBudget,
        static_cast<std::size_t>(std::numeric_limits<lua_Integer>::max())));
    lua_pushinteger(state, capped);
    lua_setfield(state, LUA_REGISTRYINDEX, "run3.instruction-budget");
    lua_sethook(state, instructionHook, LUA_MASKCOUNT,
                static_cast<int>(std::min<lua_Integer>(1000, capped)));
    const sol::protected_function_result result = found->second(std::move(value));
    lua_sethook(state, nullptr, 0, 0);
    if (!result.valid()) {
      const sol::error error = result;
      throw ScriptError("<mygui-callback>", error.what());
    }
  }

  fs::path approvedPath(const fs::path &requested) const {
    std::error_code error;
    fs::path candidate = requested;
    if (!candidate.is_absolute()) {
      candidate = config_.contentRoot / candidate;
    }
    candidate = fs::weakly_canonical(fs::absolute(candidate), error);
    if (error) {
      candidate = fs::absolute(candidate).lexically_normal();
    }
    if (!within(candidate, config_.contentRoot) &&
        !within(candidate, config_.userRoot)) {
      throw ScriptError(requested,
                        "sandbox rejected path outside content/user roots");
    }
    return candidate;
  }

  std::pair<sol::load_result, std::vector<std::string>>
  load(std::string source, const std::string &chunkName) {
    sol::load_result loaded =
        lua_.load(source, "@" + chunkName, sol::load_mode::text);
    std::vector<std::string> shims;
    if (loaded.valid()) {
      return {std::move(loaded), std::move(shims)};
    }
    const sol::error originalError = loaded;
    const std::string cause = originalError.what();
    bool changed = false;
    std::string compatible = applyUnknownEscapeSCompatibility(source, changed);
    if (!changed || (cause.find("invalid escape sequence") == std::string::npos &&
                     cause.find("invalid escape") == std::string::npos)) {
      throw ScriptError(chunkName, cause);
    }
    loaded = lua_.load(compatible, "@" + chunkName, sol::load_mode::text);
    if (!loaded.valid()) {
      const sol::error compatibilityError = loaded;
      throw ScriptError(chunkName, compatibilityError.what());
    }
    shims.emplace_back("lua50-unknown-escape-s");
    return {std::move(loaded), std::move(shims)};
  }

  void executeLoaded(sol::load_result loaded, const fs::path &path) {
    sol::protected_function function = loaded;
    function.set_error_handler(traceback_);
    const lua_Integer capped = static_cast<lua_Integer>(std::min<std::size_t>(
        config_.instructionBudget,
        static_cast<std::size_t>(std::numeric_limits<lua_Integer>::max())));
    lua_State *state = lua_.lua_state();
    lua_pushinteger(state, capped);
    lua_setfield(state, LUA_REGISTRYINDEX, "run3.instruction-budget");
    const int hookStep = static_cast<int>(
        std::min<lua_Integer>(1000, std::max<lua_Integer>(1, capped)));
    lua_sethook(state, instructionHook, LUA_MASKCOUNT, hookStep);
    const sol::protected_function_result result = function();
    lua_sethook(state, nullptr, 0, 0);
    if (!result.valid()) {
      const sol::error error = result;
      throw ScriptError(path, error.what());
    }
  }

  ScriptEngineConfig config_;
  ScriptDispatch dispatch_;
  sol::state lua_;
  sol::protected_function traceback_;
  std::vector<ScriptCall> calls_;
  std::unordered_map<std::uint64_t, sol::protected_function> uiCallbacks_;
  std::unordered_map<std::uint64_t, std::uint64_t> uiCallbackIds_;
  std::uint64_t nextUiCallbackId_{1};
  std::vector<std::pair<ui::WidgetHandle, std::uint64_t>> legacyButtons_;
  int legacyButtonTransform_{};

  [[nodiscard]] const std::vector<ScriptCall> &calls() const noexcept {
    return calls_;
  }
  void clearCalls() noexcept { calls_.clear(); }
};

ScriptEngine::ScriptEngine(ScriptEngineConfig config, ScriptDispatch dispatch)
    : impl_(std::make_unique<Impl>(std::move(config), std::move(dispatch))) {}

ScriptEngine::~ScriptEngine() = default;
ScriptEngine::ScriptEngine(ScriptEngine &&) noexcept = default;
ScriptEngine &ScriptEngine::operator=(ScriptEngine &&) noexcept = default;

ScriptCheckResult ScriptEngine::checkFile(const fs::path &path) {
  const fs::path approved = impl_->approvedPath(path);
  auto [loaded, shims] = impl_->load(readText(approved), approved.generic_string());
  (void)loaded;
  return {approved, std::move(shims)};
}

void ScriptEngine::executeFile(const fs::path &path) {
  const fs::path approved = impl_->approvedPath(path);
  auto [loaded, shims] = impl_->load(readText(approved), approved.generic_string());
  (void)shims;
  impl_->executeLoaded(std::move(loaded), approved);
}

void ScriptEngine::executeText(std::string source, std::string virtualName) {
  if (virtualName.empty()) {
    virtualName = "<memory>";
  }
  auto [loaded, shims] = impl_->load(std::move(source), virtualName);
  (void)shims;
  impl_->executeLoaded(std::move(loaded), virtualName);
}

const std::vector<ScriptCall> &ScriptEngine::calls() const noexcept {
  return impl_->calls();
}

void ScriptEngine::clearCalls() noexcept { impl_->clearCalls(); }

} // namespace run3::scripting
