/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>
#include <list>
#include <vector>
#include <OgreNewt.h>
#include "SoundManager.h"
#include "HUD.h"
#include "DecalProjector.h"
#include "POs.h"
#include "Bullet.h"
#include "Energy.h"

extern "C"
{
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}

using namespace Ogre;
using namespace std;

class generic_lua_weapon: public FrameListener
{
public:
   generic_lua_weapon();
   ~generic_lua_weapon();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* ent,SoundManager* sound,OgreNewt::World* world);
	void luaFuncInit();	
   virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);
	void Move(const OIS::MouseEvent &arg,Ogre::Real time);
	void In();
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void Press(const OIS::KeyEvent &arg);
    void Release(const OIS::KeyEvent &arg);
	OgreNewt::Body* get_ray_shoot();
	String get_name(void){return weapon->getName();}
	Ogre::Vector3 get_direction();
private:
	Ogre::Root *root;
	SceneNode* mWeaponNode;
	SceneNode* mPlayerNode;
	SceneNode* mTestNode;
	AnimationState* mWeaponState;
	SceneManager* mSceneMgr;
	Entity* weapon;
	OgreNewt::World* mWorld;
	OgreNewt::Body* bod;
	SoundManager* soundmgr;
	lua_State *pLuaState;
	ConfigFile cf;
	String shoot;
	String overlay;
	DecalProjector* d_p;
    unsigned int shoots;
	int energyid;
	bool in,out,shooting;
	//lua
	String keyPress;
	String keyRelease;
	String mousePressL;
	String mousePressR;
	String mousePressM;
	String mouseReleaseL;
	String mouseReleaseR;
	String mouseReleaseM;
	String mouseMove;
	String primaryAttack;
	String initLua;
	String script;
	String updateFunc;
};