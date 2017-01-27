#include "PlayerContactCallback.h"
#include "global.h"

PlayerContactCallback::PlayerContactCallback() : OgreNewt::ContactCallback()
{
}

PlayerContactCallback::~PlayerContactCallback(void)
{
}


int PlayerContactCallback::userProcess()
{
	/*Real vel = getContactNormalSpeed();
	Vector3 force = getContactForce();
	Real y = force.y;
	if (vel>500 && onfloor)
	{
		m_body0->setDamage(vel/20);
		m_body1->setDamage(vel/20);
	}*/
	Vector3 force = getContactForce();
	Real y = force.y;
	if (y>1500000)
	{
m_body0->setDamage(y/75000);
		m_body1->setDamage(y/75000);
	}
	m_body0->setUserData("plc");
	m_body1->setUserData("plc");

	if (m_body1->getType()==PHYSOBJECT_LADDER)
	{
		LogManager::getSingleton().logMessage("LADDER!");
		//m_body0->addForce(Vector3(0,1000,0));
		m_body0->setStandartAddForce(Vector3(0,1,0));//m_body0->getStdAddForce()+Vector3(0,10000,0));
	}

	if (m_body0->getType()==PHYSOBJECT_LADDER)
	{
		LogManager::getSingleton().logMessage("LADDER!");
		//m_body1->addForce(Vector3(0,1000,0));

		m_body1->setStandartAddForce(Vector3(0,1,0));
	}
	/*Vector3 force = getContactForce();
	Real y = force.y;
	if (y>1000)
	{
		m_body0->setDamage(y/2);
		m_body1->setDamage(y/2);
	}*/
	return 1;
}