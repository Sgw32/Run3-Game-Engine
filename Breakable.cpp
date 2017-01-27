#include "Breakable.h"

Breakable::Breakable()
{

}

Breakable::~Breakable()
{

}

void Breakable::init(SceneNode* node,int health,int strength,Real mass)
{
	if (node)
		return;
	brnode = node;
	if (health==0)
		return;
	mHealth = health;
	if (strength==0)
		return;
	mStrength = strength;
	mSceneMgr = global::getSingleton().getSceneManager();
	mWorld = global::getSingleton().getWorld();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, brnode->_getWorldAABB()->getSize() );
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( brnode );
	bod->setPositionOrientation( brnode->getPosition(), brnode->getOrientation() );
	delete col;
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, brnode->_getWorldAABB()->getSize() );
	bod->setMassMatrix( mass, inertia );
	String n = brnode->getName();
	bod->setName(n);
	bod->setCustomForceAndTorqueCallback<Breakable>( &Breakable::breakable_callback ,this);
	PhysObject* po;
	po->bod = bod;
	global::getSingleton().addPhysObject(po);
	POs::getSingleton().addPO(po);
}

void brk();
{
	breaknow =true;
}

void addChunk(String model);
{

}

void breakable_callback(OgreNewt::Body* me)
{

}
