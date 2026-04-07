/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
///////////////This class is unfinished!/////////////////////////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "SoundManager.h"
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
// #include "npc_x.h"   

class CNPC : public Singleton<CNPC> {
public:
  CNPC();
  ~CNPC();
  void init(Ogre::SceneManager *SceneMgr, OgreNewt::World *world,
            SoundManager *soundMgr, Ogre::Root *root); // 
  void parse();                                        //   
  void addNpc(SceneNode *spawnNode, int type); //    
  vector<SceneNode *> NPCs; //      
protected:
  Ogre::SceneManager *mSceneMgr; //  
  OgreNewt::World *mWorld;       //   
  SoundManager *mSoundMgr;       //  
  Ogre::Root *mRoot;             //     frame listenera
  ConfigFile cf;                 //    npc
};