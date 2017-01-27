#include "Pickup.h"
#include "CWeapon.h"


Pickup::Pickup()
{
	#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: Pickup constructor!");
#endif
	cal = new PickupMatCallback();
		#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: PASSED CAL!");
#endif
	mMatDefault = global::getSingleton().getPlayer()->getPlayerMat();
			#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: mMatDefault");
#endif
  physicalMat= new OgreNewt::MaterialID( global::getSingleton().getWorld() );
  			#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: physicalMat");
#endif
  mMatPair = new OgreNewt::MaterialPair( global::getSingleton().getWorld(), mMatDefault, physicalMat );
  			#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: mMatPair");
#endif
  mMatPair->setContactCallback( cal );
  			#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: mMatPair2");
#endif
  disposed=false;
#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: successfully added phys-materials");
#endif
}

Pickup::~Pickup()
{
}

void Pickup::init(String event, String object, Vector3 pos, Quaternion rot,Vector3 scale, String meshFile,unsigned int value)
{
	mEvent=event;
	mObject=object;
	mSceneMgr=global::getSingleton().getSceneManager();
	mWorld=global::getSingleton().getWorld();
	node=mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,rot);
	node->setScale(scale);
	ent=mSceneMgr->createEntity( node->getName(), meshFile );
	node->attachObject(ent);
	AxisAlignedBox aab=ent->getBoundingBox();
	Vector3 min,max,center,size;
	min = aab.getMinimum()*scale;
	max = aab.getMaximum()*scale;
	center = aab.getCenter()*scale;
	size  = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	delete col;
	Ogre::Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 10, size );
	bod->setMassMatrix( 10, inertia );
	String n = node->getName();
	bod->setName(n);
	bod->setMaterialGroupID( physicalMat );
	bod->setCustomForceAndTorqueCallback<Pickup>( &Pickup::camera_force_callback ,this);
	bod->setAutoFreeze(0);
	mValue=value;
	#ifdef DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("PICKUP DEBUG: Initialization succesful");
#endif
}


void Pickup::camera_force_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,GRAVITY,0); 
   force *= mass; 
	me->addForce(force*TIME_SHIFT*TIME_SHIFT);
	if (me->getUsing() || cal->contact)
	{
		fire();
		dispose();
		#ifdef DEBUG_AT_RELEASE
		LogManager::getSingleton().logMessage("PICKUP DEBUG: Used or contact, firing and disposing");
		#endif
	}
Vector3 f2 = me->getStdAddForce();
   me->addForce( f2*TIME_SHIFT*TIME_SHIFT ); 
   me->setStandartAddForce(Ogre::Vector3::ZERO);

}

void Pickup::update()
{

}

void Pickup::dispose()
{
	if (!disposed)
	{
	delete bod;
	Ogre::MovableObject* aEnt=0;
	if (node->numAttachedObjects()>0)
		aEnt = node->getAttachedObject(0);
	if (aEnt)
	{
	if (global::getSingleton().getSceneManager()->hasEntity(aEnt->getName()))
	{
	node->detachAllObjects();
	global::getSingleton().getSceneManager()->destroySceneNode(node);
	global::getSingleton().getSceneManager()->destroyEntity(aEnt->getName());
	}
	disposed=true;
	#ifdef DEBUG_AT_RELEASE
		LogManager::getSingleton().logMessage("PICKUP DEBUG: Disposed succesful");
	#endif
	}
	}
}

void Pickup::fire()
{
	if (mEvent=="addAmmo")
	{
		CWeapon::getSingleton().addAmmo(mValue,mObject);
			#ifdef DEBUG_AT_RELEASE
		LogManager::getSingleton().logMessage("PICKUP DEBUG: Fired!");
		#endif
	}
	if (mEvent=="buyWeapon")
	{
		CWeapon::getSingleton().addWeapon(mObject);
			#ifdef DEBUG_AT_RELEASE
		LogManager::getSingleton().logMessage("PICKUP DEBUG: Fired!");
		#endif
	}
}

