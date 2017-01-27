/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OIS/OIS.h>
#include <OgreNewt.h>
#include "OgreConsole.h"
#include "PhysObjectMatCallback.h"
#include "Run3Batcher.h"
#include "Timeshift.h"

#ifndef RUN3_PHYSOBJECT_MAXDIST
#define RUN3_PHYSOBJECT_MAXDIST 10000.0f
#endif

class PhysObject
{
public:
	
	PhysObject();
	~PhysObject();
	void init(SceneManager *SceneMgr,OgreNewt::World* mWorld);
	void CreateObject(String name,String mesh,Ogre::SceneNode *node,Ogre::Real mass,bool idleanim,String idle);
	//void CreateObject(String mesh,Ogre::SceneNode *node,Ogre::Real mass,bool idleanim,String idle);
	void CreateStaticObject(String name,String mesh,Ogre::SceneNode *node);
	void CreateStaticBoxTransparent(String name,Ogre::SceneNode *node);
	void CreateStaticBox(String name,Ogre::SceneNode *node);
	void CreateStaticPhys_Box(String name,String mesh,Ogre::SceneNode *node);
	void CreateStaticCylTransparent(String name,Ogre::SceneNode *node,Real radius,Real height);
	void CreateStaticCyl(String name,Ogre::SceneNode *node,Real radius,Real height);
	void DeleteObject();
	void set_breakable(Real health,Real strength){mHealth=CurHealth=health;mStrength=strength;breakable=true;
	LogManager::getSingleton().logMessage("PhysObj is now breakable");}
	void explosive()
	{
		isExplosive=true;
	}
	void setMaxDist(Real maxDist)
	{
		dist=maxDist;
	}
	void remove_health(Real heal)
	{
		if (breakable)
		{
		CurHealth-=heal;
		if (CurHealth<0)
			breaknow=true;
		}
	}
	void setGibModel(String meshFile)
	{
		if (meshFile!="box.mesh")
			mGibFile=meshFile;
	}
	void setGibScale(Vector3 gscale)
	{
		mGibScale=gscale;
	}
	void setGibCount(unsigned int c)
	{
		gc=c;
	}
	void forceDelete();
	void brk();
	void addForce(Vector3 force);
	void setMetal(bool metal){isMetal=metal;}
	void freezeBod()
	{
		if (bod)
			bod->freeze();
	}
	void unfreezeBod()
	{
		if (bod)
		{
			bod->setVelocity(Vector3::ZERO);
			bod->unFreeze();
			bod->setVelocity(Vector3::ZERO);
		}
	}
	bool isThisPO(OgreNewt::Body* bod2);
	bool isDynamic(){return isdynamic;}
	bool frameStarted(const Ogre::FrameEvent &evt);
	void PhysObject::camera_force_callback( OgreNewt::Body* me);
	void transformToPhysObj();
	void addToBatch();
	String getName()
	{
		return bod->getName();
	}
	Quaternion get_orient(void){Vector3 pos; Quaternion orient; bod->getPositionOrientation(pos,orient); return orient;}
	Vector3 get_pos(void){Vector3 pos; Quaternion orient; bod->getPositionOrientation(pos,orient); return pos;}
	void set_pos(Vector3 pos){bod->setPositionOrientation(pos,get_orient());}
	void attachToBone2(Ogre::Node* bone,Ogre::SceneNode* parent)
	{
		attachedToBone=bone;
		attachedToParent=parent;
		deltaToObject=get_pos()-(attachedToBone->_getDerivedPosition()*attachedToParent->_getDerivedScale()+
		   attachedToParent->_getDerivedPosition());
		//plPos=bone->_getDerivedPosition();
	}

	void attachToBone(Ogre::Node* bone,Ogre::SceneNode* parent)
	{
		attachedToBone=bone;
		attachedToParent=parent;
		deltaToObject=get_pos();
		deltaQuat=get_orient();
		set_pos(attachedToBone->_getDerivedPosition());
		gravity=0;
		LogManager::getSingleton().logMessage("ATTACH TO BONE!"+StringConverter::toString(bone==0));
		//plPos=bone->_getDerivedPosition();
	}
	
	void updateAttached()
	{
		bod->setPositionOrientation(deltaToObject+attachedToParent->_getDerivedOrientation()*attachedToBone->_getDerivedPosition()*attachedToParent->_getDerivedScale()+
		   attachedToParent->_getDerivedPosition(),deltaQuat*attachedToBone->_getDerivedOrientation());
	}

	void detachFromBone()
	{
		attachedToBone=NULL;
		gravity=-98;
	}

	/*Ogre::Real get_x();
	Ogre::Real get_y();
	Ogre::Real get_z();*/
	Ogre::Entity* ent;
	OgreNewt::Body* bod;
	bool batt;
	bool dragging;
	SceneNode* nod;
	bool isMetal;
private:
	SceneManager   *scene;
	//Ogre::Vector3 size;
	Ogre::Vector3 mDirection;
	/*Ogre::Vector3 min;
	Ogre::Vector3 max;
	Ogre::Vector3 center;
	Ogre::Vector3 scale;*/
	Vector3 mGibScale;
	String mGibFile;
	Real dist;
	bool isExplosive;
	//Player* mPly;
	Vector3 mForce;
	bool applyforce,breakable,breaknow;
	Ogre::Node* attachedToBone;
	Ogre::Node* attachedToParent;
	Vector3 deltaToObject;
	Quaternion deltaQuat;
	Real mHealth,mStrength;
	Real CurHealth;
	int forcecounter;
	unsigned int gc;
	OgreNewt::World* mWorld;
	const OgreNewt::MaterialID* mMatDefault;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* physicalMat;
	PhysObjectMatCallback* mPhysCallback;
	Ogre::Vector3 inertia;
	Ogre::Real gravity;
	//AxisAlignedBox aab;
	AnimationState* mPhysState;
	bool midleanim;
	Quaternion orsave;
	bool isdynamic;
};