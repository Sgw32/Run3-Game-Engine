/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "HUD.h"
#include "POs.h"
#include "SoundManager.h"
#include <run3/input/Input.hpp>
#include <Ogre.h>
#include <OgreFrameListener.h>
#include <OgreNewt.h>
#include <list>
#include <vector>

using namespace Ogre;
using namespace std;

class Punch : public FrameListener
{
public:
  Punch();
  ~Punch();
  void init(Ogre::Root *root, SceneManager *sceneMgr, SceneNode *weaponNode,
            Entity *ent, SoundManager *sound, OgreNewt::World *world);
  virtual bool frameStarted(const Ogre::FrameEvent &evt);
  virtual bool frameEnded(const Ogre::FrameEvent &evt);
  void Move(const run3::InputEvent &arg, Ogre::Real time);
  void MousePress(const run3::InputEvent &arg, run3::MouseButton id);
  void MouseRelease(const run3::InputEvent &arg, run3::MouseButton id);
  void Press(const run3::InputEvent &arg);
  void Release(const run3::InputEvent &arg);
  String get_name(void) { return weapon->getName(); }
  void changeHUD() { HUD::getSingleton().ChangeHUDOverlay(myHUD, false); }
  OgreNewt::Body *get_ray_shoot();
  Ogre::Vector3 get_direction();

private:
  Ogre::Root *root;
  SceneNode *mWeaponNode;
  SceneNode *mPlayerNode;
  AnimationState *mWeaponState;
  SceneManager *mSceneMgr;
  Entity *weapon;
  OgreNewt::World *mWorld;
  SoundManager *soundmgr;
  ConfigFile cf;
  String kick_f;
  String kick_w;
  String myHUD;
  bool shooting;
  Real shootcount;
  OgreNewt::Body *bod;
  unsigned int kick, hitb, hitw;
};
