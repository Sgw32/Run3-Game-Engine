/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Energy.h"

template<> Energy *Singleton<Energy>::ms_Singleton=0;

Energy::Energy(OgreNewt::World* world)
{
	mWorld = world;
}

Energy::~Energy()
{
}

void Energy::init(Real maxDist,Real energyLoss,bool energyon)
{
mMaxDist=maxDist;
mEnergyLoss=energyLoss;
mElapsedEnergy=1.0f;
energy_on=energyon;
}

void Energy::setEnergy(bool on)
{
	energy_on=on;
}

void Energy::processBodyEnergy(OgreNewt::Body* bod)
{
	/*if (energy_on)
	{
		Vector3 iner;
		Real mass;
		String myName = bod->getName();
		bod->getMassMatrix(mass,iner);
		bod->addEnergy(mass*mElapsedEnergy);
		int i,j;
		for (i=0;i!=360;i++)
		{
			OgreNewt::Body* bod2;
			String bod3;
			bod2 = getBodyOnDegree(get_pos(bod),i);
			if (bod2 && mElapsedEnergy>0 )
			{
			if (bod2->getName()==myName)
				break;
			for (j=0;j!=added.size();i++)
			{
				if (bod2->getName()==added[i])
				{
					bod3=added[i];
				}
			}
			if (!bod3.empty())
				break;
			added.push_back(bod2->getName());
			mElapsedEnergy=mElapsedEnergy-mEnergyLoss;
				this->processBodyEnergy(bod2);
			//bod2->addEnergy(floor(bod->getEnergy()*(1.0f-mEnergyLoss)));
			}
		}
	}
	String bod3;*/
	Vector3 iner;
	Real mass;
	String myName = bod->getName();
	bod->getMassMatrix(mass,iner);
	bod->addEnergy(mass*mElapsedEnergy);
	int i,j;
	for (i=0;i!=360;i++)
	{
			OgreNewt::Body* bod2;
			String bod3;
			bod2 = getBodyOnDegree(get_pos(bod),i);
			if (bod2 && mElapsedEnergy>0 )
			{
				if (bod2->getName()==myName)
					break;
				for (j=0;j!=added.size();j++)
				{
					if (bod2->getName()==added[j])
					{
						bod3=added[j];
					}
				}
				if (!bod3.empty())
					break;
				mElapsedEnergy=mElapsedEnergy-mEnergyLoss;
				added.push_back(bod2->getName());
			}
	}
	
	
}

OgreNewt::Body* Energy::getBodyOnDegree(Ogre::Vector3 pos,int deg)
{
	Ogre::Quaternion rot(Radian(deg),Ogre::Vector3::NEGATIVE_UNIT_Z);
	Ogre::Vector3 direction = rot*Ogre::Vector3::NEGATIVE_UNIT_Z;
	Ogre::Vector3 end = pos + direction * mMaxDist;
	OgreNewt::BasicRaycast shootRay(mWorld,pos,end);
	return shootRay.getFirstHit().mBody;
}