/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2015 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "FuzzyTest.h"

FuzzyTest::FuzzyTest()
{

}

FuzzyTest::~FuzzyTest()
{
}

void FuzzyTest::setup(Ogre::SceneNode* door_node,bool box)
{
	LogManager::getSingleton().logMessage("AEROXO OBJECT: NOT SUPPORTED");
}

void FuzzyTest::setup(Ogre::SceneNode* node)
{
	OgreNewt::World* mWorld = global::getSingleton().getWorld();
	door_node = node;
	Entity* ent;
	String name = node->getName();
	if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, "era50_test.mesh" );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, "era50_test.mesh" );
	}
	
	Vector3 scale = node->getScale();
	node->attachObject(ent);
	//node->setScale(scale);
	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	//Rigid body
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size);
	door_bod = new OgreNewt::Body( mWorld, col );
	door_bod->attachToNode( node );
	// initial position
	door_bod->setPositionOrientation( node->getPosition()-(center*scale), node->getOrientation() );
	delete col;
	//Make it movable!
	Real mass = 40.0f;
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, size);
	door_bod->setMassMatrix( mass, inertia );
	door_bod->setName(name);
	door_bod->setCustomForceAndTorqueCallback<FuzzyTest>( &FuzzyTest::force_callback ,this);
	door_bod->setAutoFreeze(0);

	mCpt = new copter();
	
}

void FuzzyTest::force_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
   Ogre::Vector3 force(0,0,0); 
   force *= mass; 
	Quaternion quat = get_orientation();
	//Quaternion mPitchRot = Quaternion(Degree(mCpt->getPitch()),Vector3::UNIT_X);
	//set_orientation(mPitchRot);
	Real pitch = quat.getPitch().valueDegrees();
	mCpt->setPitch(pitch);
   me->addForce(force);
	me->setOmega(Vector3(mCpt->getPitchOmega()/100));
}

void FuzzyTest::Fire(String event)
{
	
}

Vector3 FuzzyTest::get_position()
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
return oldpos;
}

void FuzzyTest::set_orientation(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat);
}

void FuzzyTest::rotateBody(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat*quat2);
}

void FuzzyTest::setname(String name)
{
}

String FuzzyTest::getname()
{
	return "";
}


void FuzzyTest::unload()
{
LogManager::getSingleton().logMessage("AEROXO OBJECT: Disposing");
delete door_bod;
	   door_bod = 0;
	Entity* ent = (Entity*)door_node->getAttachedObject(0);
	door_node->detachAllObjects();
	LogManager::getSingleton().logMessage("AEROXO OBJECT: Disposing child");

	door_node->removeAndDestroyAllChildren();

	if (ent)
	global::getSingleton().getSceneManager()->destroyEntity(ent);
	   global::getSingleton().getSceneManager()->destroySceneNode(door_node);
	   LogManager::getSingleton().logMessage("AEROXO OBJECT: Ok.");
	   if (mCpt)
	   {
		   delete mCpt;
		   mCpt=0;
	   }
}

void FuzzyTest::set_position(Vector3 pos)
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
door_bod->setPositionOrientation(pos,quat);
}

bool FuzzyTest::frameStarted(const Ogre::FrameEvent &evt)
{

	if (evt.timeSinceLastFrame>3.0f)
		return true;
	mCpt->step(evt);

	
	return true;
}
