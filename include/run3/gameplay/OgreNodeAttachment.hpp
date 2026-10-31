#pragma once

#include <OgreSceneNode.h>

namespace run3::gameplay {

// Follow a skeleton bone without detaching the node's objects or children.
// Authored sizes are in world units, independent of the NPC's model scale.
class OgreNodeAttachment {
public:
  OgreNodeAttachment(Ogre::SceneNode &node, Ogre::SceneNode &owner,
                     Ogre::Node &bone)
      : node_(&node), owner_(&owner), bone_(&bone),
        position_(node.getPosition()), orientation_(node.getOrientation()),
        authoredOrientation_(node._getDerivedOrientation()) {}

  void update() const {
    const auto position =
        owner_->convertLocalToWorldPosition(bone_->_getDerivedPosition());
    const auto orientation = owner_->_getDerivedOrientation() *
        bone_->_getDerivedOrientation() * authoredOrientation_;
    if (auto *parent = node_->getParent()) {
      node_->setPosition(parent->convertWorldToLocalPosition(position));
      node_->setOrientation(parent->convertWorldToLocalOrientation(orientation));
    } else {
      node_->setPosition(position);
      node_->setOrientation(orientation);
    }
    node_->_update(true, true);
  }

  void restore() const {
    node_->setPosition(position_);
    node_->setOrientation(orientation_);
  }

private:
  Ogre::SceneNode *node_;
  Ogre::SceneNode *owner_;
  Ogre::Node *bone_;
  Ogre::Vector3 position_;
  Ogre::Quaternion orientation_;
  Ogre::Quaternion authoredOrientation_;
};

} // namespace run3::gameplay
