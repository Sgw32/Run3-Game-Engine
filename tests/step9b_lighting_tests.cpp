#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <run3/rendering/Lighting.hpp>
#include <run3/app/Configuration.hpp>
#include <run3/app/AppPaths.hpp>
#include <run3/gameplay/LegacyMaterialCatalog.hpp>
#include <limits>

using namespace run3;
using namespace run3::rendering;
TEST_CASE("Lighting pipeline and shadow keys reject typos") {
  for(const auto *key:{"legacy-forward","deferred","pbr","fast-forward"})
    CHECK(pipelineName(parseLightingPipeline(key))==key);
  CHECK_THROWS(parseLightingPipeline("PBR"));
  CHECK_THROWS(parseShadowQuality("automatic"));
  CHECK(shadowBudget(ShadowQuality::Off,LightingPipeline::Pbr).textures==0);
  CHECK(shadowBudget(ShadowQuality::High,LightingPipeline::Pbr).splits==3);
  CHECK(shadowBudget(ShadowQuality::High,LightingPipeline::FastForward).textures==1);
}
TEST_CASE("Lighting CLI overrides user and content config") {
  const auto cli=parseCommandLine({"run3_shell","--lighting-pipeline","pbr",
      "--shadow-quality","high","--exposure","2","--lighting-lab"},
      std::filesystem::path(RUN3_TEST_SOURCE_DIR));
  const auto config=Configuration::merge({{"render.lighting_pipeline","legacy-forward"}},
      {{"render.lighting_pipeline","fast-forward"},{"render.shadow_quality","off"}},cli.values);
  CHECK(config.valueOr("render.lighting_pipeline","")=="pbr");
  CHECK(config.valueOr("render.shadow_quality","")=="high");
  CHECK(config.valueOr("render.exposure","")=="2");
}
TEST_CASE("Optional surface maps fall back without discarding base colour") {
  MaterialDescription surface;
  surface.name="fixture";surface.diffuse={.2F,.3F,.4F,1};
  surface.diffuseMap.name="albedo.dds";surface.normalMap.name="missing.dds";
  surface.aoMap.name="missing-ao.dds";
  const auto warnings=resolveMaterial(surface,{"albedo.dds"});
  REQUIRE(warnings.size()==2);
  CHECK(surface.diffuseMap.name=="albedo.dds");
  CHECK(surface.normalMap.name.empty());CHECK(surface.aoMap.name.empty());
  CHECK(surface.diffuse[0]==.2F);
  CHECK(surface.diffuseMap.colourSpace==ColourSpace::Srgb);
  CHECK(surface.normalMap.colourSpace==ColourSpace::Linear);
  surface.roughness=std::numeric_limits<float>::quiet_NaN();
  CHECK_THROWS(resolveMaterial(surface,{}));
}
TEST_CASE("Legacy gloss maps monotonically to PBR roughness") {
  CHECK(roughnessFromShininess(0)==Catch::Approx(1));
  CHECK(roughnessFromShininess(128)<roughnessFromShininess(32));
  CHECK(roughnessFromShininess(1e8F)==Catch::Approx(.045));
  CHECK_THROWS(roughnessFromShininess(-1));
}
TEST_CASE("Material inheritance preserves separate per-material light budgets") {
  gameplay::LegacyMaterialCatalog catalog;
  catalog.scan(std::filesystem::path(RUN3_TEST_SOURCE_DIR)/"tests/fixtures/lighting/materials");
  REQUIRE(catalog.find("Child"));
  const auto child=catalog.find("Child")->surface;
  CHECK(child.lightLimit==3);
  CHECK(child.normalMap.name=="n.png");CHECK(child.specularMap.name=="s.png");
  CHECK(child.aoMap.name=="ao.png");CHECK(child.shininess==64);
  CHECK(child.diffuse[0]==Catch::Approx(.7));
  CHECK(catalog.find("FixedTwo")->surface.lightLimit==2);
  CHECK(catalog.find("ParallaxThree")->surface.lightLimit==3);
  CHECK(catalog.find("Two")->surface.lightLimit==2);
  CHECK(catalog.find("Eight")->surface.lightLimit==8);
  CHECK(catalog.find("Cutout")->surface.surface==Surface::Cutout);
}
TEST_CASE("Derived overlay only replaces core and maps and keeps original fallback") {
  const auto root=std::filesystem::path(RUN3_TEST_SOURCE_DIR)/"tests/fixtures/lighting";
  auto paths=AppPaths::resolve(root/"shell",root/"original",root/"user");
  paths.setContentOverlay(root/"overlay");
  CHECK(paths.contentPath("run3/maps/test.scene")==root/"overlay/run3/maps/test.scene");
  CHECK(paths.contentPath("run3/maps/absent.scene")==root/"original/run3/maps/absent.scene");
  CHECK(paths.contentPath("run3/lua/denied.lua")==root/"original/run3/lua/denied.lua");
  CHECK_THROWS(paths.contentPath("../escape"));
  CHECK_THROWS(paths.setContentOverlay(root/"missing"));
}
TEST_CASE("Lighting material roles preserve unlit alpha and exclude volume noise") {
  gameplay::LegacyMaterialCatalog catalog;
  catalog.scan(std::filesystem::path(RUN3_TEST_SOURCE_DIR)/"tests/fixtures/lighting/materials");
  const auto leaves = catalog.find("UnlitLeaves");
  REQUIRE(leaves);
  CHECK_FALSE(leaves->surface.lighting);
  CHECK(leaves->surface.surface == Surface::Cutout);
  CHECK(leaves->surface.alphaCutoff == Catch::Approx(129.0 / 255.0));
  const auto glass = catalog.find("Glass");
  REQUIRE(glass);
  CHECK_FALSE(glass->surface.lighting);
  CHECK(glass->surface.surface == Surface::Transparent);
  CHECK(glass->surface.diffuse[3] == Catch::Approx(.3));
  CHECK(glass->surface.diffuseMap.name.empty());
  CHECK(glass->surface.reflectionMap.name == "env_glass.png");
  CHECK(glass->surface.reflectionMapping == ReflectionMapping::Spherical);
  CHECK(glass->surface.reflectionWeight == Catch::Approx(.1));
  const auto sky = catalog.find("SkyDirections");
  REQUIRE(sky);
  CHECK_FALSE(sky->surface.lighting);
  CHECK(sky->surface.reflectionMapping == ReflectionMapping::CubeDirection);
  CHECK(sky->surface.diffuseMap.name.empty());
  const auto volume = catalog.find("VolumeEffect");
  REQUIRE(volume);
  CHECK(volume->surface.diffuseMap.name.empty());
  CHECK_FALSE(volume->surface.compatibilityNotes.empty());
  const auto maps = catalog.find("NamedMaps");
  REQUIRE(maps);
  CHECK(maps->surface.diffuseMap.name == "base.png");
  CHECK(maps->surface.normalMap.name == "named-normal.png");
  CHECK(maps->surface.specularMap.name == "gloss.png");
  CHECK(maps->surface.reflectionMapping == ReflectionMapping::Cube);
  CHECK(maps->surface.reflectionMap.name == "morning.jpg");
  const auto overlay = catalog.find("OverlayButton");
  REQUIRE(overlay);
  CHECK(overlay->surface.diffuseMap.name == "overlay-button.png");
  CHECK_FALSE(overlay->surface.lighting);
}

TEST_CASE("Legacy buttonGUI texture aliases resolve from shipped content") {
  gameplay::LegacyMaterialCatalog catalog;
  const auto content = std::filesystem::path(RUN3_TEST_SOURCE_DIR) /
                       "Games/The Long Way/TheLongWay";
  catalog.scan(content);
  const auto antique = catalog.find("TLW/AntiqueButton1");
  REQUIRE(antique);
  CHECK(antique->surface.diffuseMap.name == "antuque_shop_portraits.jpg");
  CHECK_FALSE(antique->surface.lighting);
  const auto inventory = catalog.find("Inventory/Portmone");
  REQUIRE(inventory);
  CHECK_FALSE(inventory->surface.diffuseMap.name.empty());
}
