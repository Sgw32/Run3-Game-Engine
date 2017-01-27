/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>

class DefaultAEnt: public Ogre::FrameListener
{
public:
	DefaultAEnt();
	~DefaultAEnt();
	void init(Ogre::SceneManager* mSceneMgr,Ogre::Root* mRoot);
	void unload();
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	Ogre::Root* root;
	Ogre::SceneManager* mSceneMgr;
};