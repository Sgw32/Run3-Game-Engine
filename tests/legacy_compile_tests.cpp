#include "run3/legacy/LegacyFeatures.hpp"
#include "run3/legacy/LegacySmoke.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
TEST_CASE("project-listed reusable legacy sources link") {
    CHECK(run3::legacy::reusableSourcesSmoke());
}

TEST_CASE("disabled legacy features fail visibly") {
    using run3::legacy::Feature;
    constexpr std::array features{
        Feature::newton,      Feature::ois,        Feature::cegui,
        Feature::hydrax,      Feature::skyx,       Feature::legacyAudio,
        Feature::directShow,  Feature::serial,     Feature::namedPipes,
    };

    for (const auto feature : features) {
        INFO(run3::legacy::featureName(feature));
        CHECK_FALSE(run3::legacy::featureAvailable(feature));
        CHECK_THROWS_AS(run3::legacy::requireFeature(feature),
                        run3::legacy::FeatureUnavailable);
    }
}
