#include "enemyMatCallback.h"

enemyMatCallback::enemyMatCallback() : OgreNewt::ContactCallback()
{
}

enemyMatCallback::~enemyMatCallback(void)
{
}


int enemyMatCallback::userProcess()
{
	/*Real vel = getContactNormalSpeed();
	Vector3 force = getContactForce();
	Real y = force.y;
	if (vel>500 && onfloor)
	{
		m_body0->setDamage(vel/20);
		m_body1->setDamage(vel/20);
	}*/
	/*Vector3 force = getContactForce();
	Real y = force.y;
	if (y>1500000)
	{
m_body0->setDamage(y/75000);
		m_body1->setDamage(y/75000);
	}*/
	/*Vector3 force = getContactForce();
	Real y = force.y;
	if (y>1000)
	{
		m_body0->setDamage(y/2);
		m_body1->setDamage(y/2);
	}*/
	//LogManager::getSingleton().logMessage("contact!!!ENEMY!");'
	if (m_body1->getName()=="DEAD_NPC")
		return 0;
	if (m_body0->getName()=="DEAD_NPC")
		return 0;
	if (m_body0->getName()=="PLAYER1")
	{
		m_body0->setDamage(3);
	}
	if (m_body1->getName()=="PLAYER1")
	{
	m_body1->setDamage(3);
	}
	
	return 1;
}