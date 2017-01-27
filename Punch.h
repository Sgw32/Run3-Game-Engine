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
#include "POs.h"
#include "HUD.h"


using namespace Ogre;
using namespace std;

class Punch: public FrameListener //, public OIS::MouseListener, public OIS::KeyListener   //, Singleton<Punch>
{
public:
   Punch();
   ~Punch();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* ent,SoundManager* sound,OgreNewt::World* world);
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);
	void Move(const OIS::MouseEvent &arg,Ogre::Real time);
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void Press(const OIS::KeyEvent &arg);
    void Release(const OIS::KeyEvent &arg);
	String get_name(void){return weapon->getName();}
	void changeHUD(){HUD::getSingleton().ChangeHUDOverlay(myHUD,false);}
	OgreNewt::Body* get_ray_shoot();
	Ogre::Vector3 get_direction();
private:
	Ogre::Root *root;
	SceneNode* mWeaponNode;
	SceneNode* mPlayerNode;
	AnimationState* mWeaponState;
	SceneManager* mSceneMgr;
	Entity* weapon;
	OgreNewt::World* mWorld;
	SoundManager* soundmgr;
	ConfigFile cf;
	String kick_f;
	String kick_w;
	String myHUD;
	bool shooting;
	Real shootcount;
	OgreNewt::Body* bod;
   unsigned int kick,hitb,hitw;
};