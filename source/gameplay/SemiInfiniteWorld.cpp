#include <run3/gameplay/SemiInfiniteWorld.hpp>

#include <run3/rendering/OgreLighting.hpp>

#include <OgreEntity.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgrePass.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace run3::gameplay {
namespace fs = std::filesystem;
namespace {

using Json = nlohmann::json;
constexpr double standardCellCm = 2000.0;
const Ogre::String resourceGroup = "Run3Step6BContent";

std::uint64_t mix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

std::uint64_t coordinateHash(const std::uint64_t seed, const std::int64_t x,
                             const std::int64_t z,
                             const std::uint64_t salt = 0) noexcept {
  std::uint64_t value = seed ^ salt;
  value ^= mix64(static_cast<std::uint64_t>(x));
  value ^= mix64(static_cast<std::uint64_t>(z) + 0x632be59bd9b4e019ULL);
  return mix64(value);
}

std::uint64_t coordinateHash3(const std::uint64_t seed, const std::int64_t x,
                              const std::int64_t y, const std::int64_t z,
                              const std::uint64_t salt = 0) noexcept {
  // Preserve the original level-zero vectors while giving every other level
  // an independent, deterministic layout.
  std::uint64_t value = coordinateHash(seed, x, z, salt);
  if (y == 0) return value;
  value ^= mix64(static_cast<std::uint64_t>(y) + 0xd6e8feb86659fd93ULL);
  return mix64(value ^ 0xa5a3564e27f8862bULL);
}

std::int64_t floorDiv(const std::int64_t value,
                      const std::int64_t divisor) noexcept {
  const std::int64_t quotient = value / divisor;
  const std::int64_t remainder = value % divisor;
  return remainder < 0 ? quotient - 1 : quotient;
}

BlockSide parseSide(const std::string &value) {
  if (value == "N") return BlockSide::North;
  if (value == "S") return BlockSide::South;
  if (value == "E") return BlockSide::East;
  if (value == "W") return BlockSide::West;
  throw std::runtime_error("block manifest has invalid port side '" + value +
                           "'");
}

physics::Vec3 triple(const Json &value, const char *field) {
  if (!value.is_array() || value.size() != 3) {
    throw std::runtime_error(std::string("block manifest field '") + field +
                             "' must contain three numbers");
  }
  return {value.at(0).get<double>(), value.at(1).get<double>(),
          value.at(2).get<double>()};
}

Ogre::Vector3 toOgre(const physics::Vec3 value) {
  return {static_cast<Ogre::Real>(value.x), static_cast<Ogre::Real>(value.y),
          static_cast<Ogre::Real>(value.z)};
}

physics::Vec3 authoredToOgre(const physics::Vec3 value) {
  // Blender authoring is X/depth/Z-up. The supplied Ogre meshes use X/Y-up/-Z.
  return {value.x, value.z, -value.y};
}

struct CellHash {
  std::size_t operator()(const BlockCell cell) const noexcept {
    return static_cast<std::size_t>(
        coordinateHash3(0, cell.x, cell.y, cell.z));
  }
};

struct MeshGeometry {
  std::vector<physics::Vec3> vertices;
  std::vector<std::uint32_t> indices;
};

MeshGeometry extractMesh(const Ogre::MeshPtr &mesh) {
  MeshGeometry result;
  for (unsigned subIndex = 0; subIndex < mesh->getNumSubMeshes(); ++subIndex) {
    Ogre::SubMesh *subMesh = mesh->getSubMesh(subIndex);
    Ogre::VertexData *vertexData = subMesh->useSharedVertices
                                       ? mesh->sharedVertexData
                                       : subMesh->vertexData;
    if (vertexData == nullptr || subMesh->indexData == nullptr) continue;
    const Ogre::VertexElement *positionElement =
        vertexData->vertexDeclaration->findElementBySemantic(
            Ogre::VES_POSITION);
    if (positionElement == nullptr) continue;
    const Ogre::HardwareVertexBufferSharedPtr vertexBuffer =
        vertexData->vertexBufferBinding->getBuffer(positionElement->getSource());
    auto *vertexBytes = static_cast<unsigned char *>(
        vertexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
    const std::uint32_t base = static_cast<std::uint32_t>(result.vertices.size());
    for (std::size_t index = 0; index < vertexData->vertexCount; ++index) {
      float *position{};
      positionElement->baseVertexPointerToElement(
          vertexBytes + index * vertexBuffer->getVertexSize(), &position);
      result.vertices.push_back({position[0], position[1], position[2]});
    }
    vertexBuffer->unlock();

    const Ogre::HardwareIndexBufferSharedPtr indexBuffer =
        subMesh->indexData->indexBuffer;
    const bool use32 =
        indexBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT;
    const void *raw = indexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
    for (std::size_t index = 0; index < subMesh->indexData->indexCount; ++index) {
      const std::size_t offset = subMesh->indexData->indexStart + index;
      const std::uint32_t value =
          use32 ? static_cast<const std::uint32_t *>(raw)[offset]
                : static_cast<std::uint32_t>(
                      static_cast<const std::uint16_t *>(raw)[offset]);
      result.indices.push_back(base + value);
    }
    indexBuffer->unlock();
  }
  return result;
}

const BlockPort *port(const BlockDefinition &block, const BlockSide side) {
  const auto found = std::find_if(
      block.ports.begin(), block.ports.end(),
      [side](const BlockPort &candidate) { return candidate.side == side; });
  return found == block.ports.end() ? nullptr : &*found;
}

std::string styleTexture(const std::string &style) {
  return style + "_Atlas_4K.png";
}

} // namespace

BlockSide opposite(const BlockSide side) noexcept {
  switch (side) {
  case BlockSide::North: return BlockSide::South;
  case BlockSide::South: return BlockSide::North;
  case BlockSide::East: return BlockSide::West;
  case BlockSide::West: return BlockSide::East;
  }
  return BlockSide::North;
}

physics::Vec3 alignedBlockOrigin(const physics::Vec3 placedBlockOrigin,
                                 const BlockPort &placed,
                                 const BlockPort &attached) {
  if (opposite(placed.side) != attached.side)
    throw std::invalid_argument("block ports must have opposite outward normals");
  if (std::abs(placed.widthCm - attached.widthCm) > 0.01 ||
      std::abs(placed.heightCm - attached.heightCm) > 0.01)
    throw std::invalid_argument("block ports must have matching opening sizes");
  const physics::Vec3 source = authoredToOgre(placed.positionCm);
  const physics::Vec3 target = authoredToOgre(attached.positionCm);
  return {placedBlockOrigin.x + source.x - target.x,
          placedBlockOrigin.y + source.y - target.y,
          placedBlockOrigin.z + source.z - target.z};
}

BlockManifest loadBlockManifest(const fs::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot open block manifest '" + path.string() + "'");
  Json root;
  try {
    stream >> root;
  } catch (const Json::exception &error) {
    throw std::runtime_error("invalid block manifest '" + path.string() +
                             "': " + error.what());
  }
  BlockManifest result;
  result.unitsPerMeter = root.value("units_per_meter", 100U);
  result.gridCm = root.value("grid_cm", 500U);
  result.biomeScaleCm = root.value("biome_scale_cm", 24000.0);
  if (result.unitsPerMeter == 0 || result.gridCm == 0 ||
      result.biomeScaleCm < standardCellCm) {
    throw std::runtime_error("block manifest has invalid scale constants");
  }
  if (!root.contains("blocks") || !root.at("blocks").is_array())
    throw std::runtime_error("block manifest has no blocks array");
  std::set<std::string> ids;
  for (const Json &item : root.at("blocks")) {
    BlockDefinition block;
    block.id = item.at("id").get<std::string>();
    block.mesh = item.value("mesh", block.id + ".mesh");
    block.style = item.value("style", std::string("DEFAULT"));
    block.dimensionsCm = triple(item.at("dimensions_cm"), "dimensions_cm");
    block.termination = item.value("termination", false);
    if (!ids.insert(block.id).second || block.id.empty() || block.mesh.empty() ||
        block.dimensionsCm.x <= 0 || block.dimensionsCm.y <= 0 ||
        block.dimensionsCm.z <= 0) {
      throw std::runtime_error("block manifest has duplicate or invalid block '" +
                               block.id + "'");
    }
    if (item.contains("ports")) {
      for (const Json &source : item.at("ports")) {
        BlockPort parsed;
        parsed.side = parseSide(source.at("side").get<std::string>());
        parsed.positionCm = triple(source.at("position_cm"), "position_cm");
        if (source.contains("size_cm")) {
          const Json &size = source.at("size_cm");
          if (!size.is_array() || size.size() != 2)
            throw std::runtime_error("port size_cm must contain two numbers");
          parsed.widthCm = size.at(0).get<double>();
          parsed.heightCm = size.at(1).get<double>();
        }
        block.ports.push_back(parsed);
      }
    }
    if (item.contains("light")) {
      const Json &light = item.at("light");
      block.light.enabled = light.value("enabled", true);
      block.light.positionCm =
          light.contains("position_cm")
              ? triple(light.at("position_cm"), "light.position_cm")
              : physics::Vec3{0, 0, block.dimensionsCm.z - 40.0};
      block.light.rangeCm = light.value("range_cm", 1800.0F);
      if (light.contains("colour")) {
        const physics::Vec3 colour = triple(light.at("colour"), "light.colour");
        block.light.colour = {static_cast<float>(colour.x),
                              static_cast<float>(colour.y),
                              static_cast<float>(colour.z)};
      }
    }
    result.blocks.push_back(block);
    if (item.contains("variants")) {
      for (const Json &source : item.at("variants")) {
        BlockDefinition variant = block;
        variant.id = source.at("id").get<std::string>();
        variant.mesh = source.value("mesh", variant.id + ".mesh");
        if (!ids.insert(variant.id).second || variant.id.empty() ||
            variant.mesh.empty())
          throw std::runtime_error("block manifest has duplicate or invalid variant '" +
                                   variant.id + "'");
        const std::string openSides = source.value("open_sides", "NSEW");
        variant.ports.erase(
            std::remove_if(variant.ports.begin(), variant.ports.end(),
                           [&openSides](const BlockPort &candidate) {
              const char side = candidate.side == BlockSide::North ? 'N' :
                                candidate.side == BlockSide::South ? 'S' :
                                candidate.side == BlockSide::East ? 'E' : 'W';
              return openSides.find(side) == std::string::npos;
            }),
            variant.ports.end());
        result.blocks.push_back(std::move(variant));
      }
    }
  }
  if (result.blocks.empty()) throw std::runtime_error("block manifest is empty");
  return result;
}

SeededBlockGenerator::SeededBlockGenerator(const BlockManifest &manifest,
                                           const std::uint64_t seed,
                                           std::string originBlock)
    : manifest_(&manifest), seed_(seed) {
  const auto hasSide = [](const BlockDefinition &block, const BlockSide side) {
    return std::any_of(block.ports.begin(), block.ports.end(),
                       [side](const BlockPort &candidate) {
                         return candidate.side == side;
                       });
  };
  for (std::size_t index = 0; index < manifest.blocks.size(); ++index) {
    const BlockDefinition &block = manifest.blocks[index];
    if (block.id == originBlock) originIndex_ = index;
    if (block.termination) continue;
    maximumHeightCells_ = std::max(
        maximumHeightCells_, static_cast<unsigned>(std::ceil(
                                 block.dimensionsCm.z / manifest.gridCm)));
    const bool small = std::abs(block.dimensionsCm.x - 2000.0) < 0.01 &&
                       std::abs(block.dimensionsCm.y - 2000.0) < 0.01;
    const bool fullSmall = small && hasSide(block, BlockSide::North) &&
                           hasSide(block, BlockSide::South) &&
                           hasSide(block, BlockSide::East) &&
                           hasSide(block, BlockSide::West);
    const bool horizontalFeature =
        std::abs(block.dimensionsCm.x - 4000.0) < 0.01 &&
        (std::abs(block.dimensionsCm.y - 2000.0) < 0.01 ||
         std::abs(block.dimensionsCm.y - 6000.0) < 0.01) &&
        hasSide(block, BlockSide::East) && hasSide(block, BlockSide::West);
    const bool house = std::abs(block.dimensionsCm.x - 12000.0) < 0.01 &&
                       std::abs(block.dimensionsCm.y - 8000.0) < 0.01;
    if (fullSmall) smallIndices_.push_back(index);
    else if (horizontalFeature) horizontalFeatureIndices_.push_back(index);
    else if (house) houseIndices_.push_back(index);
    if (fullSmall &&
        std::find(styles_.begin(), styles_.end(), block.style) == styles_.end())
      styles_.push_back(block.style);
  }
  if (smallIndices_.empty() || styles_.empty())
    throw std::runtime_error(
        "block manifest needs a four-port 20 metre occupancy block");
  if (originBlock.empty()) originIndex_ = smallIndices_.front();
  const auto origin = std::find(smallIndices_.begin(), smallIndices_.end(), originIndex_);
  if (origin == smallIndices_.end())
    throw std::runtime_error("origin block must be a 20 metre occupancy tile");
}

std::string_view SeededBlockGenerator::biomeFor(const BlockCell cell) const {
  const std::int64_t horizontalSpan = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::llround(
             manifest_->biomeScaleCm / standardCellCm)));
  const std::int64_t verticalSpan = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::llround(
             manifest_->biomeScaleCm / manifest_->gridCm)));
  const std::int64_t coarseX = floorDiv(cell.x, horizontalSpan);
  const std::int64_t coarseY = floorDiv(cell.y, verticalSpan);
  const std::int64_t coarseZ = floorDiv(cell.z, horizontalSpan);
  const auto index = coordinateHash3(seed_, coarseX, coarseY, coarseZ,
                                     0x42494f4d45ULL) % styles_.size();
  return styles_[static_cast<std::size_t>(index)];
}

