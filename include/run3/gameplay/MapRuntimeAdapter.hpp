#pragma once

#include <run3/content/MapDefinition.hpp>

#include <string_view>
#include <vector>

namespace run3::gameplay {

// The legacy DotScene loader instantiated renderables only while an exact
// <node> was active.  In particular, <nodev> blocks are authored as inactive
// alternatives and must not attach any descendants to the runtime scene.
[[nodiscard]] bool isMapRenderableTag(std::string_view tag) noexcept;

[[nodiscard]] std::vector<const content::AuthoredElement *>
activeMapRenderables(const content::MapDefinition &definition);

} // namespace run3::gameplay
