#include <run3/content/OgreAssetValidation.hpp>
#include <run3/content/XmlParser.hpp>

#include <OgreColourValue.h>
#include <OgreDataStream.h>
#include <OgreEntity.h>
#include <OgreException.h>
#include <OgreLight.h>
#include <OgreLogManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreMeshSerializer.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreScriptCompiler.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonManager.h>
#include <OgreSkeletonSerializer.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace run3 {
namespace fs = std::filesystem;

namespace {

class ValidationLogListener final : public Ogre::LogListener {
public:
  explicit ValidationLogListener(AssetReport &report) : report_(report) {}

  void setPath(fs::path path) { path_ = std::move(path); }

  void messageLogged(const Ogre::String &message, Ogre::LogMessageLevel level,
                     bool, const Ogre::String &, bool &) override {
    std::string folded = message;
    std::transform(folded.begin(), folded.end(), folded.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::tolower(character));
                   });
    if (level == Ogre::LML_CRITICAL || folded.find("compiler error") != std::string::npos ||
        folded.find("script error") != std::string::npos) {
      report_.addIssue(AssetIssueSeverity::Error, "ogre-script-parse", path_,
                       message);
    }
  }

private:
  AssetReport &report_;
  fs::path path_;
};

class DebugOutputGuard final {
public:
  explicit DebugOutputGuard(Ogre::Log *log)
      : log_(log), wasEnabled_(log != nullptr && log->isDebugOutputEnabled()) {
    if (log_ != nullptr) {
      log_->setDebugOutputEnabled(false);
    }
  }
  ~DebugOutputGuard() {
    if (log_ != nullptr) {
      log_->setDebugOutputEnabled(wasEnabled_);
    }
  }

private:
  Ogre::Log *log_{};
  bool wasEnabled_{};
};

Ogre::DataStreamPtr openDataStream(const AssetFile &file) {
  auto *stream = new std::ifstream(file.absolutePath, std::ios::binary);
  if (!*stream) {
    delete stream;
    throw std::runtime_error("Cannot open serializer input");
  }
  return Ogre::DataStreamPtr(
      new Ogre::FileStreamDataStream(file.relativePath, stream, true));
}

float attribute(const content::XmlNode &element, const char *name,
                float fallback) {
  const std::string *value = element.attribute(name);
  if (value == nullptr) {
    return fallback;
  }
  try {
    return std::stof(*value);
  } catch (const std::exception &) {
    return fallback;
  }
}

Ogre::ColourValue colour(const content::XmlNode *element,
                         const Ogre::ColourValue &fallback) {
  if (element == nullptr) {
    return fallback;
  }
  return Ogre::ColourValue(attribute(*element, "r", fallback.r),
                           attribute(*element, "g", fallback.g),
                           attribute(*element, "b", fallback.b),
                           attribute(*element, "a", fallback.a));
}

void loadRenderFixture(const fs::path &path, Ogre::SceneManager &sceneManager) {
  const content::XmlDocument document =
      content::parseXmlFile(path, content::XmlSchema::scene);
  const content::XmlNode *scene = &document.root;
  if (const auto *environment = scene->firstChild("environment")) {
      sceneManager.setAmbientLight(colour(
          environment->firstChild("colourAmbient"),
          Ogre::ColourValue(0.25F, 0.25F, 0.25F)));
  }
  const content::XmlNode *nodes = scene->firstChild("nodes");
  if (nodes == nullptr) {
    throw std::runtime_error("Fixture scene has no <scene>/<nodes> element");
  }
  std::size_t index{};
  for (const content::XmlNode &nodeElement : nodes->children) {
    if (nodeElement.name != "node") {
      continue;
    }
    const std::string *configuredName = nodeElement.attribute("name");
    const std::string name = configuredName == nullptr
                                 ? "Run3Step5Node" + std::to_string(index++)
                                 : *configuredName;
    Ogre::SceneNode *node =
        sceneManager.getRootSceneNode()->createChildSceneNode(name);
    if (const auto *position = nodeElement.firstChild("position")) {
      node->setPosition(attribute(*position, "x", 0),
                        attribute(*position, "y", 0),
                        attribute(*position, "z", 0));
    }
    if (const auto *lightElement = nodeElement.firstChild("light")) {
      const std::string *lightName = lightElement->attribute("name");
      Ogre::Light *light = sceneManager.createLight(
          lightName == nullptr ? name + "/Light" : *lightName);
      const std::string *configuredType = lightElement->attribute("type");
      const std::string type =
          configuredType == nullptr ? "point" : *configuredType;
      light->setType(type == "directional" ? Ogre::Light::LT_DIRECTIONAL
                                            : Ogre::Light::LT_POINT);
      light->setDiffuseColour(colour(
          lightElement->firstChild("colourDiffuse"),
          Ogre::ColourValue::White));
      light->setSpecularColour(colour(
          lightElement->firstChild("colourSpecular"),
          Ogre::ColourValue::White));
      if (const auto *attenuation =
              lightElement->firstChild("lightAttenuation")) {
        light->setAttenuation(attribute(*attenuation, "range", 1000),
                              attribute(*attenuation, "constant", 1),
                              attribute(*attenuation, "linear", 0),
                              attribute(*attenuation, "quadratic", 0));
      }
      node->attachObject(light);
    }
    if (const auto *entityElement = nodeElement.firstChild("entity")) {
      const std::string *primitive = entityElement->attribute("primitive");
      if (primitive == nullptr || *primitive != "cube") {
        throw std::runtime_error(
            "Fixture entities must use the assetless primitive=\"cube\"");
      }
      const std::string *entityName = entityElement->attribute("name");
      node->attachObject(sceneManager.createEntity(
          entityName == nullptr ? name + "/Cube" : *entityName,
          Ogre::SceneManager::PT_CUBE));
    }
  }
}

} // namespace

