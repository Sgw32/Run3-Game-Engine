///////////////////////////////////////////////////////////////////////
//		Trigger volume without using OgreNewt! <(OgreNewt and NGD)OMG!>  //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		leave this comment if you want to use it in your own code!	 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "Player.h"
#include "global.h"
#include "PhysObject.h"
#include "EventEntC.h"
#include "func_door.h"
#include "Timeshift.h"
//#include "DotScene.h"

class Trigger
{
public:
	Trigger(Root* mRoot);
	~Trigger();
	void init(AxisAlignedBox aabb);
	void init(AxisAlignedBox aabb,Player* ply);
	void init(Vector3 pos, Vector3 scale,Player* ply);
	void init(AxisAlignedBox aabb,PhysObject* phys);
	void unload();
	void setname(String name);
	void setsleep(Real sec);
	void setEnabled(bool en)
	{
		if (en)
			LogManager::getSingleton().logMessage("Trigger is enabled!");
		else
			LogManager::getSingleton().logMessage("Trigger is disabled!");
		enabled=en;
	}
	String getname();
	void setcallback(void (*)());
	void setMultiple(String luaOnEnter, String luaOnLeave)
	{
		//once=false;
		//multiple=true;
		type=4;
		mLuaOnEnter=luaOnEnter;
		mLuaOnLeave=luaOnLeave;
	}
	void addMultipleLightOn(Light* lt);
	void addMultipleLightOff(Light* lt);

	void interpretate_callback();
	void setCallbackEnt(String name,String file, String event,Vector3 pos);
	void setCallbackPlayer(String event,Vector3 pos);
	void setCallbackChangeLevel(String map);
	void setCallbackScript(String script);
	//void setCallbackChangeLevel(String map);
	void setCallbackDoor(func_door* door,String event);
	void ent(String name,String file, String event,Vector3 pos);
	void setFootstepSound(String footstep)
	{
		footstep_inside=footstep;
	}
	void setHurt(Real dam)
	{
		damage=dam;
		hurt_callback=true;
	}
	void show_box(bool show);
		//entc event
		vector<String> entc_name;
		vector<String> entc_meshFile;
		vector<String> entc_event;
		vector<Vector3> entc_spawn;
		vector<func_door*> doors;
		vector<String> d_events;
		vector<String> scripts;
		//entc player
		Vector3 player_spawn;
		String player_event;
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	void processLightsOn();
	void processLightsOff();
	void restoreLightsOn();
	void restoreLightsOff();

	map<Light*,bool> mLightsOn;
	map<Light*,bool> mLightsOff;
	Player* tPlayer;
	AxisAlignedBox objaabb;
	PhysObject* obj;
	Entity* trig;
	SceneNode* trigsnap;
	String footstep_inside;
	void (*func)();
	Real sleep;
	Real sleep_i;
	bool state_now;
	bool state_before;
	bool nowtrigger;
	bool unloaded;
	bool once;
//	void* func;
	int type;
	int i;
	int callback_type;
	bool std_callback;
	bool player_callback;
	bool entc_callback;
	bool changelevel_callback;
	bool door_callback;
	bool script_callback;
	bool hurt_callback;
	bool enabled;
	//bool multiple;
	Real damage;
	Root* root;
	SceneManager* scene;
	String mName;
	String mMap;
	String mLuaOnEnter,mLuaOnLeave;
};