const BlockDefinition &SeededBlockGenerator::blockFor(const BlockCell cell) const {
  return *placementForCell(cell).definition;
}

BlockPlacement SeededBlockGenerator::placementForCell(const BlockCell cell) const {
  constexpr std::int64_t macroWidth = 6;
  constexpr std::int64_t macroDepth = 4;
  constexpr std::uint64_t occupancySalt = 0x4f43435550414e43ULL;
  constexpr std::uint64_t variantSalt = 0x56415249414e5400ULL;
  const std::int64_t macroX = floorDiv(cell.x, macroWidth);
  const std::int64_t macroZ = floorDiv(cell.z, macroDepth);
  const std::int64_t macroMinX = macroX * macroWidth;
  const std::int64_t macroMinZ = macroZ * macroDepth;
  // Shift the vertical bands down by one stacking level. For the current
  // 4000 cm house on a 500 cm grid this gives roots ...,-9,-1,7,... . The
  // root at -1 puts the basement on level -1 and its ground exits on level 0.
  const std::int64_t houseRootY =
      floorDiv(cell.y + 1, static_cast<std::int64_t>(maximumHeightCells_)) *
          static_cast<std::int64_t>(maximumHeightCells_) -
      1;

  const auto select = [this](const std::vector<std::size_t> &source,
                             const std::string_view style,
                             const std::int64_t x, const std::int64_t y,
                             const std::int64_t z,
                             const std::uint64_t salt) -> const BlockDefinition * {
    std::vector<std::size_t> choices;
    for (const std::size_t index : source)
      if (manifest_->blocks[index].style == style) choices.push_back(index);
    if (choices.empty()) choices = source;
    const std::size_t selected = static_cast<std::size_t>(
        coordinateHash3(seed_, x, y, z, salt) % choices.size());
    return &manifest_->blocks[choices[selected]];
  };
  const auto placement = [this](const BlockCell anchor,
                                const BlockDefinition *definition,
                                const unsigned width, const unsigned depth) {
    const unsigned height = std::max(
        1U, static_cast<unsigned>(std::ceil(
                definition->dimensionsCm.z / manifest_->gridCm)));
    return BlockPlacement{
        anchor, definition, width, height, depth,
        {(static_cast<double>(anchor.x) + (width - 1) * 0.5) * standardCellCm,
         static_cast<double>(anchor.y) * manifest_->gridCm,
         (static_cast<double>(anchor.z) + (depth - 1) * 0.5) * standardCellCm}};
  };

  // Tall landmarks are selected only at a shared vertical-band root. Their
  // complete X/Y/Z volume maps back to one placement, so no independently
  // generated floor can appear inside a house.
  const bool originMacro = macroX == 0 && macroZ == 0 && houseRootY == -1;
  const bool useHouse = !originMacro && !houseIndices_.empty() &&
      coordinateHash3(seed_, macroX, houseRootY, macroZ, occupancySalt) % 11U == 0U;
  if (useHouse) {
    const BlockDefinition *definition = select(
        houseIndices_, "HOUSE70", macroX, houseRootY, macroZ, variantSalt);
    const BlockPlacement house = placement(
        {macroMinX, houseRootY, macroMinZ}, definition, 6, 4);
    if (cell.y < houseRootY + static_cast<std::int64_t>(house.heightCells))
      return house;
  }

  const std::int64_t localX = cell.x - macroMinX;
  const std::int64_t localZ = cell.z - macroMinZ;
  if (!horizontalFeatureIndices_.empty() &&
      coordinateHash3(seed_, macroX, cell.y, macroZ, occupancySalt) % 8U == 0U) {
    const bool right =
        (coordinateHash3(seed_, macroX, cell.y, macroZ,
                         occupancySalt ^ 0x5249474854ULL) & 1U) != 0;
    const std::int64_t featureX = right ? 4 : 0;
    const BlockCell featureProbe{macroMinX + featureX, cell.y, macroMinZ + 1};
    const std::string_view biome = biomeFor(featureProbe);
    std::vector<std::size_t> compatible;
    for (const std::size_t index : horizontalFeatureIndices_)
      if (manifest_->blocks[index].style == biome) compatible.push_back(index);
    if (compatible.empty()) compatible = horizontalFeatureIndices_;
    const BlockDefinition *definition = &manifest_->blocks[compatible[
        coordinateHash3(seed_, macroX, cell.y, macroZ,
                        variantSalt ^ 0x484f52495aULL) % compatible.size()]];
    const unsigned depth = static_cast<unsigned>(
        std::llround(definition->dimensionsCm.y / standardCellCm));
    const std::int64_t featureZ = depth == 1 ? 1 : 0;
    const BlockCell featureAnchor{macroMinX + featureX, cell.y,
                                  macroMinZ + featureZ};
    if (localX >= featureX && localX < featureX + 2 &&
        localZ >= featureZ &&
        localZ < featureZ + static_cast<std::int64_t>(depth))
      return placement(featureAnchor, definition, 2, depth);
  }

  const BlockDefinition *definition =
      cell.x == 0 && cell.y == 0 && cell.z == 0
          ? &manifest_->blocks[originIndex_]
          : select(smallIndices_, biomeFor(cell), cell.x, cell.y, cell.z,
                   variantSalt);
  return placement(cell, definition, 1, 1);
}

