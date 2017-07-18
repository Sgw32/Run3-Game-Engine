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
		//LogManager::getSingleton().logMessage("Ladder1");
		Vector3 magneticForce = get_body_position(m_body1)-get_body_position(m_body0);
		magneticForce*=0.01;
		m_body0->setStandartAddForce(Vector3(magneticForce.x,1,magneticForce.y));
		global::getSingleton().getPlayer()->setOnLadder(true);
	}

	if (m_body0->getType()==PHYSOBJECT_LADDER)
	{
		//LogManager::getSingleton().logMessage("Ladder2");
		Vector3 magneticForce = get_body_position(m_body0)-get_body_position(m_body1);
		magneticForce*=0.01;
		m_body1->setStandartAddForce(Vector3(magneticForce.x,1,magneticForce.y));
		global::getSingleton().getPlayer()->setOnLadder(true);
	}
	return 1;
}