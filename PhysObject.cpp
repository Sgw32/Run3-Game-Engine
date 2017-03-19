/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "PhysObject.h"
#include "global.h"
#include "ExplosionManager.h"
#include "GibManager.h"
#include "Run3SoundRuntime.h"

PhysObject::PhysObject()
{
	mForce = Ogre::Vector3::ZERO;
	applyforce = false;
	breakable = false;
	CurHealth=0;
	midleanim=false;
	dist=-1.0f;
	isdynamic=false;
	batt=false;
	mGibFile="box.mesh";
	attachedToBone=0;
	//root->addFrameListener(this);
	//Ogre::Root* root
}

PhysObject::~PhysObject()
{
}

void PhysObject::init(SceneManager *SceneMgr,OgreNewt::World* world)
{
  if( !SceneMgr )
  {
    throw Ogre::Exception( -1, "SceneMgr :  assertion failed at PhysObject::init","" );
  }
  scene=SceneMgr;
  if( !world )
  {
    throw Ogre::Exception( -1, "World :  assertion failed at PhysObject::init","" );
  }
  mWorld=world;
  gravity=GRAVITY;
  isExplosive=false;
 /* mPhysCallback = new PhysObjectMatCallback();
  mMatDefault = mWorld->getDefaultMaterialID();*/
  physicalMat= global::getSingleton().physicalMat;
 /* mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, physicalMat );
  mMatPair->setContactCallback( mPhysCallback );*/
	isMetal=false;
	dragging=false;
  //mMatPair->setDefaultFriction( 1.5, 1.4 );
  forcecounter=0;
}



void PhysObject::CreateStaticBoxTransparent(String name,Ogre::SceneNode *node)
{
	Ogre::Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	nod=node;
	delete col;
}

void PhysObject::addToBatch()
{
	Run3Batcher::getSingleton().addObject(ent,nod->getPosition(),nod->getScale(),nod->getOrientation());
	nod->detachAllObjects();
	scene->destroySceneNode(nod);
}

void PhysObject::CreateStaticBox(String name,Ogre::SceneNode *node)
{
	//ent=scene->createEntity( name, "box.mesh" );
	if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, "box.mesh" );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, "box.mesh" );
	}
	node->attachObject(ent);
	Ogre::Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	//bod->setMaterialGroupID( physicalMat );
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	nod=node;
	delete col;
}

void PhysObject::CreateStaticCylTransparent(String name,Ogre::SceneNode *node,Real radius,Real height)
{
	//Ogre::Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Cylinder(mWorld,radius,height);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	nod=node;
	delete col;
}

void PhysObject::CreateStaticCyl(String name,Ogre::SceneNode *node,Real radius,Real height)
{
	ent=scene->createEntity( name, "cylinder.mesh" );
	node->attachObject(ent);
	//Ogre::Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Cylinder(mWorld,radius,height);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	//bod->setMaterialGroupID( physicalMat );
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	nod=node;
	delete col;
}

/*void PhysObject::CreateObject(String mesh,Ogre::SceneNode *node,Ogre::Real mass,bool idleanim,String idle)
{
//Entity
	ent=scene->createEntity( name, mesh );
isdynamic=true;
	//ent->setCastShadows(true);
	node->attachObject(ent);
	aab=ent->getBoundingBox();
	scale=node->getScale();
	min = aab.getMinimum()*scale;
	max = aab.getMaximum()*scale;
	center = aab.getCenter()*node->getScale();
	Ogre::Vector3 Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	//Rigid body
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, Ogre::Vector3 size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	// initial position
	bod->setPositionOrientation( node->getPosition()-(center*scale), node->getOrientation() );
	delete col;
	//Make it movable!
	//mass;
	inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, Ogre::Vector3 Ogre::Vector3 size);
	bod->setMassMatrix( mass, inertia );
	String n = node->getName();
	bod->setName(n);
	bod->setMaterialGroupID( physicalMat );
	//bod->setStandardForceCallback();
	bod->setCustomForceAndTorqueCallback<PhysObject>( &PhysObject::camera_force_callback ,this);
	nod=node;
	bod->setAutoFreeze(0);
	midleanim=idleanim;
		if (idleanim)
		{
		mPhysState=ent->getAnimationState(idle);
		mPhysState->setEnabled(true);
		mPhysState->setLoop(true);
		}
}*/

