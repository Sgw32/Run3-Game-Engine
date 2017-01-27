#pragma once

#include <OgreNewt.h>
using namespace Ogre; 

class ButtonContactMatCallback :
	public OgreNewt::ContactCallback
{
public:
	ButtonContactMatCallback(void);
	~ButtonContactMatCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
};

