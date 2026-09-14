#include "run3/legacy/LegacyFeatures.hpp"
#include "run3/legacy/LegacySmoke.hpp"
#include "../InputManager2.h"

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace {
class CountingInputListener final : public run3::IInputListener {
public:
    bool onInputEvent(const run3::InputEvent &) override {
        ++calls;
        return false;
    }
    int calls{};
};
} // namespace
TEST_CASE("project-listed reusable legacy sources link") {
    CHECK(run3::legacy::reusableSourcesSmoke());
}

TEST_CASE("legacy input dispatcher consumes backend-neutral replay input") {
    run3::InputEvent event;
    event.type = run3::InputEventType::KeyPressed;
    event.key = run3::Key::W;
    run3::ReplayInput replay({{event}});
    CountingInputListener listener;
    auto *manager = buttonGUI::InputManager2::getSingletonPtr();
    manager->removeAllListeners();
    manager->initialise(replay);
    manager->addKeyListener(&listener, "test");
    manager->capture();
    CHECK(listener.calls == 1);
    CHECK(manager->state()->keyDown(run3::Key::W));
    manager->removeAllListeners();
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