void PhysObject::CreateStaticPhys_Box(String name,String mesh,Ogre::SceneNode *node)
{
		if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, mesh );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, mesh );
	}
	node->attachObject(ent);
	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 scale=node->getScale();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	//bod->setMaterialGroupID( physicalMat );
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	bod->setType(4);
	nod=node;
	delete col;
}

void PhysObject::CreateStaticObject(String name,String mesh,Ogre::SceneNode *node)
{
		if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, mesh );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, mesh );
	}
	node->attachObject(ent);
	//Ogre::Vector3 size=node->getScale();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, node,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	//bod->setMaterialGroupID( physicalMat );
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	bod->setType(4);
	nod=node;
	delete col;
}

void PhysObject::CreateObject(String name,String mesh,Ogre::SceneNode *node,Ogre::Real mass,bool idleanim,String idle)
{
	//Entity
			if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, mesh );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, mesh );
	}
isdynamic=true;
	//ent->setCastShadows(true);
	node->attachObject(ent);
	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 scale=node->getScale();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	//Rigid body
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	// initial position
	bod->setPositionOrientation( node->getPosition()-(center*scale), node->getOrientation() );
	delete col;
	//Make it movable!
	//mass;
	inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, size);
	bod->setMassMatrix( mass, inertia );
	String n = node->getName();
	bod->setName(n);
	bod->setMaterialGroupID( physicalMat );
	//bod->setStandardForceCallback();
	bod->setCustomForceAndTorqueCallback<PhysObject>( &PhysObject::camera_force_callback ,this);
	nod=node;
	bod->setAutoFreeze(0);
	midleanim=idleanim;
		if (idleanim)
		{
		mPhysState=ent->getAnimationState(idle);
		mPhysState->setEnabled(true);
		mPhysState->setLoop(true);
		}
}

bool PhysObject::frameStarted(const Ogre::FrameEvent &evt)
{
	if ((bod->getUserData()=="plc")&&(batt))
		brk();
			if (midleanim)
		{
			mPhysState->addTime(evt.timeSinceLastFrame);
		}
		if (TIME_SHIFT==0.0f)
			bod->freeze();
		if (TIME_SHIFT!=0.0f)
			bod->unFreeze();
		return true;
}

void PhysObject::transformToPhysObj()
{
	if (!isdynamic)
	{
		isdynamic=true;
		AxisAlignedBox aab=ent->getBoundingBox();
		Ogre::Vector3 scale=nod->getScale();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*nod->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	//Rigid body
	delete bod;
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( nod );
	// initial position
	bod->setPositionOrientation( nod->getPosition()-(center*scale), nod->getOrientation() );
	delete col;
	//Make it movable!
	//mass;
	inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 40*size.length(), size);
	bod->setMassMatrix( 40*size.length(), inertia );
	String n = nod->getName();
	bod->setName(n);
	bod->setMaterialGroupID( physicalMat );
	//bod->setStandardForceCallback();
	bod->setCustomForceAndTorqueCallback<PhysObject>( &PhysObject::camera_force_callback ,this);
	bod->setAutoFreeze(0);
		
	}
}

bool PhysObject::isThisPO(OgreNewt::Body* bod2)
{
	if (bod)
	{
	if (bod2->getName()==bod->getName())
		return true;
	}
	return false;
}

void PhysObject::addForce(Vector3 force)
{
	mForce = force;
	applyforce=true;
	if (breakable)
   {
	   LogManager::getSingleton().logMessage("breakable:health "+StringConverter::toString(CurHealth));
	   if ((force.length()>20002.0f)||(force.length()<19998.0f))
	   {
		CurHealth -= mStrength;
	   }
		if (CurHealth<0)
	   {
			breaknow=true;	
			brk();
	   }
   }
}

