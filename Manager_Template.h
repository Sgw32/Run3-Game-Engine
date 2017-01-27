#pragma once
#include "Ogre.h"

using namespace Ogre;

class managerTemplate
{
public:
	managerTemplate(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
	managerTemplate(){};
	virtual ~managerTemplate(){}
	virtual void init(){}
//	virtual void create
	virtual void upd(const FrameEvent& evt){}
	virtual void cleanup(){}
};