std::uint64_t SeededBlockGenerator::seed() const noexcept { return seed_; }
double SeededBlockGenerator::cellSizeCm() const noexcept { return standardCellCm; }

class SemiInfiniteWorld::Impl {
public:
  Impl(Ogre::SceneManager &sceneManager, physics::PhysicsWorld &physics,
       SemiInfiniteWorldOptions options)
      : scene_(&sceneManager), physics_(&physics), options_(std::move(options)),
        manifest_(loadBlockManifest(options_.manifest)),
        generator_(manifest_, options_.seed, options_.originBlock) {
    if (options_.horizontalLoadRadiusCm < standardCellCm ||
        options_.verticalLoadRadiusCm < 0 ||
        options_.rebaseThresholdCm < options_.horizontalLoadRadiusCm ||
        options_.logicalExtentCm < 100000000 || options_.lightBudget > 3) {
      throw std::runtime_error("invalid semi-infinite world limits");
    }
    blockRoot_ = options_.manifest.parent_path();
    auto &resources = Ogre::ResourceGroupManager::getSingleton();
    resources.addResourceLocation(blockRoot_.string(), "FileSystem",
                                  resourceGroup, true, true);
    const fs::path textures = blockRoot_ / "textures";
    if (fs::is_directory(textures))
      resources.addResourceLocation(textures.string(), "FileSystem",
                                    resourceGroup, false, true);
    root_ = scene_->getRootSceneNode()->createChildSceneNode(
        "Run3SemiInfiniteWorldRoot");
    createMaterials();
    if (options_.terminators) createTerminatorMesh();
    Ogre::LogManager::getSingleton().logMessage(
        "Luniverse: seed=" + std::to_string(options_.seed) +
        " active-radius=" + std::to_string(options_.horizontalLoadRadiusCm) +
        "cm vertical-levels=+/-" + std::to_string(std::max<std::int64_t>(
            1, static_cast<std::int64_t>(std::ceil(
                   options_.verticalLoadRadiusCm / manifest_.gridCm)))) +
        " logical-extent=" + std::to_string(options_.logicalExtentCm) +
        "cm terminators=" + (options_.terminators ? "on" : "off"));
  }

