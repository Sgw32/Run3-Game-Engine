#include "PlayerCollisionForceAdder.h"

int PlayerCollisionForceAdder::userBegin()
{

   mBody01 = m_body0;
   mBody02 = m_body1;
    
  
   return true;
   
}
int PlayerCollisionForceAdder::userProcess()
{   
	Vector3 vel mBody01->getVelocity();
	if (vel!=Vector3::ZERO)
		mBody02->setVelocity(vel);
   return true;
}