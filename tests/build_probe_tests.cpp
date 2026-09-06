#include <run3/build_probe.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the build probe exposes its stable success message",
          "[build][probe]") {
    REQUIRE(run3::build::probe_message ==
            "Run3 modern build skeleton is operational");
}