  ~Impl() { unload(); }

  struct ActiveBlock {
    Ogre::SceneNode *node{};
    Ogre::Entity *entity{};
    physics::BodyHandle body;
    const BlockDefinition *definition{};
    BlockPlacement placement;
  };

  struct TerminationKey {
    BlockCell cell;
    BlockSide side{BlockSide::North};
    friend bool operator==(const TerminationKey &left,
                           const TerminationKey &right) {
      return left.cell == right.cell && left.side == right.side;
    }
  };
  struct TerminationHash {
    std::size_t operator()(const TerminationKey &key) const noexcept {
      return CellHash{}(key.cell) ^
             (static_cast<std::size_t>(key.side) * 0x9e3779b9U);
    }
  };
  struct ActiveTermination {
    Ogre::SceneNode *node{};
    Ogre::Entity *entity{};
    physics::BodyHandle body;
  };
  struct ActiveLight {
    BlockCell cell;
    Ogre::SceneNode *node{};
    Ogre::Light *light{};
  };
  struct StreamPlan {
    BlockCell centre;
    std::unordered_map<BlockCell, BlockPlacement, CellHash> blocks;
    std::unordered_set<TerminationKey, TerminationHash> terminations;
  };

  void createMaterials() {
    auto &manager = Ogre::MaterialManager::getSingleton();
    std::set<std::string> styles;
    for (const BlockDefinition &block : manifest_.blocks)
      if (!block.termination) styles.insert(block.style);
    for (const std::string &style : styles) {
      for (const std::string &role : {"Surface", "Luminous", "Water"}) {
        const std::string name = "Run3/Luniverse/" + style + "/" + role;
        Ogre::MaterialPtr material = manager.getByName(name, resourceGroup);
        if (!material) material = manager.create(name, resourceGroup);
        rendering::MaterialDescription description;
        description.name = name;
        description.diffuseMap.name = styleTexture(style);
        description.roughness = role == "Water" ? 0.12F : 0.52F;
        description.specular = role == "Water"
                                   ? std::array<float, 3>{0.32F, 0.38F, 0.42F}
                                   : std::array<float, 3>{0.045F, 0.045F, 0.045F};
        if (role == "Luminous")
          description.emissive = {0.48F, 0.46F, 0.34F};
        description.lightLimit = 3;
        rendering::OgreLighting::configureMaterial(
            *material, description, options_.lighting, false);
        materials_[style + "/" + role] = material;

        const std::string authoredRole =
            role == "Surface" ? "SURFACE" :
            role == "Luminous" ? "LUMINOUS" : "WATER";
        const std::string authoredName =
            style + "_/_" + authoredRole + "_/_shared_atlas";
        if (!manager.resourceExists(authoredName, resourceGroup))
          material->clone(authoredName, resourceGroup);
      }
    }
    const auto houseLight = materials_.find("HOUSE70/Luminous");
    const std::string artificialSky =
        "HOUSE70_/_painted_artificial_sky_/_shared_atlas";
    if (houseLight != materials_.end() &&
        !manager.resourceExists(artificialSky, resourceGroup))
      houseLight->second->clone(artificialSky, resourceGroup);
    if (options_.terminators) {
      rendering::MaterialDescription terminator;
      terminator.name = "Run3/Luniverse/Terminator";
      terminator.diffuseMap.name = "BACKROOMS_Atlas_4K.png";
      terminator.diffuse = {0.38F, 0.16F, 0.10F, 1.0F};
      terminator.roughness = 0.68F;
      terminator.lightLimit = 3;
      Ogre::MaterialPtr material = manager.getByName(terminator.name, resourceGroup);
      if (!material) material = manager.create(terminator.name, resourceGroup);
      rendering::OgreLighting::configureMaterial(
          *material, terminator, options_.lighting, false);
      materials_["TERMINATOR/Surface"] = material;
    }
  }

  void createTerminatorMesh() {
    if (Ogre::MeshManager::getSingleton().resourceExists("box.mesh",
                                                          resourceGroup))
      return;
    Ogre::ManualObject *object = scene_->createManualObject(
        "Run3LuniverseTerminatorBuilder");
    object->begin("Run3/Luniverse/Terminator", Ogre::RenderOperation::OT_TRIANGLE_LIST,
                  resourceGroup);
    const float x0 = -200, x1 = 200, y0 = 0, y1 = 320, z0 = -25, z1 = 25;
    const auto face = [object](const Ogre::Vector3 &a, const Ogre::Vector3 &b,
                               const Ogre::Vector3 &c, const Ogre::Vector3 &d,
                               const Ogre::Vector3 &normal) {
      const std::uint32_t base = static_cast<std::uint32_t>(object->getCurrentVertexCount());
      for (const auto &[point, u, v] :
           {std::tuple<Ogre::Vector3, float, float>{a, 0.0F, 1.0F},
            {b, 1.0F, 1.0F}, {c, 1.0F, 0.0F}, {d, 0.0F, 0.0F}}) {
        object->position(point); object->normal(normal); object->textureCoord(u, v);
      }
      object->triangle(base, base + 1, base + 2);
      object->triangle(base, base + 2, base + 3);
    };
    face({x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}, Ogre::Vector3::UNIT_Z);
    face({x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0}, Ogre::Vector3::NEGATIVE_UNIT_Z);
    face({x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1}, Ogre::Vector3::UNIT_X);
    face({x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0}, Ogre::Vector3::NEGATIVE_UNIT_X);
    face({x0,y1,z1},{x1,y1,z1},{x1,y1,z0},{x0,y1,z0}, Ogre::Vector3::UNIT_Y);
    face({x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1}, Ogre::Vector3::NEGATIVE_UNIT_Y);
    object->end();
    object->convertToMesh("box.mesh", resourceGroup);
    scene_->destroyManualObject(object);
  }

  Ogre::MaterialPtr materialFor(const std::string &style,
                                const std::string &source) const {
    std::string role = "Surface";
    if (source.find("LUMINOUS") != std::string::npos ||
        source.find("artificial_sky") != std::string::npos)
      role = "Luminous";
    else if (source.find("WATER") != std::string::npos)
      role = "Water";
    const auto found = materials_.find(style + "/" + role);
    return found == materials_.end() ? Ogre::MaterialPtr{} : found->second;
  }

