/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "LoadMap.h"

class EventChangeLevel: public Singleton<EventChangeLevel>
{
public:
  EventChangeLevel();
   ~EventChangeLevel();
   void init(SceneManager* scene);
   void fire(String map);
private:
	vector<String> entc_name;
	SceneManager* mSceneMgr;
//	LoadMap* ml;
};