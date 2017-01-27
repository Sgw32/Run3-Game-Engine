/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "OgreConsole.h"
#include <OIS/OIS.h>

class EventEntC: public Singleton<EventEntC>
{
public:
  EventEntC();
   ~EventEntC();
   void init(SceneManager* scene);
   void ent(String name,String file, String event,Vector3 pos);
   void cleanup();
private:
	vector<String> entc_name;
	SceneManager* mSceneMgr;
};