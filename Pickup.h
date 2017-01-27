/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OIS/OIS.h>
#include <OgreNewt.h>
#include "OgreConsole.h"
#include "global.h"
#include "PickupMatCallback.h"
#include "Timeshift.h"

using namespace Ogre;

#define DEBUG_AT_RELEASE

class Pickup
{
public:
	Pickup();
	~Pickup();
	void init(String event, String object, Vector3 pos, Quaternion rot,Vector3 scale, String meshFile,unsigned int value);
	void camera_force_callback( OgreNewt::Body* me);
	void update();
	void dispose();
	void fire();
private:
	OgreNewt::Body* bod;
	OgreNewt::World* mWorld;
	Entity* ent;
	SceneNode* node;
	SceneManager* mSceneMgr;
	String mEvent;
	String mObject;
	unsigned int mValue;
	const OgreNewt::MaterialID* mMatDefault;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* physicalMat;
	PickupMatCallback* cal;
	bool disposed;
};