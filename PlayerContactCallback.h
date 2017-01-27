#pragma once

#include <OgreNewt.h>
using namespace Ogre; 

class PlayerContactCallback :
	public OgreNewt::ContactCallback
{
public:
	PlayerContactCallback(void);
	~PlayerContactCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
};

