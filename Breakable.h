#pragma once
#include "POs.h"
#include "PhysObject.h"
#include "global.h"
#include <OIS/OIS.h>
#include <Ogre.h>
#include <OgreFrameListener.h>
#include <OgreNewt.h>
#include <list>
#include <vector>

class Breakable {
public:
  Breakable();
  ~Breakable();
  void init(SceneNode *node, int health, int strength, Real mass);
  void brk();
  void addChunk(String model);
  void breakable_callback(OgreNewt::Body *me);

private:
  bool breaknow;
  int mHealth, mStrength;
  SceneManager *mSceneMgr;
  String mModel;
  OgreNewt::Body *bod;
  OgreNewt::World *mWorld;
  SceneNode *brnode;
  Entity *brent;
};