void validateOgreContent(AssetReport &report, Ogre::SceneManager &sceneManager,
                         const fs::path &renderFixture) {
  Ogre::ResourceGroupManager &resources =
      Ogre::ResourceGroupManager::getSingleton();
  const Ogre::String group = "Run3AssetValidation";
  if (resources.resourceGroupExists(group)) {
    resources.destroyResourceGroup(group);
  }
  resources.createResourceGroup(group);

  ValidationLogListener listener(report);
  Ogre::Log *log = Ogre::LogManager::getSingleton().getDefaultLog();
  DebugOutputGuard outputGuard(log);
  if (log != nullptr) {
    log->addListener(&listener);
  }

  const std::string selectedConfig = report.resourceProfile;
  for (const AssetResourceLocation &location : report.resourceLocations) {
    if (location.sourceConfig == selectedConfig) {
      try {
        resources.addResourceLocation(location.absolutePath.string(), location.type,
                                      group, false, true);
      } catch (const Ogre::Exception &error) {
        report.addIssue(AssetIssueSeverity::Error, "ogre-resource-location",
                        location.absolutePath, error.getFullDescription());
      }
    }
  }

  ++report.checks["ogre_resource_groups"];
  // Parse only the resources visible through the manifest's active profile.
  // This respects Ogre's duplicate-name resolution and avoids mixing mutually
  // exclusive high/medium/low material sets into one synthetic runtime.
  const std::string patterns[]{"*.program", "*.material", "*.compositor"};
  for (const std::string &pattern : patterns) {
    const Ogre::StringVectorPtr names =
        resources.findResourceNames(group, pattern);
    const std::set<Ogre::String> uniqueNames(names->begin(), names->end());
    for (const Ogre::String &name : uniqueNames) {
      ++report.checks["ogre_scripts_attempted"];
      listener.setPath(name);
      const std::size_t errorsBefore = report.errorCount();
      try {
        Ogre::DataStreamPtr stream = resources.openResource(name, group);
        Ogre::ScriptCompilerManager::getSingleton().parseScript(stream, group);
        if (report.errorCount() == errorsBefore) {
          ++report.checks["ogre_scripts_parsed"];
        }
      } catch (const Ogre::Exception &error) {
        report.addIssue(AssetIssueSeverity::Error, "ogre-script-parse", name,
                        error.getFullDescription());
      } catch (const std::exception &error) {
        report.addIssue(AssetIssueSeverity::Error, "ogre-script-parse", name,
                        error.what());
      }
    }
  }
  if (log != nullptr) {
    log->removeListener(&listener);
  }

  Ogre::MeshSerializer meshSerializer;
  Ogre::SkeletonSerializer skeletonSerializer;
  std::size_t sequence{};
  for (const AssetFile &file : report.files) {
    if (file.extension == ".mesh") {
      ++report.checks["ogre_mesh_files"];
      Ogre::MeshPtr mesh;
      try {
        mesh = Ogre::MeshManager::getSingleton().createManual(
            "Run3AssetValidation/Mesh/" + std::to_string(sequence++), group);
        Ogre::DataStreamPtr stream = openDataStream(file);
        meshSerializer.importMesh(stream, mesh.get());
      } catch (const std::exception &error) {
        report.addIssue(AssetIssueSeverity::Error, "mesh-readability",
                        file.relativePath, error.what());
      }
      if (mesh) {
        Ogre::MeshManager::getSingleton().remove(mesh);
      }
    } else if (file.extension == ".skeleton") {
      ++report.checks["ogre_skeleton_files"];
      Ogre::SkeletonPtr skeleton;
      try {
        skeleton = Ogre::SkeletonManager::getSingleton().create(
            "Run3AssetValidation/Skeleton/" + std::to_string(sequence++), group);
        Ogre::DataStreamPtr stream = openDataStream(file);
        skeletonSerializer.importSkeleton(stream, skeleton.get());
      } catch (const std::exception &error) {
        report.addIssue(AssetIssueSeverity::Error, "skeleton-readability",
                        file.relativePath, error.what());
      }
      if (skeleton) {
        Ogre::SkeletonManager::getSingleton().remove(skeleton);
      }
    }
  }

  try {
    loadRenderFixture(renderFixture, sceneManager);
    ++report.checks["render_fixtures_loaded"];
  } catch (const std::exception &error) {
    report.addIssue(AssetIssueSeverity::Error, "render-fixture-load",
                    renderFixture, error.what());
  }
}

} // namespace run3
