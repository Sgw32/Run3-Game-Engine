#include "run3/legacy/LegacyFeatures.hpp"

#include <cstdio>
#include <string>

namespace run3::legacy {

FeatureUnavailable::FeatureUnavailable(const std::string_view name)
    : std::runtime_error("Run3 legacy feature is unavailable: " +
                         std::string{name}) {}

const char* featureName(const Feature feature) noexcept {
    switch (feature) {
    case Feature::newton:
        return "Newton/OgreNewt";
    case Feature::ois:
        return "OIS";
    case Feature::cegui:
        return "CEGUI";
    case Feature::hydrax:
        return "Hydrax";
    case Feature::skyx:
        return "SkyX";
    case Feature::legacyAudio:
        return "Audiere/ALUT";
    case Feature::directShow:
        return "DirectShow";
    case Feature::serial:
        return "serial device";
    case Feature::namedPipes:
        return "named pipes";
    }
    return "unknown";
}

bool featureAvailable(const Feature feature) noexcept {
    switch (feature) {
    case Feature::newton:
        return RUN3_LEGACY_ENABLE_NEWTON != 0;
    case Feature::ois:
        return RUN3_LEGACY_ENABLE_OIS != 0;
    case Feature::cegui:
        return RUN3_LEGACY_ENABLE_CEGUI != 0;
    case Feature::hydrax:
        return RUN3_LEGACY_ENABLE_HYDRAX != 0;
    case Feature::skyx:
        return RUN3_LEGACY_ENABLE_SKYX != 0;
    case Feature::legacyAudio:
        return RUN3_LEGACY_ENABLE_LEGACY_AUDIO != 0;
    case Feature::directShow:
        return RUN3_LEGACY_ENABLE_DIRECTSHOW != 0;
    case Feature::serial:
        return RUN3_LEGACY_ENABLE_SERIAL != 0;
    case Feature::namedPipes:
        return RUN3_LEGACY_ENABLE_NAMED_PIPES != 0;
    }
    return false;
}

void requireFeature(const Feature feature) {
    if (featureAvailable(feature)) {
        return;
    }

    const auto* const name = featureName(feature);
    std::fprintf(stderr, "Run3 legacy null backend called: %s\n", name);
    throw FeatureUnavailable{name};
}

} // namespace run3::legacy