  physics::Vec3 localPosition(const physics::Vec3 logical) const noexcept {
    return {logical.x - originX_, logical.y - originY_, logical.z - originZ_};
  }

  void createBlock(const BlockPlacement &placement) {
    const BlockCell cell = placement.anchor;
    const BlockDefinition &definition = *placement.definition;
    ActiveBlock active;
    auto &pool = blockPool_[definition.mesh];
    if (!pool.empty()) {
      active = std::move(pool.back());
      pool.pop_back();
      active.node->setPosition(toOgre(localPosition(placement.rootCm)));
      active.node->setVisible(true);
      active.node->showBoundingBox(debugDraw_);
      physics::Transform transform;
      transform.position = localPosition(placement.rootCm);
      physics_->setTransform(active.body, transform);
      physics_->setEnabled(active.body, true);
      ++reusedBlocks_;
    } else {
      const std::string suffix = "Pool/" + std::to_string(allocatedBlocks_++);
      Ogre::Entity *entity = scene_->createEntity(
          "Run3LuniverseBlock/" + suffix, definition.mesh, resourceGroup);
      for (unsigned index = 0; index < entity->getNumSubEntities(); ++index) {
        Ogre::SubEntity *sub = entity->getSubEntity(index);
        const Ogre::MaterialPtr material = materialFor(definition.style,
                                                        sub->getMaterialName());
        //if (material) sub->setMaterial(material);
      }
      entity->setCastShadows(true);
      Ogre::SceneNode *node = root_->createChildSceneNode(
          "Run3LuniverseBlockNode/" + suffix,
          toOgre(localPosition(placement.rootCm)));
      node->attachObject(entity);
      node->showBoundingBox(debugDraw_);

      Ogre::MeshPtr mesh = entity->getMesh();
      auto geometry = geometry_.find(definition.mesh);
      if (geometry == geometry_.end())
        geometry = geometry_.emplace(definition.mesh, extractMesh(mesh)).first;
      physics::BodyDesc body(physics::Shape::triangleMesh(
          geometry->second.vertices, geometry->second.indices));
      body.motion = physics::BodyMotion::Static;
      body.group = physics::CollisionGroup::World;
      body.mask = physics::collisionMask(physics::CollisionGroup::Player) |
                  physics::collisionMask(physics::CollisionGroup::Dynamic) |
                  physics::collisionMask(physics::CollisionGroup::Npc) |
                  physics::collisionMask(physics::CollisionGroup::Projectile);
      body.transform.position = localPosition(placement.rootCm);
      body.metadata.type = physics::BodyType::World;
      active.node = node;
      active.entity = entity;
      active.body = physics_->createBody(body);
    }
    active.definition = &definition;
    active.placement = placement;
    active_.emplace(cell, std::move(active));
    ++stats_.generatedCells;
  }

  void retireBlock(const BlockCell cell) noexcept {
    const auto found = active_.find(cell);
    if (found == active_.end()) return;
    try {
      physics_->setEnabled(found->second.body, false);
      found->second.node->setVisible(false);
      found->second.node->showBoundingBox(false);
    } catch (...) {}
    blockPool_[found->second.definition->mesh].push_back(std::move(found->second));
    active_.erase(found);
  }

  void destroyBlockResources(ActiveBlock &block) noexcept {
    block.body.reset();
    try {
      if (block.entity) scene_->destroyEntity(block.entity);
      if (block.node) scene_->destroySceneNode(block.node);
    } catch (...) {}
    block = {};
  }

  void createTermination(const TerminationKey key) {
    const auto block = active_.find(key.cell);
    if (block == active_.end() || block->second.definition == nullptr) return;
    const BlockPort *socket = port(*block->second.definition, key.side);
    if (socket == nullptr) return;
    physics::Vec3 position = localPosition(block->second.placement.rootCm);
    const physics::Vec3 socketPosition = authoredToOgre(socket->positionCm);
    position.x += socketPosition.x;
    position.y += socketPosition.y;
    position.z += socketPosition.z;
    position.y += socket->heightCm * 0.5;
    physics::Quaternion rotation;
    const bool rotated = key.side == BlockSide::East || key.side == BlockSide::West;
    if (rotated)
      rotation = {std::sqrt(0.5), 0, std::sqrt(0.5), 0};
    ActiveTermination active;
    if (!terminationPool_.empty()) {
      active = std::move(terminationPool_.back());
      terminationPool_.pop_back();
      active.node->setPosition(toOgre(position));
      active.node->setOrientation(Ogre::Quaternion::IDENTITY);
      if (rotated) active.node->yaw(Ogre::Degree(90));
      active.node->setVisible(true);
      active.node->showBoundingBox(debugDraw_);
      physics::Transform transform;
      transform.position = position;
      transform.rotation = rotation;
      physics_->setTransform(active.body, transform);
      physics_->setEnabled(active.body, true);
      ++reusedTerminators_;
    } else {
      const std::string suffix =
          "Pool/" + std::to_string(allocatedTerminators_++);
      Ogre::Entity *entity = scene_->createEntity(
          "Run3LuniverseTerminator/" + suffix, "box.mesh", resourceGroup);
      entity->setMaterial(materials_.at("TERMINATOR/Surface"));
      entity->setCastShadows(true);
      Ogre::SceneNode *node = root_->createChildSceneNode(
          "Run3LuniverseTerminatorNode/" + suffix, toOgre(position));
      if (rotated) node->yaw(Ogre::Degree(90));
      node->attachObject(entity);
      node->showBoundingBox(debugDraw_);
      physics::BodyDesc body(
          physics::Shape::box({socket->widthCm * 0.5,
                               socket->heightCm * 0.5, 25.0}));
      body.motion = physics::BodyMotion::Static;
      body.group = physics::CollisionGroup::World;
      body.mask = physics::collisionMask(physics::CollisionGroup::All);
      body.transform.position = position;
      body.transform.rotation = rotation;
      body.metadata.type = physics::BodyType::World;
      active.node = node;
      active.entity = entity;
      active.body = physics_->createBody(body);
    }
    terminations_.emplace(key, std::move(active));
  }

  void retireTermination(const TerminationKey key) noexcept {
    const auto found = terminations_.find(key);
    if (found == terminations_.end()) return;
    try {
      physics_->setEnabled(found->second.body, false);
      found->second.node->setVisible(false);
      found->second.node->showBoundingBox(false);
    } catch (...) {}
    terminationPool_.push_back(std::move(found->second));
    terminations_.erase(found);
  }

  void destroyTerminationResources(ActiveTermination &termination) noexcept {
    termination.body.reset();
    try {
      if (termination.entity) scene_->destroyEntity(termination.entity);
      if (termination.node) scene_->destroySceneNode(termination.node);
    } catch (...) {}
    termination = {};
  }

