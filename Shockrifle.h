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
//#include "PhysObject.h"
//#include "global.h"
#include "POs.h"
#include "Bullet.h"
#include "Energy.h"

using namespace Ogre;
using namespace std;

class Shockrifle: public FrameListener //, public OIS::MouseListener, public OIS::KeyListener   //, Singleton<Punch>
{
public:
   Shockrifle();
   ~Shockrifle();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* ent,SoundManager* sound,OgreNewt::World* world);
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);
	void Move(const OIS::MouseEvent &arg,Ogre::Real time);
	void In();
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void Press(const OIS::KeyEvent &arg);
    void Release(const OIS::KeyEvent &arg);
	void changeHUD(){HUD::getSingleton().ChangeHUDOverlay(myHUD,true);}
	String get_name(void){return weapon->getName();}
	OgreNewt::Body* get_ray_shoot();
	Ogre::Vector3 get_direction();
	/*OgreNewt::Body* get_ray_shoot();
	Ogre::Vector3 get_direction();*/
	SceneNode* mLightenNode;
	SceneNode* mTreeNode;
private:
	Ogre::Root *root;
	SceneNode* mWeaponNode;
	SceneNode* mPlayerNode;
	SceneNode* mTestNode;
	AnimationState* mWeaponState;
	ParticleSystem* lShock_t;
	ParticleEmitter* rgbEmit_t;
	/*BillboardSet* lShock_t;
	BillboardSet* lShock;
	Billboard* rgbEmit;
	Billboard* rgbEmit_t;*/
	SceneManager* mSceneMgr;
	Entity* ent;
	Entity* ent2;
	MaterialPtr mater;
	Entity* weapon;
	OgreNewt::World* mWorld;
	OgreNewt::Body* bod;
	SoundManager* soundmgr;
	ConfigFile cf;
	String shoot;
	String overlay;
	DecalProjector* d_p;
    unsigned int shoots;
	int energyid;
	bool in,out,shooting;
	bool right,right2,iright2;
	bool nochange;
	Vector3 magnet;
	String myHUD;
	Real red_energy;
	Real green_energy;
	Real blue_energy;
	Real minusr;
	Real minusg;
	Real minusb;
};