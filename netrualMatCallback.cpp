#include "neutralMatCallback.h"

neutralMatCallback::neutralMatCallback() : OgreNewt::ContactCallback()
{
}

neutralMatCallback::~neutralMatCallback(void)
{
}


int neutralMatCallback::userProcess()
{
	if (m_body1->getName()=="DEAD_NPC")
		return 0;
	if (m_body0->getName()=="DEAD_NPC")
		return 0;
	if (m_body0->getName()=="PLAYER1")
	{
		
		return 1
	}
	m_body0->setDamage(3);
	
	return 1;
}