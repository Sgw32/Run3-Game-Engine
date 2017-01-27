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
#include "Modulator.h"
#include "BulletHitManager.h"
#include "Timeshift.h"
//#include "PhysObject.h"
//#include "global.h"
#include "POs.h"
#include "Bullet.h"
#include "WeaponTemplate.h"
//#include "Energy.h"
#include "LuaHelperFunctions.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

using namespace Ogre;
using namespace std;

#define MINIGUN_DAMAGE 10

/*enum RIGHTBUTTON_EVENT
{
	BOMB,
	IRONSIGHT
};*/

class LaserMinigun: public FrameListener, public Weapon_Template //, public OIS::MouseListener, public OIS::KeyListener   //, Singleton<Punch>
{
public:
   LaserMinigun();
   ~LaserMinigun();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* ent,SoundManager* sound,OgreNewt::World* world);
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);
	void Move(const OIS::MouseEvent &arg,Ogre::Real time);
	void In();
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void Press(const OIS::KeyEvent &arg);
	String get_name(void){return weapon->getName();}
	void Release(const OIS::KeyEvent &arg);
	void changeHUD(){
		/*HUD* hud = HUD::getSingletonPtr();*/
		HUD::getSingleton().ChangeHUDOverlay(myHUD,false);
		if (!secondSlot_allow)
		{
			HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
		}
		else
		{
			HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
		}
		if (hasRingShift)
			HUD::getSingleton().setRingMaterial(ringMat);
		HUD::getSingleton().setCrosshairMaterial(crosshairMat);
		HUD::getSingleton().activateCrosshairRing(hasRingShift);
	}
	void release();
	void reload()
	{
		if (!secondSlot_allow)
		{
		ammo=stammo;
		}
		else
		{
			primaryAmmo=second_Slot;
			ammo=stammo-primaryAmmo;
		}
	}
	void addAmmo(unsigned int i)
	{
		if (secondSlot_allow)
		{
			ammo=ammo+i;
			HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
		}	
		else
		{
		ammo=ammo+i;
		HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
		}
	}

	void buyWeapon();
	bool hasMinigun(){return hasIt;}
	void stripWeapon(){hasIt=false;}
	void setAmmo(unsigned int i)
	{
		if (secondSlot_allow)
		{
			if (i>=second_Slot)
			{
				primaryAmmo=second_Slot;
				ammo=i-primaryAmmo;
				//HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
			}
			else
			{
				primaryAmmo=i;
				ammo=0;
				//HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
			}
			HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
		}
		else
		{
			ammo=i;
			HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
		}
	}
	OgreNewt::Body* get_ray_shoot();
	Ogre::Vector3 get_direction();
	/*OgreNewt::Body* get_ray_shoot();
	Ogre::Vector3 get_direction();*/
	SceneNode* mLightenNode;
	SceneNode* mTreeNode;
private:
	//LUA
	lua_State* pLua;

	Ogre::Root *root;
	SceneNode* mWeaponNode;
	SceneNode* mPlayerNode;
	SceneNode* mTestNode;
	AnimationState* mWeaponState;
	ParticleSystem* lShock_t;
	ParticleEmitter* rgbEmit_t;
	Light* flashLight;
	ColourValue col;
	SceneManager* mSceneMgr;
	Entity* ent;
	Entity* ent2;
	MaterialPtr mater;
	Real defOtt;
	Real curOtt;
	Real ottSpd;
	Real dmgPSHT;
	Entity* weapon;
	OgreNewt::World* mWorld;
	String myHUD;
	String luaOnShoot;
	OgreNewt::Body* bod;
	OgreNewt::Body* weapon_bod;
	SoundManager* soundmgr;
	ConfigFile cf;
	String shoot;
	String overlay;
	String ringMat,crosshairMat;
	Real fuzzy;
	Real soundEstUnload;
    unsigned int shoots;
	Real m_pause;
	int energyid;
	bool in,out,shooting;
	bool haveanim;
	bool hasMuzzleFlash;
	bool hasRingShift;
	bool hasIt;
	bool resetTimePosition;
	Vector3 scale_ind;
	bool startCount;
	bool soundLoaded;
	bool hasRing;
	Real fcnt;
	unsigned int second_Slot;
	bool secondSlot_allow;
	Vector3 modPos;
	unsigned int primaryAmmo;
	Real pauseBetweenShots;
	Real time_pause;
	Vector3 ironSight;
	bool singleShot;
	bool allowShoot;
	unsigned int ammo,stammo;
};