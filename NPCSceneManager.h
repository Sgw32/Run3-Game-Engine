/*A PART OF RUN3 GAME ENGINE
NPCScene Manager by Sgw32 2011
*/
#pragma once

#include "Ogre.h"
#include "global.h"
#include "Manager_Template.h"
#include "NPCScene.h"

using namespace Ogre;
using namespace std;
 


class NPCSceneManager : public Singleton<NPCSceneManager>, public managerTemplate
{
public:
	NPCSceneManager(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
	NPCSceneManager();
	virtual ~NPCSceneManager();
	virtual void init();
	
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	vector<NPCScene*> zps;
	NPCScene* curZP;
};