#include "Button.h"
#include "Timeshift.h"

using namespace Run3;

Run3::Button::Button()
{
	showFrame=true;
	d_b=Vector3::ZERO;
	p=false;
}

Run3::Button::~Button()
{
}

void Run3::Button::init(SceneNode* node,int type)
{
	mNode=node;
	mWorld=global::getSingleton().getWorld();
	mSceneMgr=global::getSingleton().getSceneManager();
	pLuaState=global::getSingleton().getLuaState();
	// Standart static object
	Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, node,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	delete col;
}

void Run3::Button::initMovable(SceneNode* node,int type,Real mass)
{
	mNode=node;
	mWorld=global::getSingleton().getWorld();
	mSceneMgr=global::getSingleton().getSceneManager();
	pLuaState=global::getSingleton().getLuaState();
	// Dyn from PhysObject
	Entity* ent = (Entity*)mNode->getAttachedObject(0);
	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 scale=node->getScale();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));


	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size,Quaternion::IDENTITY,center);
	bod = new OgreNewt::Body( mWorld, col );
	delete col;
	//Make it movable!
	//mass;
	Ogre::Vector3 inertia;
	inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, size);
	bod->setMassMatrix( mass, inertia );
	
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	bod->setCustomForceAndTorqueCallback<Run3::Button>( &Run3::Button::camera_force_callback ,this);
	bod->setAutoFreeze(0);
	//delete col;
}


void Run3::Button::camera_force_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,-500,0); 
   force *= mass; 

   me->addForce( force*TIME_SHIFT*TIME_SHIFT ); 
}

void Run3::Button::init(SceneNode* node,int type,SceneNode* parent)
{
	mNode=node;
	mWorld=global::getSingleton().getWorld();
	mSceneMgr=global::getSingleton().getSceneManager();
	pLuaState=global::getSingleton().getLuaState();
	// Standart static object
	Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, node,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	delete col;
	mparent=parent;
	d_b=node->getPosition()-parent->getPosition();
	p=true;

}

bool Run3::Button::frameStarted(const Ogre::FrameEvent &evt)
{
	if (showFrame)
	{
		if(p)
			bod->setPositionOrientation(mparent->getPosition()+d_b,mNode->getOrientation());
	if (bod->getUsing())
	{
		this->use();
	}
	}
	return true;
}

void Run3::Button::setCallback(String luaScript)
{
	mLuaScript=luaScript;
}

void Run3::Button::use()
{
	RunLuaScript(pLuaState,mLuaScript.c_str());
}
