#include "Ladder.h"
//
//using namespace Run3;

Ladder::Ladder()
{

}

Ladder::~Ladder()
{
}

void Ladder::init(SceneNode* node,int type)
{
	mNode=node;
	mWorld=global::getSingleton().getWorld();
	mSceneMgr=global::getSingleton().getSceneManager();
	// Standart static object
	Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, node,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	bod->setType(PHYSOBJECT_LADDER);
	delete col;
}

void Ladder::init(SceneNode* node,int type,SceneNode* parent)
{
	mNode=node;
	mWorld=global::getSingleton().getWorld();
	mSceneMgr=global::getSingleton().getSceneManager();
	// Standart static object
	Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, node,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	bod->setType(PHYSOBJECT_LADDER);
	delete col;

}

bool Ladder::frameStarted(const Ogre::FrameEvent &evt)
{
	return true;
}