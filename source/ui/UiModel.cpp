#include <run3/ui/UiModel.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace run3::ui {
namespace {

std::string lower(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

} // namespace

std::string_view contextName(const Context context) noexcept {
  switch (context) {
  case Context::Main: return "main";
  case Context::Hud: return "hud";
  case Context::Computer: return "computer";
  }
  return "main";
}

std::optional<Context> parseContext(const std::string_view name) noexcept {
  const std::string value = lower(name);
  if (value == "main") return Context::Main;
  if (value == "hud") return Context::Hud;
  if (value == "computer") return Context::Computer;
  return std::nullopt;
}

std::string_view widgetTypeName(const WidgetType type) noexcept {
  switch (type) {
  case WidgetType::Panel: return "panel";
  case WidgetType::Text: return "text";
  case WidgetType::Button: return "button";
  case WidgetType::Edit: return "edit";
  case WidgetType::CheckBox: return "checkbox";
  case WidgetType::List: return "list";
  }
  return "panel";
}

std::optional<WidgetType> parseWidgetType(const std::string_view name) noexcept {
  const std::string value = lower(name);
  if (value == "panel" || value == "window") return WidgetType::Panel;
  if (value == "text" || value == "label") return WidgetType::Text;
  if (value == "button") return WidgetType::Button;
  if (value == "edit" || value == "editbox") return WidgetType::Edit;
  if (value == "checkbox" || value == "check") return WidgetType::CheckBox;
  if (value == "list" || value == "listbox") return WidgetType::List;
  return std::nullopt;
}

std::optional<UiEvent> parseUiEvent(const std::string_view name) noexcept {
  const std::string value = lower(name);
  if (value == "click") return UiEvent::Click;
  if (value == "change" || value == "changed") return UiEvent::Change;
  if (value == "submit" || value == "accept") return UiEvent::Submit;
  return std::nullopt;
}

std::string WidgetHandle::token() const {
  if (!valid()) return {};
  return "run3ui:" + std::string(contextName(context)) + ":" +
         std::to_string(id) + ":" + std::to_string(generation);
}

std::optional<WidgetHandle> WidgetHandle::parse(const std::string_view token) {
  constexpr std::string_view prefix = "run3ui:";
  if (token.substr(0, prefix.size()) != prefix) return std::nullopt;
  const std::size_t contextEnd = token.find(':', prefix.size());
  const std::size_t idEnd = contextEnd == std::string_view::npos
                                ? contextEnd
                                : token.find(':', contextEnd + 1);
  if (contextEnd == std::string_view::npos || idEnd == std::string_view::npos)
    return std::nullopt;
  const auto context = parseContext(token.substr(prefix.size(),
                                                  contextEnd - prefix.size()));
  if (!context) return std::nullopt;
  WidgetHandle result;
  result.context = *context;
  const auto idResult = std::from_chars(token.data() + contextEnd + 1,
                                        token.data() + idEnd, result.id);
  const auto generationResult = std::from_chars(
      token.data() + idEnd + 1, token.data() + token.size(), result.generation);
  if (idResult.ec != std::errc{} || idResult.ptr != token.data() + idEnd ||
      generationResult.ec != std::errc{} ||
      generationResult.ptr != token.data() + token.size() || !result.valid())
    return std::nullopt;
  return result;
}

std::size_t UiRegistry::index(const Context context) noexcept {
  return static_cast<std::size_t>(context);
}

UiRegistry::UiRegistry() {
  beginContext(Context::Main, "main-window");
  beginContext(Context::Hud, "main-viewport");
}

void UiRegistry::beginContext(const Context context, std::string ownerValue) {
  clearContext(context);
  auto &state = contexts_[index(context)];
  ++state.generation;
  if (state.generation == 0) ++state.generation;
  state.active = true;
  state.owner = std::move(ownerValue);
}

void UiRegistry::clearContext(const Context context) noexcept {
  const auto matches = [context](const auto &item) {
    return item.second.handle.context == context;
  };
  for (auto iterator = callbacks_.begin(); iterator != callbacks_.end();) {
    iterator = matches(*iterator) ? callbacks_.erase(iterator) : std::next(iterator);
  }
  for (auto iterator = widgets_.begin(); iterator != widgets_.end();) {
    iterator = iterator->second.handle.context == context
                   ? widgets_.erase(iterator)
                   : std::next(iterator);
  }
  auto &state = contexts_[index(context)];
  state.active = false;
  state.owner.clear();
}

bool UiRegistry::current(const WidgetHandle handle) const noexcept {
  if (!handle.valid()) return false;
  const auto &state = contexts_[index(handle.context)];
  return state.active && state.generation == handle.generation;
}

WidgetHandle UiRegistry::add(const WidgetSpec &spec) {
  auto &state = contexts_[index(spec.context)];
  if (!state.active) throw std::logic_error("UI context is not active");
  if (spec.name.empty()) throw std::invalid_argument("UI widget name is empty");
  if (find(spec.context, spec.name))
    throw std::invalid_argument("duplicate UI widget name '" + spec.name + "'");
  if (spec.parent.valid()) {
    const WidgetRecord &parent = require(spec.parent);
    if (parent.handle.context != spec.context)
      throw std::invalid_argument("UI parent belongs to another context");
  }
  const WidgetHandle handle{spec.context, nextWidget_++, state.generation};
  widgets_.emplace(handle.id,
                   WidgetRecord{handle, spec.parent, spec.type, spec.name});
  return handle;
}

const WidgetRecord &UiRegistry::require(const WidgetHandle handle) const {
  const auto found = widgets_.find(handle.id);
  if (!current(handle) || found == widgets_.end() ||
      !(found->second.handle == handle))
    throw std::invalid_argument("stale or unknown UI handle '" + handle.token() + "'");
  return found->second;
}

std::optional<WidgetHandle> UiRegistry::find(const Context context,
                                             const std::string_view name) const {
  const auto found = std::find_if(widgets_.begin(), widgets_.end(),
      [context, name](const auto &item) {
        return item.second.handle.context == context && item.second.name == name;
      });
  return found == widgets_.end() ? std::nullopt
                                  : std::optional<WidgetHandle>(found->second.handle);
}

std::vector<WidgetHandle> UiRegistry::erase(const WidgetHandle handle) {
  static_cast<void>(require(handle));
  std::vector<WidgetHandle> removed;
  bool progress = true;
  while (progress) {
    progress = false;
    for (auto iterator = widgets_.begin(); iterator != widgets_.end();) {
      const bool root = iterator->second.handle == handle;
      const bool child = std::find(removed.begin(), removed.end(),
                                   iterator->second.parent) != removed.end();
      if (root || child) {
        removed.push_back(iterator->second.handle);
        iterator = widgets_.erase(iterator);
        progress = true;
      } else {
        ++iterator;
      }
    }
  }
  for (auto iterator = callbacks_.begin(); iterator != callbacks_.end();) {
    iterator = std::find(removed.begin(), removed.end(), iterator->second.handle) !=
                       removed.end()
                   ? callbacks_.erase(iterator)
                   : std::next(iterator);
  }
  return removed;
}

std::uint64_t UiRegistry::bind(const WidgetHandle handle, const UiEvent event,
                               UiCallback callback) {
  static_cast<void>(require(handle));
  if (!callback) throw std::invalid_argument("UI callback is empty");
  const std::uint64_t id = nextCallback_++;
  callbacks_.emplace(id, CallbackRecord{handle, event, std::move(callback)});
  return id;
}

void UiRegistry::unbind(const std::uint64_t id) noexcept { callbacks_.erase(id); }
void UiRegistry::clearCallbacks() noexcept { callbacks_.clear(); }

void UiRegistry::emit(const WidgetHandle handle, const UiEvent event,
                      std::string value) {
  static_cast<void>(require(handle));
  std::vector<UiCallback> pending;
  for (const auto &[id, record] : callbacks_) {
    static_cast<void>(id);
    if (record.handle == handle && record.event == event)
      pending.push_back(record.callback);
  }
  for (auto &callback : pending) callback(value);
}

bool UiRegistry::contextActive(const Context context) const noexcept {
  return contexts_[index(context)].active;
}
const std::string &UiRegistry::owner(const Context context) const {
  return contexts_[index(context)].owner;
}
std::size_t UiRegistry::widgetCount(const Context context) const noexcept {
  return static_cast<std::size_t>(std::count_if(
      widgets_.begin(), widgets_.end(), [context](const auto &item) {
        return item.second.handle.context == context;
      }));
}
std::size_t UiRegistry::callbackCount() const noexcept { return callbacks_.size(); }

ScaledLayout calculateLayout(const unsigned width, const unsigned height,
                             const float dpiScale) {
  if (width == 0 || height == 0 || !std::isfinite(dpiScale) || dpiScale <= 0.0F)
    throw std::invalid_argument("invalid UI viewport or DPI scale");
  constexpr float logicalWidth = 1280.0F;
  constexpr float logicalHeight = 720.0F;
  const float availableWidth = static_cast<float>(width) / dpiScale;
  const float availableHeight = static_cast<float>(height) / dpiScale;
  const float scale = std::min(availableWidth / logicalWidth,
                               availableHeight / logicalHeight) * dpiScale;
  return {scale,
          (static_cast<float>(width) - logicalWidth * scale) * 0.5F,
          (static_cast<float>(height) - logicalHeight * scale) * 0.5F,
          static_cast<unsigned>(logicalWidth),
          static_cast<unsigned>(logicalHeight)};
}

} // namespace run3::ui
