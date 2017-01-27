#pragma once
#include "Ogre.h"
#include "PhysObject.h"
#include <vector>

class POs:public Ogre::Singleton<POs>
{
public:
	POs();
	~POs();
	void addPO(PhysObject* po);
	void clear();
	std::vector<PhysObject*> getObjects(){return pobjects;}
private:
	std::vector<PhysObject*> pobjects;
};