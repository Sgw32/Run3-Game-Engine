#pragma once
#include "Ogre.h"
#include "Manager_Template.h"

using namespace Ogre;
using namespace std;

class LightPerfomanceManager: public Singleton<LightPerfomanceManager>, public managerTemplate
{
public:
	LightPerfomanceManager(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
	LightPerfomanceManager();
	virtual ~LightPerfomanceManager();
	virtual void init();
	void addLight(AxisAlignedBox area, String lightName);
//	virtual void create
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	SceneManager* mSceneMgr;
	void* player;
	map<String,AxisAlignedBox> lights;
};