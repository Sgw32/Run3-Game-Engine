#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <run3/rendering/Lighting.hpp>
#include <run3/app/Configuration.hpp>
#include <run3/app/AppPaths.hpp>
#include <run3/gameplay/LegacyMaterialCatalog.hpp>
#include <run3/gameplay/SemiInfiniteWorld.hpp>
#include <limits>
#include <optional>
#include <set>

using namespace run3;
using namespace run3::rendering;
TEST_CASE("Lighting pipeline and shadow keys reject typos") {
  for(const auto *key:{"legacy-forward","deferred","pbr","fast-forward"})
    CHECK(pipelineName(parseLightingPipeline(key))==key);
  CHECK_THROWS(parseLightingPipeline("PBR"));
  CHECK_THROWS(parseShadowQuality("automatic"));
  CHECK(shadowBudget(ShadowQuality::Off,LightingPipeline::Pbr).textures==0);
  CHECK(shadowBudget(ShadowQuality::High,LightingPipeline::Pbr).splits==3);
  for (const auto pipeline : {LightingPipeline::LegacyForward,
                              LightingPipeline::FastForward,
                              LightingPipeline::Pbr,
                              LightingPipeline::Deferred})
    CHECK(shadowBudget(ShadowQuality::High,pipeline).textures==6);
  CHECK(shadowBudget(ShadowQuality::Low,LightingPipeline::Pbr).resolution==512);
  CHECK(shadowBudget(ShadowQuality::Ultra,LightingPipeline::Pbr).resolution==4096);
  CHECK(shadowBudget(ShadowQuality::Low,LightingPipeline::Pbr).distance==25000);
  CHECK(shadowBudget(ShadowQuality::High,LightingPipeline::Pbr).distance==500000);
  CHECK(shadowBudget(ShadowQuality::Ultra,LightingPipeline::Pbr).distance==500000);
}
TEST_CASE("Lighting CLI overrides user and content config") {
  const auto cli=parseCommandLine({"run3_shell","--lighting-pipeline","pbr",
      "--shadow-quality","high","--shadow-update-interval","3",
      "--exposure","2","--lighting-lab"},
      std::filesystem::path(RUN3_TEST_SOURCE_DIR));
  const auto config=Configuration::merge({{"render.lighting_pipeline","legacy-forward"}},
      {{"render.lighting_pipeline","fast-forward"},{"render.shadow_quality","off"}},cli.values);
  CHECK(config.valueOr("render.lighting_pipeline","")=="pbr");
  CHECK(config.valueOr("render.shadow_quality","")=="high");
  CHECK(config.valueOr("render.shadow_update_interval","")=="3");
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
TEST_CASE("Specular fallback stays dim and tight without a mask") {
  MaterialDescription surface;
  surface.name = "glossless";
  surface.specular = {.8F, .7F, .6F};
  surface.shininess = 16.0F;
  surface.roughness = .8F;
  static_cast<void>(resolveMaterial(surface, {}));
  CHECK(surface.specular[0] == Catch::Approx(.08F));
  CHECK(surface.specular[1] == Catch::Approx(.08F));
  CHECK(surface.shininess == Catch::Approx(64.0F));
  CHECK(surface.roughness == Catch::Approx(.35F));
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
  const auto transparentPlaceholder =
      catalog.find("TransparentInheritedPlaceholder");
  REQUIRE(transparentPlaceholder);
  CHECK(transparentPlaceholder->surface.surface == Surface::Transparent);
  CHECK(transparentPlaceholder->surface.diffuseMap.name == "wire.dds");
  CHECK(transparentPlaceholder->surface.specularMap.name.empty());
  const auto transparentGloss = catalog.find("TransparentAuthoredGloss");
  REQUIRE(transparentGloss);
  CHECK(transparentGloss->surface.specularMap.name == "glass_gloss.dds");
  const auto transparentAlias = catalog.find("TransparentLegacyAlias");
  REQUIRE(transparentAlias);
  CHECK(transparentAlias->surface.surface == Surface::Transparent);
  CHECK(transparentAlias->surface.diffuseMap.name == "wire.dds");
  CHECK(transparentAlias->surface.specularMap.name.empty());
  const auto animated = catalog.find("AnimatedScreen");
  REQUIRE(animated);
  CHECK_FALSE(animated->surface.lighting);
  CHECK(animated->surface.diffuseMap.name == "screen1.png");
  CHECK(animated->surface.diffuseAnimationFrames ==
        std::vector<std::string>{"screen1.png", "screen2.png", "screen3.png"});
  CHECK(animated->surface.diffuseAnimationDuration == Catch::Approx(1.5F));
  const auto generated = catalog.find("GeneratedAnimation");
  REQUIRE(generated);
  CHECK(generated->surface.diffuseAnimationBase == "caustic.png");
  CHECK(generated->surface.diffuseAnimationFrameCount == 32);
  CHECK(generated->surface.diffuseAnimationDuration == Catch::Approx(5.0F));
  CHECK(generated->surface.diffuseScrollU == Catch::Approx(0.25F));
  CHECK(generated->surface.diffuseScrollV == Catch::Approx(-0.5F));
  CHECK(generated->surface.diffuseRotate == Catch::Approx(0.125F));
  REQUIRE(generated->surface.diffuseWaveAnimations.size() == 1);
  CHECK(generated->surface.diffuseWaveAnimations.front().transform ==
        TextureTransform::ScaleU);
  CHECK(generated->surface.diffuseWaveAnimations.front().waveform ==
        TextureWaveform::Triangle);
  const auto unmapped = catalog.find("UnmappedGloss");
  REQUIRE(unmapped);
  auto unmappedGloss = unmapped->surface;
  static_cast<void>(resolveMaterial(unmappedGloss, {"glossless.png"}));
  CHECK(unmappedGloss.specular[0] == Catch::Approx(.08F));
  CHECK(unmappedGloss.shininess == Catch::Approx(64.0F));
  const auto additive = catalog.find("AdditiveParticle");
  REQUIRE(additive);
  CHECK(additive->surface.surface == Surface::Additive);
  const auto multipass = catalog.find("LitMultipass");
  REQUIRE(multipass);
  CHECK(multipass->surface.surface != Surface::Additive);
}
TEST_CASE("Derived overlay replaces remaster namespaces with fallback") {
  const auto root=std::filesystem::path(RUN3_TEST_SOURCE_DIR)/"tests/fixtures/lighting";
  auto paths=AppPaths::resolve(root/"shell",root/"original",root/"user");
  paths.setContentOverlay(root/"overlay");
  CHECK(paths.contentPath("run3/maps/test.scene")==root/"overlay/run3/maps/test.scene");
  CHECK(paths.contentPath("run3/maps/absent.scene")==root/"original/run3/maps/absent.scene");
  CHECK(paths.contentPath("run3/game/test.material")==root/"overlay/run3/game/test.material");
  CHECK(paths.contentPath("run3/game/absent.material")==root/"original/run3/game/absent.material");
  CHECK(paths.contentPath("run3/fonts/test.fontdef")==root/"overlay/run3/fonts/test.fontdef");
  CHECK(paths.contentPath("run3/fonts/absent.fontdef")==root/"original/run3/fonts/absent.fontdef");
  CHECK(paths.contentPath("run3/bblocks/test.json")==root/"overlay/run3/bblocks/test.json");
  CHECK(paths.contentPath("run3/bblocks/absent.json")==root/"original/run3/bblocks/absent.json");
  CHECK(paths.contentPath("run3/shaders/test.cg")==root/"overlay/run3/shaders/test.cg");
  CHECK(paths.contentPath("run3/shaders/absent.cg")==root/"original/run3/shaders/absent.cg");
  CHECK(paths.contentPath("run3/lua/denied.lua")==root/"original/run3/lua/denied.lua");
  CHECK_THROWS(paths.contentPath("../escape"));
  CHECK_THROWS(paths.setContentOverlay(root/"missing"));
}

TEST_CASE("Luniverse manifest and seeded biome tiles are deterministic") {
  const auto root=std::filesystem::path(RUN3_TEST_SOURCE_DIR)/
      "derived-content/luniverse/run3/bblocks/block_manifest.json";
  const auto manifest=gameplay::loadBlockManifest(root);
  REQUIRE(manifest.blocks.size()>=11);
  const gameplay::SeededBlockGenerator first(manifest,0x123456789abcdef0ULL,
                                              "01_BACKROOMS_20");
  const gameplay::SeededBlockGenerator second(manifest,0x123456789abcdef0ULL,
                                               "01_BACKROOMS_20");
  CHECK(first.blockFor({0,0,0}).id=="01_BACKROOMS_20");
  CHECK(first.blockFor({137,0,-91}).id==second.blockFor({137,0,-91}).id);
  // These fixed vectors are also consumed by tools/luniverse_map_viewer.py's
  // uint64 generator and prevent the editor and runtime algorithms drifting.
  CHECK(first.blockFor({-1,0,0}).id=="15_WOODROOMS_20x20x5");
  CHECK(first.blockFor({12,0,12}).id=="03_POOLROOMS_20");
  CHECK(first.blockFor({137,0,-91}).id=="09_OFFICE_20x20x5");
  CHECK(first.biomeFor({1,0,1})==first.biomeFor({2,0,2}));
  const auto &tile=first.blockFor({0,0,0});
  CHECK(tile.dimensionsCm.x==2000);
  CHECK(tile.dimensionsCm.y==2000);
  CHECK(tile.ports.size()==4);
  CHECK(gameplay::opposite(gameplay::BlockSide::North)==
        gameplay::BlockSide::South);

  const auto &standard=manifest.blocks.at(0);
  const auto houseIterator=std::find_if(
      manifest.blocks.begin(),manifest.blocks.end(),[](const auto &block) {
        return block.id=="05_HOUSE70_120x80";
      });
  REQUIRE(houseIterator!=manifest.blocks.end());
  const auto &house=*houseIterator;
  const auto standardSouth=std::find_if(
      standard.ports.begin(),standard.ports.end(),[](const auto &port) {
        return port.side==gameplay::BlockSide::South;
      });
  const auto houseNorth=std::find_if(
      house.ports.begin(),house.ports.end(),[](const auto &port) {
        return port.side==gameplay::BlockSide::North;
      });
  REQUIRE(standardSouth!=standard.ports.end());
  REQUIRE(houseNorth!=house.ports.end());
  const auto standardOrigin=gameplay::alignedBlockOrigin(
      {},*houseNorth,*standardSouth);
  CHECK(standardOrigin.x==Catch::Approx(0));
  CHECK(standardOrigin.y==Catch::Approx(500));
  CHECK(standardOrigin.z==Catch::Approx(-5000));

  std::set<std::string> generatedMeshes;
  bool foundCompatibleLargeFeature=false;
  std::optional<gameplay::BlockPlacement> visibleHouse;
  for(std::int64_t z=-40;z<=40;++z) {
    for(std::int64_t x=-40;x<=40;++x) {
      const auto placement=first.placementForCell({x,0,z});
      REQUIRE(placement.definition!=nullptr);
      CHECK(x>=placement.anchor.x);
      CHECK(0>=placement.anchor.y);
      CHECK(z>=placement.anchor.z);
      CHECK(x<placement.anchor.x+placement.widthCells);
      CHECK(0<placement.anchor.y+placement.heightCells);
      CHECK(z<placement.anchor.z+placement.depthCells);
      const auto repeated=first.placementForCell(placement.anchor);
      CHECK(repeated.definition->id==placement.definition->id);
      CHECK(repeated.widthCells==placement.widthCells);
      CHECK(repeated.depthCells==placement.depthCells);
      if(placement.widthCells==1 && placement.depthCells==1)
        CHECK(placement.definition->ports.size()==4);
      if(placement.widthCells==2 &&
         (placement.depthCells==1 || placement.depthCells==3))
        foundCompatibleLargeFeature=true;
      CHECK_FALSE((placement.widthCells==2 && placement.depthCells==2));
      if(placement.widthCells==6 && placement.depthCells==4)
        visibleHouse=placement;
      generatedMeshes.insert(placement.definition->mesh);
    }
  }
  CHECK(foundCompatibleLargeFeature);
  CHECK(generatedMeshes.size()>=5);
  REQUIRE(visibleHouse);
  CHECK(visibleHouse->anchor.y==-1);
  CHECK(visibleHouse->heightCells==8);
  const auto houseAtBasement=first.placementForCell(
      {visibleHouse->anchor.x,-1,visibleHouse->anchor.z});
  const auto houseAtGround=first.placementForCell(
      {visibleHouse->anchor.x,0,visibleHouse->anchor.z});
  const auto houseAboveGround=first.placementForCell(
      {visibleHouse->anchor.x,1,visibleHouse->anchor.z});
  CHECK(houseAtBasement.anchor==visibleHouse->anchor);
  CHECK(houseAtGround.anchor==visibleHouse->anchor);
  CHECK(houseAboveGround.anchor==visibleHouse->anchor);
  CHECK(houseAtGround.definition->id==visibleHouse->definition->id);
  CHECK(houseAboveGround.definition->id==visibleHouse->definition->id);

  // Vertical layouts are deterministic and use the manifest's 500 cm stack
  // grid. A tall landmark owns every level through its declared height.
  const auto upper=first.placementForCell({7,1,9});
  CHECK(upper.rootCm.y==Catch::Approx(
      upper.anchor.y*static_cast<double>(manifest.gridCm)));
  CHECK(upper.definition->id==second.placementForCell({7,1,9}).definition->id);
  bool foundTallVolume=false;
  for(std::int64_t z=-100;z<=100 && !foundTallVolume;++z) {
    for(std::int64_t x=-100;x<=100 && !foundTallVolume;++x) {
      const auto occupied=first.placementForCell({x,-1,z});
      if(occupied.heightCells<=1) continue;
      CHECK(occupied.anchor.y<=-1);
      CHECK(-1<occupied.anchor.y+occupied.heightCells);
      const auto bottom=first.placementForCell(
          {x,occupied.anchor.y,z});
      CHECK(bottom.anchor==occupied.anchor);
      CHECK(bottom.definition->id==occupied.definition->id);
      foundTallVolume=true;
    }
  }
  CHECK(foundTallVolume);

  const auto findPort=[](const gameplay::BlockPlacement &placement,
                         gameplay::BlockSide side) {
    return std::find_if(placement.definition->ports.begin(),
                        placement.definition->ports.end(),
                        [side](const auto &candidate) {
                          return candidate.side==side;
                        });
  };
  const auto socketWorld=[](const gameplay::BlockPlacement &placement,
                            const gameplay::BlockPort &port) {
    return physics::Vec3{
        placement.rootCm.x+port.positionCm.x,
        placement.rootCm.y+port.positionCm.z,
        placement.rootCm.z-port.positionCm.y};
  };
  // Every macro's horizontal backbone must cross both boundaries, including
  // where a 40x20 or 40x60 feature replaces multiple 20 m cells.
  for(std::int64_t macroZ=-8;macroZ<=8;++macroZ) {
    const std::int64_t z=macroZ*4+1;
    std::int64_t x=-48;
    while(x<48) {
      const auto left=first.placementForCell({x,0,z});
      const std::int64_t nextX=left.anchor.x+left.widthCells;
      const auto right=first.placementForCell({nextX,0,z});
      if(left.widthCells==6 || right.widthCells==6) {
        x=std::max(nextX,right.anchor.x+
            static_cast<std::int64_t>(right.widthCells));
        continue;
      }
      const auto east=findPort(left,gameplay::BlockSide::East);
      const auto west=findPort(right,gameplay::BlockSide::West);
      REQUIRE(east!=left.definition->ports.end());
      REQUIRE(west!=right.definition->ports.end());
      const auto a=socketWorld(left,*east);
      const auto b=socketWorld(right,*west);
      CHECK(a.x==Catch::Approx(b.x));
      CHECK(a.y==Catch::Approx(b.y));
      CHECK(a.z==Catch::Approx(b.z));
      x=nextX;
    }
  }
  // X=2 inside every six-cell macro column is reserved as an uninterrupted
  // north/south route and never intersects a large feature footprint.
  for(std::int64_t macroX=-8;macroX<=8;++macroX) {
    const std::int64_t x=macroX*6+2;
    for(std::int64_t z=-32;z<32;++z) {
      const auto near=first.placementForCell({x,0,z});
      const auto far=first.placementForCell({x,0,z+1});
      if(near.widthCells==6 || far.widthCells==6) continue;
      const auto south=findPort(near,gameplay::BlockSide::South);
      const auto north=findPort(far,gameplay::BlockSide::North);
      REQUIRE(south!=near.definition->ports.end());
      REQUIRE(north!=far.definition->ports.end());
      const auto a=socketWorld(near,*south);
      const auto b=socketWorld(far,*north);
      CHECK(a.x==Catch::Approx(b.x));
      CHECK(a.y==Catch::Approx(b.y));
      CHECK(a.z==Catch::Approx(b.z));
    }
  }
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
  const auto homeTv = catalog.find("TLW/HomeTvAnim");
  REQUIRE(homeTv);
  CHECK_FALSE(homeTv->surface.lighting);
  REQUIRE(homeTv->surface.diffuseAnimationFrames.size() == 5);
  CHECK(homeTv->surface.diffuseAnimationFrames.front() == "vesti_anim1.jpg");
  CHECK(homeTv->surface.diffuseAnimationFrames.back() == "vesti_anim5.jpg");
  CHECK(homeTv->surface.diffuseAnimationDuration == Catch::Approx(30.0F));
  const auto tvHum = catalog.find("TV_HUM");
  REQUIRE(tvHum);
  CHECK_FALSE(tvHum->surface.lighting);
  CHECK(tvHum->surface.diffuseAnimationFrames ==
        std::vector<std::string>{"kvan01.dds", "kvan02.dds", "kvan03.dds"});
  CHECK(tvHum->surface.diffuseAnimationDuration == Catch::Approx(0.3F));
  const auto tvHum2 = catalog.find("TV_HUM2");
  REQUIRE(tvHum2);
  CHECK(tvHum2->surface.diffuseAnimationFrames ==
        std::vector<std::string>{"petr01.dds", "petr02.dds", "petr03.dds"});
  CHECK(tvHum2->surface.diffuseAnimationDuration == Catch::Approx(5.0F));
  const auto scope = catalog.find("SCOPE_MATERIAL01");
  REQUIRE(scope);
  CHECK(scope->surface.diffuseMap.name == "scope_phl.dds");
  CHECK(scope->surface.diffuseScrollU == Catch::Approx(2.0F));
  CHECK(scope->surface.diffuseScrollV == Catch::Approx(0.0F));
}

TEST_CASE("Legacy converted materials resolve absent TGA names to selected DDS") {
  gameplay::LegacyMaterialCatalog catalog;
  const auto content = std::filesystem::path(RUN3_TEST_SOURCE_DIR) /
                       "Games/The Long Way/TheLongWay";
  catalog.scan(content, content / "run3/mats/high");

  const auto vodniki = catalog.find("VODNIKI1material_18");
  REQUIRE(vodniki);
  CHECK(vodniki->surface.diffuseMap.name == "vodniki1_material_18.dds");
  CHECK(vodniki->surface.surface == Surface::Transparent);
  CHECK_FALSE(vodniki->surface.lighting);
}
