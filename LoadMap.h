/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
/*
The standart Run3 Scene Loader.Loads the dot scene with dotsceneloader or clears
it. Coded by Sgw32 2009(c)
*/
#pragma once
#include "CWeapon.h"
#include "Credits.h"
#include "DotSceneLoader.h"
#include "FacialAnimationManager.h"
#include "LightPerfomanceManager.h"
#include "MeshDecalMgr.h"
#include "MirrorManager.h"
#include "OgreConsole.h"
#include "POs.h"
#include "Player.h"
#include "Run3Batcher.h"
#include "SceneLoadOverlay.h"
#include "Sequence.h"
#include "SoundManager.h"
#include "global.h"
#include <OIS/OIS.h>
#include <Ogre.h>

class LoadMap : public Singleton<LoadMap> {
public:
  LoadMap();
  ~LoadMap();
  void init(SceneManager *SceneMgr, OgreNewt::World *world, Camera *camera,
            SoundManager *soundMgr, Player *ply);
  void LoadM(const String &map, const String &title, const String &sdir,
             const String &mapdir, const String &groupName,
             SceneManager *mSceneMgr);
  void MergeM(String map);
  void LoadPrev(const String &title, const String &sdir, const String &mapdir,
                const String &groupName, SceneManager *mSceneMgr);
  void UnloadM(SceneManager *mSceneMgr);
  void setScriptOnExit(String script) { scriptOnExit = script; }
  SceneNode *mnode;
  Ogre::DotSceneLoader *dsl;
  // const String &map, const String &title, const String &sdir,const String
  // &mapdir,const String& groupName, SceneManager *mSceneMgr
  //  Example - LoadMap("test","Ogre Test Map","/scripts","run3/maps");
private:
  SceneManager *scene;
  OgreNewt::World *mWorld;
  Camera *mCamera;
  int NumChld;
  int NumObj;
  int i;
  String obj;
  String prevMap;
  String scriptOnExit;
  String lOverlay;
  SoundManager *sound;
  Player *player;
};
