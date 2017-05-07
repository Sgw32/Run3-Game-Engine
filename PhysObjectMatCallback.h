#pragma once

#include <OgreNewt.h>
#include "Timeshift.h"

class PhysObjectMatCallback :
	public OgreNewt::ContactCallback
{
public:
	PhysObjectMatCallback(void);
	~PhysObjectMatCallback(void);

	 int userBegin();
	int userProcess();
	void userEnd();
	void setOnFloor(bool set){onfloor=set;}
	bool onfloor;
	Ogre::Vector3 get_pos(OgreNewt::Body* bod)
	{
		Ogre::Vector3 pos;
		Ogre::Quaternion quat;
		bod->getPositionOrientation(pos,quat);
		return pos;
	}
	bool contact;
	bool first;
};

class RagdollMatCallback :
	public PhysObjectMatCallback
{
public:
	RagdollMatCallback(void) : PhysObjectMatCallback(){}
	~RagdollMatCallback(void){}

	int userProcess();
};

class Ragdoll2RagdollMatCallback :
	public PhysObjectMatCallback
{
public:
	Ragdoll2RagdollMatCallback(void) : PhysObjectMatCallback(){}
	~Ragdoll2RagdollMatCallback(void){}

	int userProcess();
};


