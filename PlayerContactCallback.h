#pragma once

#include <OgreNewt.h>
using namespace Ogre; 

class PlayerContactCallback :
	public OgreNewt::ContactCallback
{
public:
	PlayerContactCallback(void);
	~PlayerContactCallback(void);

	Ogre::Vector3 get_body_position(OgreNewt::Body* bod)
	{
		Quaternion orient;
		Vector3 pos;
		bod->getPositionOrientation(pos, orient);
		return pos; 
	}
	// in this example we only need the Process() function.
	int userProcess();
};

