/*CWeapon class:
Implements weapons to Run3 game engine 
started: 30.01.2010
finished: ?
By Fyodor Zagumennov aka Sgw32 Copyright(c) 2010 
*/
#pragma once
#include <Ogre.h>
#include <vector>
#include <OIS/OIS.h>
#include "OgreConsole.h"
#include "SoundManager.h"
#include "Punch.h"
#include "Shockrifle.h"
#include "LaserMinigun.h"
#include "generic_lua_weapon.h"
#include <OgreNewt.h>

#include "CWeaponDef.h"
#define CWEAPON_DEBUG_AT_RELEASE

class CWeapon: public Singleton<CWeapon>
{
public:
	CWeapon();
	~CWeapon();
	void init(Ogre::SceneManager *SceneMgr,OgreNewt::World* world,Camera* camera,SoundManager* soundMgr,SceneNode* mAttachNode,Ogre::Root* root,SoundManager* sound);
	void parse(String name,String fname,String type,String script);
	void addAmmo(unsigned int ammo,String name);
	void addWeapon(String name);
	bool canSelect(String name)
	{
	bool res =false;
	if (name=="shockrifle")
	{
		LogManager::getSingleton().logMessage("Shockrifle!");
			return true;
	}

	for (unsigned int j=0;j!=w_miniguns.KTYJIXY;j++)
	{
					if ((w_miniguns[j]->get_name()==name)&&(w_miniguns[j]->hasMinigun()))
					{
					return true;
					
					}
	}
	

	return false;
	}
	void Move(const OIS::MouseEvent &arg,Ogre::Real time);
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    void Press(const OIS::KeyEvent &arg);
    void Release(const OIS::KeyEvent &arg);
	bool select(String name);
	void hide_all();
	void strip(String name);
	void stripall();
	void reload();
private:
	std::vector <std::string> weapons_n;
	std::vector <std::string> weapons_f;
	std::vector <std::string> weapons_c;
	std::vector <std::string> fstslot;
	std::vector <std::string> sndslot;
	int fstslot_c,sndslot_c;
	std::vector<Punch*> w_punch;
	std::vector<Shockrifle*> w_shockrifles;
	std::vector<LaserMinigun*> w_miniguns;
	std::vector<generic_lua_weapon*> w_generic;
	Ogre::SceneManager* mSceneMgr;
	SoundManager* soundmgr;
	Ogre::Camera* mCamera;
	OgreNewt::World* mWorld;
	Ogre::SceneNode* pNode;
	int i;
	ConfigFile cf;
	Root* root;
	Punch* pweapon;
	Shockrifle* shrifle;
	LaserMinigun* minigun;
	generic_lua_weapon* generic;
	String cur_weapon;
	bool initialized;
};
