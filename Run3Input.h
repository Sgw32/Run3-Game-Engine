/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "Display.h"
#include "HUD.h"
#include "Inventory.h"
#include "OgreConsole.h"
#include "Player.h"
#include "SuperFX.h"
#include "global.h"
#include <run3/input/Input.hpp>
#include <CEGUI/CEGUI.h>
#include <Ogre.h>

class Run3Input : public Ogre::Singleton<Run3Input> {
public:
  Run3Input();
  ~Run3Input();
  void init(Ogre::Overlay *menuOverlay, CEGUI::Window *shit);
  void processPress(const run3::InputEvent &arg, bool &GUIorGame, bool ingame);
  void processRelease(const run3::InputEvent &arg, bool GUIorGame, bool ingame);

public:
  Player *player;
  Ogre::Overlay *mMenuOverlay;
  CEGUI::Window *sheet;
};
