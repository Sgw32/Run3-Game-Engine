#include "BlastWave.h"
#include "global.h"

BlastWave::BlastWave(String blastLog) : managerTemplate(blastLog)
{
}

BlastWave::~BlastWave()
{
}

void BlastWave::init()
{
	mWorld=global::getSingleton().getWorld();
}

void BlastWave::blast(Vector3 pos,Vector3 size,Real power)
{
	LogManager::getSingleton().logMessage("BlastWave");
	blastDist=400;
	mPower=power;
	Vector3 stdPos = pos-size/2;
	unsigned int i,j,d;
	d=2;
	for (j=0;j!=d;j++)
	{
	for (i=1;i!=360;i++)
	{
//			OgreNewt::Body* bod2;
			String bod3;
			getBodyOnDegree(stdPos+size/2*j,i);//LogManager::getSingleton().logMessage("BlastWave: scanning "+StringConverter::toString(i));
			
	}
	}
	//LogManager::getSingleton().logMessage("BlastWave");
}

void BlastWave::cleanup()
{

}

void BlastWave::upd(const FrameEvent &evt)
{

}


void BlastWave::getBodyOnDegree(Ogre::Vector3 pos,int deg)
{
	Ogre::Quaternion rot(Radian(Degree(deg)),Ogre::Vector3::NEGATIVE_UNIT_Y);
	Ogre::Vector3 direction = rot*Ogre::Vector3::NEGATIVE_UNIT_Z;
	
	Ogre::Vector3 end = pos + direction * blastDist;
	//LogManager::getSingleton().logMessage("BlastWave: direction "+StringConverter::toString(direction) + " pos "+StringConverter::toString(pos ) + " end " +StringConverter::toString(end));
	OgreNewt::BasicRaycast shootRay(mWorld,pos,end);
	/*Real harm;
	//LogManager::getSingleton().logMessage("BlastWave:harm="+StringConverter::toString(harm));
	for(int i=0;i<shootRay.getHitCount();i++)
	{
            OgreNewt::BasicRaycast::BasicRaycastInfo found = shootRay.getInfoAt(i);
			LogManager::getSingleton().logMessage("distance"+StringConverter::toString(found.mDistance));
			if (found.mDistance!=0)
			{
				
			harm = blastDist*blastDist/found.mDistance;
			}
			else
			{
				harm = blastDist*blastDist*10000;
			}

	}*/
	OgreNewt::Body* bod2 = shootRay.getFirstHit().mBody;
	if (bod2)
	{
				//LogManager::getSingleton().logMessage("BlastWave: harms body:"+bod2->getName()+" "+StringConverter::toString(harm));
				bod2->setDamage(mPower);
				bod2->setStandartAddForce(direction);
	}
}