/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
///////////////This class is unfinished!/////////////////////////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <vector>
#include <OgreNewt.h>
#include "SoundManager.h"
//#include "npc_x.h" инклудим все нпц

class CNPC: public Singleton<CNPC>
{
public:
	CNPC();
	~CNPC();
	void init(Ogre::SceneManager *SceneMgr,OgreNewt::World* world,SoundManager* soundMgr,Ogre::Root* root); //инициализация
	void parse(); //загрузка из конфигов
	void addNpc(SceneNode* spawnNode,int type); //добавления нпц на сцену
	vector<SceneNode*> NPCs; // вектор в котором все спавны нпц
protected:
	Ogre::SceneManager *mSceneMgr; //для сцены
	OgreNewt::World* mWorld; //для физического мира
	SoundManager* mSoundMgr; //для звукоффф
	Ogre::Root* mRoot; //для создания в них frame listenera
	ConfigFile cf; //обработка всех конфигов npc
};