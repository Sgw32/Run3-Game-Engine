#include "ButtonContactMatCallback.h"
#include "global.h"

ButtonContactMatCallback::ButtonContactMatCallback() : OgreNewt::ContactCallback()
{
}

ButtonContactMatCallback::~ButtonContactMatCallback(void)
{
}


int ButtonContactMatCallback::userProcess()
{	
	//if (m_body1->getMaterialGroupID()!=global::getSingleton().getWorld()->getDefaultMaterialID())
		//m_body1->use();
	//if (m_body0->getMaterialGroupID()!=global::getSingleton().getWorld()->getDefaultMaterialID())
		m_body0->use();
	//m_body0->setDamage(3);
	
	return 1;
}