void PhysObject::brk()
{
   if (breakable)
   {
	   Vector3 p = get_pos();
	   forceDelete();
	   Run3SoundRuntime::getSingleton().emitSound("run3/sounds/wood_crate_break4.wav",2,false,p,300,300);
	   GibManager::getSingleton().spawnGibs(mGibFile,p,mGibScale,10.0f,gc,40);
	   if (isExplosive)
	   {
		ExplosionManager::getSingleton().spawnExplosionNosound(p,Vector3(2,2,2));
Run3SoundRuntime::getSingleton().emitSound("run3/sounds/explode_3.wav",4,false,p,200,50,800);
  
	   }
   }
}

void PhysObject::forceDelete()
{
		delete bod;
	   bod = 0;
	   scene->destroySceneNode(nod);
}

void PhysObject::camera_force_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,gravity,0); 
   force *= mass; 
   Vector3 plPos = mWorld->getGlobalActorPosition();
   bool use = me->getUsing();
   if (!use)
   {
   if (dist==-1.0f)
		dist=RUN3_PHYSOBJECT_MAXDIST;
   if (plPos.distance(nod->getPosition())>dist)
	   nod->setVisible(false);
   if (plPos.distance(nod->getPosition())<dist)
	   nod->setVisible(true);
	//Vector3 plPos = mPly->get_location(); 
   
	if (!dragging)
	{
   if (applyforce)
   {
	   forcecounter++;
	   if (forcecounter<2)
	   {
	   me->addForce(mForce*100*TIME_SHIFT*TIME_SHIFT);
	   }
	 /*  if (breakable)
   {
	   LogManager::getSingleton().logMessage("breakable:health "+StringConverter::toString(CurHealth));
		CurHealth -= mStrength;
   }*/
	   //me->addForce(Vector3(0,1000000,0));
	   //force = mForce;
	   if (forcecounter>2)
	   {
		   forcecounter=0;
			applyforce=false;
	   }
   }
   me->addForce( force*TIME_SHIFT*TIME_SHIFT ); 
   Vector3 forcee = me->getEnergyForce();

/**/
   me->addForce(forcee*TIME_SHIFT*TIME_SHIFT);
   me->addForce(Vector3(0,1,0));
    if (TIME_SHIFT==0)
		me->setVelocity(Vector3(0,0,0));
	}
   }
   else
   {
	if (!dragging)
	orsave=get_orient();
	dragging=!dragging;
	
   }
   if (dragging)
	   me->setPositionOrientation(plPos+global::getSingleton().getPlayer()->get_direction()*200+Vector3(0,50,0),global::getSingleton().getPlayer()->get_orientation());
   
   //as dragging
	
   if (attachedToBone)
   {
	   me->setPositionOrientation(deltaToObject+attachedToBone->_getDerivedPosition()*attachedToParent->_getDerivedScale()+
		   attachedToParent->_getDerivedPosition(),Quaternion::IDENTITY);//attachedToBone->_getDerivedOrientation());
	  // LogManager::getSingleton().logMessage(StringConverter::toString(attachedToParent->_getDerivedPosition()));
	  // LogManager::getSingleton().logMessage(StringConverter::toString(deltaToObject+attachedToBone->_getDerivedPosition()*attachedToParent->_getDerivedScale()+
		   //attachedToParent->_getDerivedPosition()));
   }
   
   //me->setOmega(Vector3(0,camera_rotation_x,0));  // To rotate the camera in the X axes
   if (breakable)
   {
	   /*if (me->getVelocity()>(force+Vector3(1,1,1))*mStrength)
	   {
		CurHealth = CurHealth - mStrength;
	   }*/
	   if (CurHealth<0)
	   {
			breaknow=true;	
			brk();
	   }
	      Vector3 f3 = me->getStdAddForce();
f3.normalise();
   if ((f3.length()!=0)&&(!applyforce))
addForce(f3*1000);
   }

}

/*Ogre::Real PhysObject::get_z()
{
	return Ogre::Vector3 size.z;
}

Ogre::Real PhysObject::get_x()
{
	return Ogre::Vector3 size.x;
}

Ogre::Real PhysObject::get_y()
{
	return Ogre::Vector3 size.y;
}*/

void PhysObject::DeleteObject()
{
/*ent->detatchFromParent();
scene->destroyEntity(ent);*/
//bod->removeForceAndTorqueCallback();
if (bod)
	delete bod;
}