/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OIS/OIS.h>
#include <CEGUI/CEGUI.h>
#include "OgreConsole.h"
#include "HUD.h"
#include "Player.h"
#include "global.h"
#include "SuperFX.h"
#include "Display.h"
#include "Inventory.h"

class Run3Input:public Ogre::Singleton<Run3Input>
{
public: 
	Run3Input();
	~Run3Input();
	void init(Ogre::Overlay* menuOverlay,CEGUI::Window* shit);
	void processPress(const OIS::KeyEvent &arg,bool &GUIorGame,bool ingame);
    void processRelease(const OIS::KeyEvent &arg,bool GUIorGame,bool ingame);
	
public:
	Player* player;
	Ogre::Overlay* mMenuOverlay;
	CEGUI::Window* sheet;
};