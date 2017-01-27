#pragma once

#include <OgreNewt.h>
using namespace Ogre; 

class enemyMatCallback :
	public OgreNewt::ContactCallback
{
public:
	enemyMatCallback(void);
	~enemyMatCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
};

