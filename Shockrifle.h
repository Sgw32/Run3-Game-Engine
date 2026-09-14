/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "DecalProjector.h"
#include "HUD.h"
#include "SoundManager.h"
#include <run3/input/Input.hpp>
#include <Ogre.h>
#include <OgreFrameListener.h>
#include <OgreNewt.h>
#include <list>
#include <vector>
// #include "PhysObject.h"
// #include "global.h"
#include "Bullet.h"
#include "Energy.h"
#include "POs.h"

using namespace Ogre;
using namespace std;

class Shockrifle : public FrameListener
{
public:
  Shockrifle();
  ~Shockrifle();
  void init(Ogre::Root *root, SceneManager *sceneMgr, SceneNode *weaponNode,
            Entity *ent, SoundManager *sound, OgreNewt::World *world);
  virtual bool frameStarted(const Ogre::FrameEvent &evt);
  virtual bool frameEnded(const Ogre::FrameEvent &evt);
  void Move(const run3::InputEvent &arg, Ogre::Real time);
  void In();
  void MousePress(const run3::InputEvent &arg, run3::MouseButton id);
  void MouseRelease(const run3::InputEvent &arg, run3::MouseButton id);
  void Press(const run3::InputEvent &arg);
  void Release(const run3::InputEvent &arg);
  void changeHUD() { HUD::getSingleton().ChangeHUDOverlay(myHUD, true); }
  String get_name(void) { return weapon->getName(); }
  OgreNewt::Body *get_ray_shoot();
  Ogre::Vector3 get_direction();
  /*OgreNewt::Body* get_ray_shoot();
  Ogre::Vector3 get_direction();*/
  SceneNode *mLightenNode;
  SceneNode *mTreeNode;

private:
  Ogre::Root *root;
  SceneNode *mWeaponNode;
  SceneNode *mPlayerNode;
  SceneNode *mTestNode;
  AnimationState *mWeaponState;
  ParticleSystem *lShock_t;
  ParticleEmitter *rgbEmit_t;
  /*BillboardSet* lShock_t;
  BillboardSet* lShock;
  Billboard* rgbEmit;
  Billboard* rgbEmit_t;*/
  SceneManager *mSceneMgr;
  Entity *ent;
  Entity *ent2;
  MaterialPtr mater;
  Entity *weapon;
  OgreNewt::World *mWorld;
  OgreNewt::Body *bod;
  SoundManager *soundmgr;
  ConfigFile cf;
  String shoot;
  String overlay;
  DecalProjector *d_p;
  unsigned int shoots;
  int energyid;
  bool in, out, shooting;
  bool right, right2, iright2;
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
