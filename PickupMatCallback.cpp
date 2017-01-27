#include "PickupMatCallback.h"
#include "global.h"

PickupMatCallback::PickupMatCallback() : OgreNewt::ContactCallback()
{
	contact=false;
}

PickupMatCallback::~PickupMatCallback(void)
{
}


int PickupMatCallback::userProcess()
{
	if (m_body0->getType()==NEWTBODYTYPE_PLAYER || m_body1->getType()==NEWTBODYTYPE_PLAYER )
	{
		contact=true;
	}
	return 1;
}