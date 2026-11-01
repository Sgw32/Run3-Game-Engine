#pragma once

#include <OgreCamera.h>
#include <OgreLight.h>
#include <OgreSceneNode.h>
#include <OgreShadowCameraSetup.h>
#include <OgreViewport.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace run3::rendering {

// Nested, fixed-size light-space tiles. Neither their basis nor their extent
// depends on the viewer's orientation or on visible-scene bounds.
class StableShadowCamera final : public Ogre::DefaultShadowCameraSetup {
public:
  StableShadowCamera(std::array<float, 3> radii, unsigned resolution)
      : radii_(radii), resolution_(resolution) {}

  void getShadowCamera(const Ogre::SceneManager *scene,
                       const Ogre::Camera *viewer, const Ogre::Viewport *viewport,
                       const Ogre::Light *light, Ogre::Camera *shadow,
                       std::size_t iteration) const override {
    if (light->getType() != Ogre::Light::LT_DIRECTIONAL) {
      Ogre::DefaultShadowCameraSetup::getShadowCamera(
          scene, viewer, viewport, light, shadow, iteration);
      return;
    }
    const float radius = radii_[std::min<std::size_t>(iteration, 2)];
    const auto backwards = -light->getDerivedDirection().normalisedCopy();
    const auto up = std::abs(backwards.dotProduct(Ogre::Vector3::UNIT_Y)) > 0.99F
        ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_Y;
    const Ogre::Quaternion basis(Ogre::Math::lookRotation(backwards, up));
    auto centre = basis.Inverse() * viewer->getDerivedPosition();
    const float texel = 2 * radius / static_cast<float>(resolution_);
    centre.x = std::round(centre.x / texel) * texel;
    centre.y = std::round(centre.y / texel) * texel;
    const auto position = basis * centre + backwards * (radius * 2);
    const auto configure = [&](Ogre::Camera *camera) {
      camera->setCustomViewMatrix(false);
      camera->setCustomProjectionMatrix(false);
      camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
      camera->setOrthoWindow(2 * radius, 2 * radius);
      camera->setNearClipDistance(1);
      camera->setFarClipDistance(4 * radius);
      camera->getParentSceneNode()->setPosition(position);
      camera->getParentSceneNode()->setOrientation(basis);
    };
    configure(shadow);
    // Ogre maintains a separate default culling camera for shadow textures.
    // It must use the same volume, including off-screen casters.
    if (auto *culling = dynamic_cast<Ogre::Camera *>(shadow->getCullingFrustum()))
      configure(culling);
  }

private:
  std::array<float, 3> radii_;
  unsigned resolution_;
};

} // namespace run3::rendering
