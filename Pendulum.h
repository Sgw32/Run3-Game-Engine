/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
//#include "DefaultAEnt.h"
//#include "PlayerCollisionForceAdder.h"
#include "Run3SoundRuntime.h"
#include "global.h"
#include <OgreNewt.h>
#include "Timeshift.h"


class Pendulum
{
public:
	Pendulum();
	~Pendulum();
	void init(Ogre::SceneManager* mSceneMgr)
	{
		positionPend=Vector3::ZERO;
		this->mSceneMgr=mSceneMgr;
	}

	void transformToFuncRot()
	{
		funcrot=true;
	}
	void setRotateDoor(Vector3 axis,Real angle)
	{
		mAxis=axis;
		rotDoor=true;
		mAngle=angle;
		mQuatAngle=Quaternion(Radian(Degree(mAngle)),axis);
		mTime=angle/mspeed;
	}
	void setRotateDoor(Real pitch,Real yaw, Real roll,Real speed)
	{
		mQuatAngle = get_orientation();
		/*mp=quat.getPitch().valueDegrees();
		my=quat.getYaw().valueDegrees();
		mr=quat.getRoll().valueDegrees();*/
		mp=pitch;
		my=yaw;
		mr=roll;
		mp1=0;
		my1=0;
		mr1=0;
		rotSpeed=speed;
		rotDoor=true;
	}
	void setRotateDoor(Real pitch,Real yaw, Real roll,Real speed,Vector3 pend)
	{
		mQuatAngle = get_orientation();
		/*mp=quat.getPitch().valueDegrees();
		my=quat.getYaw().valueDegrees();
		mr=quat.getRoll().valueDegrees();*/
		mp=pitch;
		my=yaw;
		mr=roll;
		mp1=0;
		my1=0;
		mr1=0;
		rotSpeed=speed;
		rotDoor=true;
		positionPend=pend;
	}
	void setup(Ogre::SceneNode* door_node);
	void setup(Ogre::SceneNode* door_node,bool box);


	void addParticleSystem(String name, Vector3 pos, Vector3 scale)
	{
	LogManager::getSingleton().logMessage("Pendulum: creating attached particle");
	ParticleSystem *pParticles = global::getSingleton().getSceneManager()->createParticleSystem(mNode->getName()+"particle", name);
	particleNode = mNode->createChildSceneNode(pos);
	particleNode->setScale(scale);
	particleNode->attachObject(pParticles);
	}

	void setname(String name);
	void set_position(Vector3 pos);
	void set_orientation(Quaternion quat);
	Quaternion get_orientation()
	{
		Quaternion quat;
		Vector3 pos;
		door_bod->getPositionOrientation(pos,quat);
		return quat;
	}
	void rotateBody(Quaternion quat);
	Vector3 get_position();
	String getname();
	void unload();
	void Fire(String event);
void setUseInteract(bool interact)
	{
mInteract=interact;
	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	bool funcrot;
	Ogre::SceneNode* mNode;
	Ogre::SceneNode* particleNode;
	Ogre::AxisAlignedBox aab;
	OgreNewt::Body* door_bod;
	Ogre::Real dist;
	Ogre::Real mspeed,time,mTime;
	Ogre::Real velocity;
	Ogre::Vector3 vDirection,fPosition,sPosition;
	Ogre::Vector3 direction;
	Ogre::Vector3 positionPend;
	String mName,cur_event,mOnOpen,mOnClose;
	bool nowfire,opened,closed,sop,scl;
	bool rotDoor;
	bool mInteract;
	Real mAngle;
	Real mp,my,mr,mp1,my1,mr1;
	Real rotSpeed;
	Vector3 mAxis;
	Real mCurAngle;
	Quaternion mQuatAngle;
	Quaternion mStartAngle;
	Vector3 basePos;
	Ogre::SceneManager* mSceneMgr;
	unsigned int sound;
	SoundManager* sMgr;
	String lDoorFopened,lDoorFclosed,lDoorS;
};