#pragma once

#include <OgreNewt.h>
using namespace Ogre; 

class neutralMatCallback :
	public OgreNewt::ContactCallback
{
public:
	neutralMatCallback(void);
	~neutralMatCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
};

