/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "DefaultAEnt.h"
#include "PlayerCollisionForceAdder.h"
#include "Run3SoundRuntime.h"
#include "global.h"
#include <OgreNewt.h>
#include "Timeshift.h"


class func_door
{
public:
	func_door();
	~func_door();
	void init(Ogre::SceneManager* mSceneMgr)
	{
		this->mSceneMgr=mSceneMgr;
	}
	void setDistanceDirection(Ogre::Real distance,Ogre::Vector3 dir = Ogre::Vector3::UNIT_Z,Ogre::Real speed = 100);

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
		Quaternion quat = get_orientation();
		/*mp=quat.getPitch().valueDegrees();
		my=quat.getYaw().valueDegrees();
		mr=quat.getRoll().valueDegrees();*/
mp=pitch;
		my=yaw;
		mr=roll;
		mp1=pitch;
		my1=yaw;
		mr1=roll;
		rotSpeed=speed;
		rotDoor=true;
	}
	void setRotSpeed(Real rspeed)
	{
		rotSpeed=rspeed;
	}
	void setup(Ogre::SceneNode* door_node);
	void setup(Ogre::SceneNode* door_node,bool box);
	OgreNewt::Body* getBodyOnTop();
	Ogre::Real getMass(OgreNewt::Body* bod);
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
	void fullOpened();
	void fullClosed();
	void setLuaScripts(String onOpenFull,String onClosedFull,String onStarted)
	{
		lDoorFopened=onOpenFull;
		lDoorFclosed=onClosedFull;
		lDoorS=onStarted;
	}
	void setLockedLua(String lua)
	{
		lLocked=lua;
	}
	void setSounds(String onopen,String onclose)
	{
		if (onopen!="nosound")
		{
			sop=true;
			mOnOpen=onopen;
		}
		if (onclose!="nosound")
		{
			scl=true;
			mOnClose=onclose;
		}
	}
	void deleteSound()
	{
		if (sound!=256)
		{
			sMgr->stopAudio(sound);
			sMgr->releaseAudio(sound);
			sound=256;
		}
	}
	void setUseInteract(bool interact)
	{
mInteract=interact;
	}
	void allowPRegulator(bool allowP)
	{
		allowPReg=allowP;
	}
	void setAcceleration(Real acc)
	{
		mAcceleration=acc;
	}
	Real getAcceleration()
	{
		return mAcceleration;
	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	bool funcrot;
	bool locked;
	bool allowPReg;
	Ogre::SceneNode* mNode;
	Ogre::AxisAlignedBox aab;
	OgreNewt::Body* door_bod;
	Ogre::Real dist;
	Ogre::Real mspeed,time,mTime;
	Ogre::Real velocity;
	Ogre::Vector3 vDirection,fPosition,sPosition;
	Ogre::Vector3 direction;
	String mName,cur_event,mOnOpen,mOnClose;
	bool nowfire,opened,closed,sop,scl;
	bool rotDoor;
	bool mInteract;
	Real mAngle;
	Real mp,my,mr,mp1,my1,mr1,mpt2,mr2,myt3;
	Real rotSpeed;
	Vector3 mAxis;
	Real mCurAngle;
	Real mAcceleration;
	Quaternion mQuatAngle;
	Quaternion mStartAngle;
	Ogre::SceneManager* mSceneMgr;
	unsigned int sound;
	SoundManager* sMgr;
	String lDoorFopened,lDoorFclosed,lDoorS,lLocked;
};