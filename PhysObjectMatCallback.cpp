#include "PhysObjectMatCallback.h"
#include "Run3SoundRuntime.h"

PhysObjectMatCallback::PhysObjectMatCallback() : OgreNewt::ContactCallback()
{
	contact=false;
	first=true;
}

PhysObjectMatCallback::~PhysObjectMatCallback(void)
{
}


int PhysObjectMatCallback::userProcess()
{
	Vector3 force = getContactForce();
	//LogManager::getSingleton().logMessage(StringConverter::toString(force));
	Real y = force.y;
	if (y>100000)
	{
	//Run3SoundRuntime::getSingleton().emitSound("run3/sounds/soft1.wav",2.0f,false,get_pos(m_body1),200,50,800);
	//BulletHitManager::getSingleton().addBulletEffect(get_pos(m_body1),1.0f,5,"bulletHit3"); //TODO FIX IT
	}
	/*if (TIME_SHIFT==0.0f)
		return 0;*/
	return 1;
}


int PhysObjectMatCallback::userBegin()
{

	
	return 1;
}

void PhysObjectMatCallback::userEnd()
{

}

int RagdollMatCallback::userProcess()
{
	m_body1->freeze();
	m_body0->freeze();
	//LogManager::getSingleton().logMessage("freezing!");
	return 1;
}

int Ragdoll2RagdollMatCallback::userProcess()
{
	return 0;
}