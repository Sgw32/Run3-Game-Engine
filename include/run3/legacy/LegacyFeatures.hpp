#pragma once

#include <stdexcept>
#include <string_view>

#ifndef RUN3_LEGACY_ENABLE_NEWTON
#define RUN3_LEGACY_ENABLE_NEWTON 0
#endif
#ifndef RUN3_LEGACY_ENABLE_OIS
#define RUN3_LEGACY_ENABLE_OIS 0
#endif
#ifndef RUN3_LEGACY_ENABLE_CEGUI
#define RUN3_LEGACY_ENABLE_CEGUI 0
#endif
#ifndef RUN3_LEGACY_ENABLE_HYDRAX
#define RUN3_LEGACY_ENABLE_HYDRAX 0
#endif
#ifndef RUN3_LEGACY_ENABLE_SKYX
#define RUN3_LEGACY_ENABLE_SKYX 0
#endif
#ifndef RUN3_LEGACY_ENABLE_LEGACY_AUDIO
#define RUN3_LEGACY_ENABLE_LEGACY_AUDIO 0
#endif
#ifndef RUN3_LEGACY_ENABLE_DIRECTSHOW
#define RUN3_LEGACY_ENABLE_DIRECTSHOW 0
#endif
#ifndef RUN3_LEGACY_ENABLE_SERIAL
#define RUN3_LEGACY_ENABLE_SERIAL 0
#endif
#ifndef RUN3_LEGACY_ENABLE_NAMED_PIPES
#define RUN3_LEGACY_ENABLE_NAMED_PIPES 0
#endif

namespace run3::legacy {

enum class Feature {
    newton,
    ois,
    cegui,
    hydrax,
    skyx,
    legacyAudio,
    directShow,
    serial,
    namedPipes,
};

class FeatureUnavailable final : public std::runtime_error {
public:
    explicit FeatureUnavailable(std::string_view featureName);
};

[[nodiscard]] const char* featureName(Feature feature) noexcept;
[[nodiscard]] bool featureAvailable(Feature feature) noexcept;

// Temporary null-backend boundary. Calling code must use this before a
// required retired subsystem; an unavailable feature logs to stderr and
// throws instead of silently pretending that gameplay work succeeded.
void requireFeature(Feature feature);

} // namespace run3::legacy
