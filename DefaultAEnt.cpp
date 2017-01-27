/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "DefaultAEnt.h"

DefaultAEnt::DefaultAEnt()
{
	
}

DefaultAEnt::~DefaultAEnt()
{

}

void DefaultAEnt::init(Ogre::SceneManager* scene,Ogre::Root* mRoot)
{
	root = mRoot;
	root->addFrameListener(this);
	mSceneMgr = scene;
}

void DefaultAEnt::unload()
{
	root->removeFrameListener(this);
}

bool DefaultAEnt::frameStarted(const Ogre::FrameEvent &evt)
{
return true;
}