#pragma once

#include <OgreNewt.h>

class PickupMatCallback :
	public OgreNewt::ContactCallback
{
public:
	PickupMatCallback(void);
	~PickupMatCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
	bool contact;
};