  std::size_t foreignLightCount() const {
    std::size_t result{};
    for (const auto &[name, object] : scene_->getMovableObjects("Light")) {
      static_cast<void>(name);
      if (object != nullptr &&
          object->getName().rfind("Run3LuniverseLight/", 0) != 0)
        ++result;
    }
    return result;
  }

  void refreshLights(const physics::Vec3 player) {
    const std::size_t foreignLights = foreignLightCount();
    foreignLightsSeen_ = foreignLights;
    const std::size_t available = foreignLights >= options_.lightBudget
                                      ? 0
                                      : options_.lightBudget - foreignLights;
    std::vector<std::pair<double, BlockCell>> candidates;
    for (const auto &[cell, active] : active_) {
      if (active.definition == nullptr || !active.definition->light.enabled) continue;
      const physics::Vec3 position = localPosition(active.placement.rootCm);
      const double dx = position.x - player.x;
      const double dy = position.y - player.y;
      const double dz = position.z - player.z;
      candidates.push_back({dx * dx + dy * dy + dz * dz, cell});
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const auto &left, const auto &right) {
                if (left.first != right.first) return left.first < right.first;
                if (left.second.x != right.second.x)
                  return left.second.x < right.second.x;
                if (left.second.y != right.second.y)
                  return left.second.y < right.second.y;
                return left.second.z < right.second.z;
              });
    const std::size_t count = std::min(available, candidates.size());
    // The budget is tiny and fixed. Allocate the complete light pool during
    // the first refresh, then only retarget existing lights while streaming.
    while (lights_.size() < options_.lightBudget) {
      const std::size_t poolIndex = lights_.size();
      const std::string name = "Run3LuniverseLight/Pool/" +
                               std::to_string(poolIndex);
      Ogre::Light *light = scene_->createLight(name);
      light->setType(Ogre::Light::LT_SPOTLIGHT);
      Ogre::SceneNode *node = root_->createChildSceneNode(name + "Node");
      node->attachObject(light);
      node->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Y,
                         Ogre::Node::TS_WORLD);
      lights_.push_back({{}, node, light});
    }
    for (std::size_t index = 0; index < count; ++index) {
      const BlockCell cell = candidates[index].second;
      const BlockDefinition &definition = *active_.at(cell).definition;
      ActiveLight &active = lights_[index];
      Ogre::Light *light = active.light;
      light->setType(Ogre::Light::LT_SPOTLIGHT);
      light->setDiffuseColour(definition.light.colour[0],
                              definition.light.colour[1],
                              definition.light.colour[2]);
      light->setSpecularColour(definition.light.colour[0] * 0.25F,
                               definition.light.colour[1] * 0.25F,
                               definition.light.colour[2] * 0.25F);
      light->setSpotlightRange(Ogre::Degree(72), Ogre::Degree(105));
      light->setAttenuation(definition.light.rangeCm, 1.0F, 0.00035F, 0.000002F);
      light->setCastShadows(index == 0 &&
                            options_.lighting.shadows != rendering::ShadowQuality::Off);
      physics::Vec3 position = localPosition(active_.at(cell).placement.rootCm);
      const physics::Vec3 offset = authoredToOgre(definition.light.positionCm);
      position.x += offset.x; position.y += offset.y; position.z += offset.z;
      active.cell = cell;
      active.node->setPosition(toOgre(position));
      active.node->setVisible(true);
    }
    for (std::size_t index = count; index < lights_.size(); ++index) {
      lights_[index].light->setCastShadows(false);
      lights_[index].node->setVisible(false);
    }
    activeLightCount_ = count;
  }

  void prewarmBlockPool() {
    if (blockPoolPrewarmed_) return;
    const std::size_t generatedBefore = stats_.generatedCells;
    std::int64_t fake = -1000000;
    for (const BlockDefinition &definition : manifest_.blocks) {
      if (definition.termination) continue;
      BlockPlacement placement;
      placement.anchor = {fake--, -1000000, -1000000};
      placement.definition = &definition;
      placement.rootCm = {};
      placement.widthCells = static_cast<unsigned>(
          std::llround(definition.dimensionsCm.x / standardCellCm));
      placement.heightCells = static_cast<unsigned>(std::ceil(
          definition.dimensionsCm.z / manifest_.gridCm));
      placement.depthCells = static_cast<unsigned>(
          std::llround(definition.dimensionsCm.y / standardCellCm));
      createBlock(placement);
      retireBlock(placement.anchor);
    }
    stats_.generatedCells = generatedBefore;
    stats_.pooledBlocks = 0;
    for (const auto &[mesh, blocks] : blockPool_) {
      static_cast<void>(mesh);
      stats_.pooledBlocks += blocks.size();
    }
    blockPoolPrewarmed_ = true;
    Ogre::LogManager::getSingleton().logMessage(
        "Luniverse cache: blocks=" + std::to_string(stats_.pooledBlocks) +
        " lights=" + std::to_string(lights_.size()) +
        " terminators=" + std::to_string(terminationPool_.size()));
  }

  BlockCell centreWithHysteresis(const physics::Vec3 player) const {
    const double globalX = player.x + static_cast<double>(originX_);
    const double globalY = player.y + static_cast<double>(originY_);
    const double globalZ = player.z + static_cast<double>(originZ_);
    if (!navigationCentre_)
      return {static_cast<std::int64_t>(std::llround(globalX / standardCellCm)),
              static_cast<std::int64_t>(std::llround(globalY / manifest_.gridCm)),
              static_cast<std::int64_t>(std::llround(globalZ / standardCellCm))};
    BlockCell result = *navigationCentre_;
    constexpr double horizontalThreshold = standardCellCm * 0.70;
    const double verticalThreshold = manifest_.gridCm * 0.70;
    while (globalX - result.x * standardCellCm > horizontalThreshold) ++result.x;
    while (globalX - result.x * standardCellCm < -horizontalThreshold) --result.x;
    while (globalY - result.y * manifest_.gridCm > verticalThreshold) ++result.y;
    while (globalY - result.y * manifest_.gridCm < -verticalThreshold) --result.y;
    while (globalZ - result.z * standardCellCm > horizontalThreshold) ++result.z;
    while (globalZ - result.z * standardCellCm < -horizontalThreshold) --result.z;
    return result;
  }

  StreamPlan buildPlan(const BlockCell centre) const {
    StreamPlan plan;
    plan.centre = centre;
    const std::int64_t radius = static_cast<std::int64_t>(
        std::ceil(options_.horizontalLoadRadiusCm / standardCellCm));
    const std::int64_t verticalRadius = std::max<std::int64_t>(
        1, static_cast<std::int64_t>(
               std::ceil(options_.verticalLoadRadiusCm / manifest_.gridCm)));
    const std::int64_t maximumHorizontalCell =
        (options_.logicalExtentCm / 2) / static_cast<std::int64_t>(standardCellCm);
    const std::int64_t maximumVerticalCell =
        (options_.logicalExtentCm / 2) /
        static_cast<std::int64_t>(manifest_.gridCm);
    for (std::int64_t y = centre.y - verticalRadius;
         y <= centre.y + verticalRadius; ++y) {
      if (std::abs(y) > maximumVerticalCell) continue;
      for (std::int64_t x = centre.x - radius; x <= centre.x + radius; ++x) {
        for (std::int64_t z = centre.z - radius; z <= centre.z + radius; ++z) {
          if (std::abs(x) > maximumHorizontalCell ||
              std::abs(z) > maximumHorizontalCell)
            continue;
          const double dx = static_cast<double>(x - centre.x) * standardCellCm;
          const double dz = static_cast<double>(z - centre.z) * standardCellCm;
          if (dx * dx + dz * dz >
              options_.horizontalLoadRadiusCm * options_.horizontalLoadRadiusCm)
            continue;
          const BlockPlacement placement = generator_.placementForCell({x, y, z});
          const std::int64_t maximumX = placement.anchor.x +
              static_cast<std::int64_t>(placement.widthCells) - 1;
          const std::int64_t maximumY = placement.anchor.y +
              static_cast<std::int64_t>(placement.heightCells) - 1;
          const std::int64_t maximumZ = placement.anchor.z +
              static_cast<std::int64_t>(placement.depthCells) - 1;
          if (std::abs(placement.anchor.x) <= maximumHorizontalCell &&
              std::abs(placement.anchor.y) <= maximumVerticalCell &&
              std::abs(placement.anchor.z) <= maximumHorizontalCell &&
              std::abs(maximumX) <= maximumHorizontalCell &&
              std::abs(maximumY) <= maximumVerticalCell &&
              std::abs(maximumZ) <= maximumHorizontalCell)
            plan.blocks.emplace(placement.anchor, placement);
        }
      }
    }
    if (!options_.terminators) return plan;
    using Socket = std::tuple<std::int64_t, std::int64_t, std::int64_t, int>;
    std::set<Socket> sockets;
    for (const auto &[cell, placement] : plan.blocks) {
      static_cast<void>(cell);
      for (const BlockPort &socket : placement.definition->ports) {
        const physics::Vec3 offset = authoredToOgre(socket.positionCm);
        sockets.emplace(
            static_cast<std::int64_t>(std::llround(placement.rootCm.x + offset.x)),
            static_cast<std::int64_t>(std::llround(placement.rootCm.y + offset.y)),
            static_cast<std::int64_t>(std::llround(placement.rootCm.z + offset.z)),
            static_cast<int>(socket.side));
      }
    }
    for (const auto &[cell, placement] : plan.blocks) {
      for (const BlockPort &socket : placement.definition->ports) {
        const physics::Vec3 offset = authoredToOgre(socket.positionCm);
        const Socket mate{
            static_cast<std::int64_t>(std::llround(placement.rootCm.x + offset.x)),
            static_cast<std::int64_t>(std::llround(placement.rootCm.y + offset.y)),
            static_cast<std::int64_t>(std::llround(placement.rootCm.z + offset.z)),
            static_cast<int>(opposite(socket.side))};
        if (sockets.find(mate) == sockets.end())
          plan.terminations.insert({cell, socket.side});
      }
    }
    return plan;
  }

  void applyPlan(StreamPlan plan, const physics::Vec3 player) {
    std::vector<TerminationKey> staleEnds;
    for (const auto &[key, active] : terminations_) {
      static_cast<void>(active);
      if (plan.terminations.find(key) == plan.terminations.end())
        staleEnds.push_back(key);
    }
    for (const TerminationKey key : staleEnds) retireTermination(key);

    std::vector<BlockCell> stale;
    for (const auto &[cell, active] : active_) {
      static_cast<void>(active);
      if (plan.blocks.find(cell) == plan.blocks.end()) stale.push_back(cell);
    }
    for (const BlockCell cell : stale) retireBlock(cell);
    std::vector<BlockCell> missing;
    for (const auto &[cell, placement] : plan.blocks) {
      static_cast<void>(placement);
      if (active_.find(cell) == active_.end()) missing.push_back(cell);
    }
    std::sort(missing.begin(), missing.end(), [centre = plan.centre](
                  const BlockCell left, const BlockCell right) {
      const auto dl = std::abs(left.x - centre.x) + std::abs(left.z - centre.z);
      const auto dr = std::abs(right.x - centre.x) + std::abs(right.z - centre.z);
      if (dl != dr) return dl < dr;
      const auto dyl = std::abs(left.y - centre.y);
      const auto dyr = std::abs(right.y - centre.y);
      if (dyl != dyr) return dyl < dyr;
      if (left.x != right.x) return left.x < right.x;
      if (left.y != right.y) return left.y < right.y;
      return left.z < right.z;
    });
    for (const BlockCell cell : missing) createBlock(plan.blocks.at(cell));

    for (const TerminationKey key : plan.terminations)
      if (terminations_.find(key) == terminations_.end()) createTermination(key);
    refreshLights(player);
    lastCentre_ = plan.centre;
    ++stats_.completedStreamPlans;
    stats_.activeBlocks = active_.size();
    stats_.activeTerminators = terminations_.size();
    stats_.activeLights = activeLightCount_;
    stats_.pooledBlocks = 0;
    for (const auto &[mesh, blocks] : blockPool_) {
      static_cast<void>(mesh);
      stats_.pooledBlocks += blocks.size();
    }
    stats_.pooledTerminators = terminationPool_.size();
    stats_.originXcm = originX_; stats_.originYcm = originY_; stats_.originZcm = originZ_;
    Ogre::LogManager::getSingleton().logMessage(
        "Luniverse stream: cell=" + std::to_string(plan.centre.x) + "," +
        std::to_string(plan.centre.y) + "," +
        std::to_string(plan.centre.z) + " blocks=" + std::to_string(active_.size()) +
        " terminators=" + std::to_string(terminations_.size()) +
        " lights=" + std::to_string(activeLightCount_) +
        " pools=" + std::to_string(stats_.pooledBlocks) + "/" +
        std::to_string(stats_.pooledTerminators) +
        " reused=" + std::to_string(reusedBlocks_) + "/" +
        std::to_string(reusedTerminators_) +
        " allocated=" + std::to_string(allocatedBlocks_) + "/" +
        std::to_string(allocatedTerminators_));
  }

  void startPlanning(const BlockCell centre) {
    planning_ = std::async(std::launch::async,
                           [this, centre] { return buildPlan(centre); });
  }

  void update(const physics::Vec3 player) {
    const BlockCell centre = centreWithHysteresis(player);
    navigationCentre_ = centre;
    if (!lastCentre_) {
      applyPlan(buildPlan(centre), player);
      prewarmBlockPool();
      return;
    }
    if (planning_.valid() &&
        planning_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      StreamPlan plan = planning_.get();
      if (plan.centre == centre) applyPlan(std::move(plan), player);
    }
    if (!planning_.valid() && !(*lastCentre_ == centre)) startPlanning(centre);
    // DotScene and scripts may add or remove lights after construction.
    if (foreignLightCount() != foreignLightsSeen_) refreshLights(player);
  }

  std::optional<physics::Vec3> requestedOriginShift(const physics::Vec3 player) const {
    if (std::abs(player.x) < options_.rebaseThresholdCm &&
        std::abs(player.y) < options_.rebaseThresholdCm &&
        std::abs(player.z) < options_.rebaseThresholdCm)
      return std::nullopt;
    const auto align = [](const double value, const double grid) {
      return std::round(value / grid) * grid;
    };
    physics::Vec3 shift{align(player.x, standardCellCm),
                        align(player.y, manifest_.gridCm),
                        align(player.z, standardCellCm)};
    if (shift == physics::Vec3{}) return std::nullopt;
    return shift;
  }

  void applyOriginShift(const physics::Vec3 offset) {
    originX_ += static_cast<std::int64_t>(std::llround(offset.x));
    originY_ += static_cast<std::int64_t>(std::llround(offset.y));
    originZ_ += static_cast<std::int64_t>(std::llround(offset.z));
    const Ogre::Vector3 translation = toOgre(offset);
    for (auto &[cell, active] : active_) {
      static_cast<void>(cell);
      active.node->setPosition(active.node->getPosition() - translation);
    }
    for (auto &[key, active] : terminations_) {
      static_cast<void>(key);
      active.node->setPosition(active.node->getPosition() - translation);
    }
    for (ActiveLight &active : lights_)
      active.node->setPosition(active.node->getPosition() - translation);
    ++stats_.rebases;
    stats_.originXcm = originX_; stats_.originYcm = originY_; stats_.originZcm = originZ_;
    Ogre::LogManager::getSingleton().logMessage(
        "Luniverse floating origin: " + std::to_string(originX_) + "," +
        std::to_string(originY_) + "," + std::to_string(originZ_));
  }

  void setDebugDraw(const bool enabled) {
    debugDraw_ = enabled;
    for (auto &[cell, active] : active_) {
      static_cast<void>(cell); active.node->showBoundingBox(enabled);
    }
    for (auto &[key, active] : terminations_) {
      static_cast<void>(key); active.node->showBoundingBox(enabled);
    }
  }

  void unload() noexcept {
    if (planning_.valid()) {
      try { static_cast<void>(planning_.get()); } catch (...) {}
    }
    for (ActiveLight &active : lights_) {
      try {
        if (active.light) scene_->destroyLight(active.light);
        if (active.node) scene_->destroySceneNode(active.node);
      } catch (...) {}
    }
    lights_.clear();
    activeLightCount_ = 0;
    while (!terminations_.empty()) {
      ActiveTermination active = std::move(terminations_.begin()->second);
      terminations_.erase(terminations_.begin());
      destroyTerminationResources(active);
    }
    for (ActiveTermination &active : terminationPool_)
      destroyTerminationResources(active);
    terminationPool_.clear();
    while (!active_.empty()) {
      ActiveBlock active = std::move(active_.begin()->second);
      active_.erase(active_.begin());
      destroyBlockResources(active);
    }
    for (auto &[mesh, blocks] : blockPool_) {
      static_cast<void>(mesh);
      for (ActiveBlock &active : blocks) destroyBlockResources(active);
    }
    blockPool_.clear();
    if (root_ != nullptr) {
      try { scene_->destroySceneNode(root_); } catch (...) {}
      root_ = nullptr;
    }
    stats_.activeBlocks = stats_.activeTerminators = stats_.activeLights = 0;
    stats_.pooledBlocks = stats_.pooledTerminators = 0;
  }

  Ogre::SceneManager *scene_{};
  physics::PhysicsWorld *physics_{};
  SemiInfiniteWorldOptions options_;
  BlockManifest manifest_;
  SeededBlockGenerator generator_;
  fs::path blockRoot_;
  Ogre::SceneNode *root_{};
  std::unordered_map<std::string, Ogre::MaterialPtr> materials_;
  std::unordered_map<std::string, MeshGeometry> geometry_;
  std::unordered_map<BlockCell, ActiveBlock, CellHash> active_;
  std::unordered_map<std::string, std::vector<ActiveBlock>> blockPool_;
  std::unordered_map<TerminationKey, ActiveTermination, TerminationHash> terminations_;
  std::vector<ActiveTermination> terminationPool_;
  std::vector<ActiveLight> lights_;
  std::optional<BlockCell> lastCentre_;
  std::optional<BlockCell> navigationCentre_;
  std::future<StreamPlan> planning_;
  std::int64_t originX_{};
  std::int64_t originY_{};
  std::int64_t originZ_{};
  std::size_t foreignLightsSeen_{};
  std::size_t activeLightCount_{};
  std::uint64_t allocatedBlocks_{};
  std::uint64_t reusedBlocks_{};
  std::uint64_t allocatedTerminators_{};
  std::uint64_t reusedTerminators_{};
  bool debugDraw_{};
  bool blockPoolPrewarmed_{};
  SemiInfiniteWorldStats stats_;
};

