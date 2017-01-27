#include "LightPerfomanceManager.h"
#include "global.h"

template<> LightPerfomanceManager *Ogre::Singleton<LightPerfomanceManager>::ms_Singleton=0;

LightPerfomanceManager::LightPerfomanceManager()
{
}

LightPerfomanceManager::~LightPerfomanceManager()
{
}

void LightPerfomanceManager::init()
{
	LogManager::getSingleton().logMessage("Light Perfomance manager initialized");
	mSceneMgr=global::getSingleton().getSceneManager();
	player=global::getSingleton().getPlayer();
}

void LightPerfomanceManager::addLight(AxisAlignedBox area, String lightName)
{
	if (mSceneMgr->hasLight(lightName))
	lights[lightName]=area;
}

void LightPerfomanceManager::upd(const FrameEvent& evt)
{
	map<String,AxisAlignedBox>::iterator i;
	for (i=lights.begin();i!=lights.end();i++)
	{
		String name = (*i).first;
		if (mSceneMgr->hasLight(name))
		{
			Light* mLight = mSceneMgr->getLight(name);
		Vector3 ppos = ((Player*)player)->get_location();
		bool active = mLight->getVisible();
		if ((*i).second.intersects(ppos)&&!active)
		{
			mLight->setVisible(true);
		}
		if (!(*i).second.intersects(ppos)&&active)
		{
			mLight->setVisible(false);
		}	
		}

	}
}

void LightPerfomanceManager::cleanup()
{
		map<String,AxisAlignedBox>::iterator i;
	for (i=lights.begin();i!=lights.end();i++)
	{
		String name = (*i).first;
		if (mSceneMgr->hasLight(name))
		{
			mSceneMgr->getLight(name)->setVisible(true);
		}
	}
	lights.clear();
}