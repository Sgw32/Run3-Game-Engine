#include "POs.h"

template<> POs *Ogre::Singleton<POs>::ms_Singleton=0;

POs::POs()
{

}

POs::~POs()
{

}

void POs::addPO(PhysObject* po)
{
	pobjects.push_back(po);
}

void POs::clear()
{
	pobjects.clear();
}