SemiInfiniteWorld::SemiInfiniteWorld(Ogre::SceneManager &sceneManager,
                                     physics::PhysicsWorld &physics,
                                     SemiInfiniteWorldOptions options)
    : implementation_(std::make_unique<Impl>(sceneManager, physics,
                                              std::move(options))) {}
SemiInfiniteWorld::~SemiInfiniteWorld() = default;
SemiInfiniteWorld::SemiInfiniteWorld(SemiInfiniteWorld &&) noexcept = default;
SemiInfiniteWorld &SemiInfiniteWorld::operator=(SemiInfiniteWorld &&) noexcept = default;
std::optional<physics::Vec3> SemiInfiniteWorld::requestedOriginShift(
    const physics::Vec3 position) const {
  return implementation_->requestedOriginShift(position);
}
void SemiInfiniteWorld::applyOriginShift(const physics::Vec3 offset) {
  implementation_->applyOriginShift(offset);
}
void SemiInfiniteWorld::update(const physics::Vec3 position) {
  implementation_->update(position);
}
void SemiInfiniteWorld::setDebugDraw(const bool enabled) {
  implementation_->setDebugDraw(enabled);
}
void SemiInfiniteWorld::unload() noexcept { implementation_->unload(); }
const SemiInfiniteWorldStats &SemiInfiniteWorld::stats() const noexcept {
  return implementation_->stats_;
}
const BlockManifest &SemiInfiniteWorld::manifest() const noexcept {
  return implementation_->manifest_;
}

} // namespace run3::gameplay
