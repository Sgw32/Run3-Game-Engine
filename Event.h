/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "OgreConsole.h"
#include "global.h"
#include "PhysObject.h"
#include "func_door.h"
#include "CutScene.h"
#include <OIS/OIS.h>
//LUA
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
//ENDLUA
#include <vector>

class Event:public FrameListener
{
public:
	Event(Ogre::Root* root,bool on,String name);
	~Event();
	void trigger();
	String getName()
	{
		return mName;
	}
	void setChangelevel(Ogre::String map);
	void setPlayer(Vector3 dest,Real secs,String event);
	void setSpawnPhys(String name,String fileName,Vector3 dest, Real secs);
	void setOpenDoor(func_door* door,String event,Real secs);
	void setStartCutScene(CutScene* cscene);
	void addLuaScript(String script)
	{
		script_callback=true;
		luascripts.push_back(script);
	}
	void addLuaScript2(String script, Real secs);
	void interpretate_callback();
	void dispose();
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	lua_State* pLuaState;
	bool started,once,unloaded;
	bool player_callback;
	bool entc_callback;
	bool changelevel_callback;
	bool cutscene_callback;
	bool door_callback;
	bool script_callback;
	Ogre::Root* mRoot;
	CutScene* mCscene;
	vector<String> entc_name;
	vector<String> entc_meshFile;
	vector<Real> entc_secs;
	vector<Vector3> entc_spawn;
	vector<func_door*> doors;
	vector<String> d_events;
	vector<Real> d_secs;
	vector<String> luascripts;
	vector<Real> l_secs;
	vector<Real> l_secs2;
	Real maxwait;
	String mName;
	int i;
	//entc player
	Vector3 player_spawn;
	String player_event,mMap;
	Real playerwait,timeAfterrun;
};