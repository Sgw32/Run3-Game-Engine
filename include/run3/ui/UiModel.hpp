#pragma once

#include <run3/ui/Ui.hpp>

#include <array>
#include <unordered_map>

namespace run3::ui {

struct WidgetRecord {
  WidgetHandle handle;
  WidgetHandle parent;
  WidgetType type{WidgetType::Panel};
  std::string name;
};

// Backend-independent authority for names, handle generations and callbacks.
// Renderer adapters may associate their non-owning widget pointer by handle id,
// but callers can never obtain it through this API.
class UiRegistry final {
public:
  UiRegistry();

  void beginContext(Context context, std::string owner = {});
  void clearContext(Context context) noexcept;
  [[nodiscard]] WidgetHandle add(const WidgetSpec &spec);
  [[nodiscard]] const WidgetRecord &require(WidgetHandle handle) const;
  [[nodiscard]] std::optional<WidgetHandle> find(Context context,
                                                 std::string_view name) const;
  [[nodiscard]] std::vector<WidgetHandle> erase(WidgetHandle handle);

  [[nodiscard]] std::uint64_t bind(WidgetHandle handle, UiEvent event,
                                   UiCallback callback);
  void unbind(std::uint64_t id) noexcept;
  void clearCallbacks() noexcept;
  void emit(WidgetHandle handle, UiEvent event, std::string value = {});

  [[nodiscard]] bool contextActive(Context context) const noexcept;
  [[nodiscard]] const std::string &owner(Context context) const;
  [[nodiscard]] std::size_t widgetCount(Context context) const noexcept;
  [[nodiscard]] std::size_t callbackCount() const noexcept;

private:
  struct ContextState {
    std::uint32_t generation{};
    bool active{};
    std::string owner;
  };
  struct CallbackRecord {
    WidgetHandle handle;
    UiEvent event{UiEvent::Click};
    UiCallback callback;
  };

  [[nodiscard]] static std::size_t index(Context context) noexcept;
  [[nodiscard]] bool current(WidgetHandle handle) const noexcept;

  std::array<ContextState, 3> contexts_{};
  std::unordered_map<std::uint64_t, WidgetRecord> widgets_;
  std::unordered_map<std::uint64_t, CallbackRecord> callbacks_;
  std::uint64_t nextWidget_{1};
  std::uint64_t nextCallback_{1};
};

struct ScaledLayout {
  float scale{1.0F};
  float offsetX{};
  float offsetY{};
  unsigned logicalWidth{1280};
  unsigned logicalHeight{720};
};

[[nodiscard]] ScaledLayout calculateLayout(unsigned width, unsigned height,
                                            float dpiScale);

} // namespace run3::ui
