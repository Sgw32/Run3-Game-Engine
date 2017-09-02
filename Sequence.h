///////////////////////////////////////////////////////////////////////
//		Sequnce class for Run3										 //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		use for implementing buttons,npcs,etc						 //
//		This class provides lua support for each detail of engine!	 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "Player.h"
#include "PhysObject.h"
#include "SoundManager.h"
#include "tinyxml.h"
#include "Trigger.h"
#include "Loader.h"
#include "func_door.h"
#include "FadeListener.h"
#include "global.h"
#include "EventEntC.h"
#include "Event.h"
#include "CutScene.h"
#include "Computer.h"
#include "SequenceLuaCallback.h"
#include "NPCManager.h"
#include "Button.h"
#include "Pickup.h"
#include "Run3SoundRuntime.h"
#include "Sequencemacro.h"
#include "LensFlare.h"
#include "MusicPlayer.h"
#include "Train.h"
#include "SeqScript.h"
#include "Timer.h"
#include "DarkZone.h"
#include "ElementGroup.h"
#include "ElementGroupGenerator.h"
#include "Pendulum.h"
#include "Ladder.h"
#include "SkyManager.h"
#include "Fire.h"
#include "FuzzyTest.h"
#include "CustomSceneManager.h"
// #include <OgreBillboardSet.h>
//#include "EventChangeLevel.h"


extern std::list<RenderableOcclusionPair*> g_RenderableOcclusionPairList;

#define SEQUENCE_DEBUG_AT_RELEASE

class Sequence:public Singleton<Sequence>, FrameListener
{
public:
   Sequence();
   ~Sequence();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SoundManager* sound);
   void init_reload();
   void SetSceneSeq(String seq);
    void SetSceneSeq(TiXmlElement *SequenceRoot);
   void interpretate(String obj,String command);
	//2 main categories
   void processAdents(TiXmlElement *XMLNode);
   void processEvents(TiXmlElement *XMLNode);
	//in <adents>
   void processTrigger(TiXmlElement *XMLNode);
   void processDoor(TiXmlElement *XMLNode);
   void processRot(TiXmlElement *XMLNode);
   void processCutScene(TiXmlElement *XMLNode);
   void processSeqScript(TiXmlElement *XMLNode);
   void processEvent(TiXmlElement *XMLNode);
   void processComputer(TiXmlElement *XMLNode);
   void processPickup(TiXmlElement *XMLNode);
   void processPendulum(TiXmlElement *XMLNode);
   void processLuaScript(TiXmlElement *XMLNode);
   void processLuaScriptOnExit(TiXmlElement *XMLNode);
   void processNPC(TiXmlElement *XMLNode);
   void processNPCGroup(TiXmlElement *XMLNode);
   void processButton(TiXmlElement *XMLNode);
   void processLadder(TiXmlElement *XMLNode);
   void processTrain(TiXmlElement *XMLNode);
   void processTimer(TiXmlElement *XMLNode);
   void processFire(TiXmlElement *XMLNode); //added by Sgw32
  // void processFuzzyObject(TiXmlElement *XMLNode); //added by Sgw32
	void processDarkZone(TiXmlElement *XMLNode);
	//AEROXO
	void processFuzzyObject(TiXmlElement *XMLNode);
   //void processTree
	//in <events>
	void processTriggerEvent(TiXmlElement *XMLNode);
	void processCutSceneEvent(TiXmlElement *XMLNode);
	void processSeqScriptEvent(TiXmlElement *XMLNode);
	void processLensFlare(TiXmlElement *XMLNode);
	
	void runCutScene(String name)
	{
		CutScene* cscene=0;
		for (unsigned int i=0; i!=cutscenes.size();i++)
	{
		if (cutscenes[i]->getname() == name)
		{
			cscene = cutscenes[i];
		}
	}
	if (cscene)
		cscene->start();
	}

	void s_trainReverse(String name)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
	{
		if (trains[i]->getName() == name)
		{
			cscene = trains[i];
		}
	}
		cscene->reverse();
		if (cscene)
			cscene->start();
	}

	void s_trainStart(String name)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
	{
		if (trains[i]->getName() == name)
		{
			cscene = trains[i];
		}
	}
	if (cscene)
		cscene->start();
	}

	void s_setAccTrain(String name,Real acc)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
		{
			if (trains[i]->getName() == name)
			{
				cscene = trains[i];
			}
		}
		if (cscene)
			cscene->setAcceleration(acc);
	}

	void s_trainStop(String name)
	{
		Train* cscene;
		cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
	{
		if (trains[i]->getName() == name)
		{
			cscene = trains[i];
		}
	}
		if (cscene)
			cscene->stop();
	}

	void s_powerComputer(String name)
	{
		Computer* cscene;
		cscene=0;
		for (unsigned int i=0; i!=computers.size();i++)
		{
			if (computers[i]->getName() == name)
			{
				cscene = computers[i];
			}
		}
		if (cscene)
			cscene->power();
	}

	void s_setSpeedTrain(String name,Real s)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
	{
		if (trains[i]->getName() == name)
		{
			cscene = trains[i];
		}
	}
	if (cscene)
	cscene->mSpeed=s;
	}

	void s_enableSaveMode(String name)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
		{
			if (trains[i]->getName() == name)
			{
				cscene = trains[i];
			}
		}
		if (cscene)
		cscene->enablePositionSaveMode();
	}

	void s_disableSaveMode(String name)
	{
		Train* cscene=0;
		for (unsigned int i=0; i!=trains.size();i++)
	{
		if (trains[i]->getName() == name)
		{
			cscene = trains[i];
		}
	}
	if (cscene)
	cscene->disablePositionSaveMode();
	}



	 static void Gluk();
	static void processEventEntc();


	static int openDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().openDor(tex);
		}
		return 1;
	}

	static int closeDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().closeDor(tex);
		}
		return 1;
	}

	static int lockDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().lockDor(tex);
		}
		return 1;
	}

	static int unlockDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().unlockDor(tex);
		}
		return 1;
	}

	static int startCutScene(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().startCutScene(tex);
		}
		return 1;
	}
	static int godmode(lua_State* pL)
		{
	global::getSingleton().getPlayer()->god=true;
		
		return 1;
	}
	static int startSeqScript(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			CutScene* cscene;
			for (unsigned int i=0; i!=Sequence::getSingleton().cutscenes.size();i++)
	{
		if (Sequence::getSingleton().cutscenes[i]->getname() == tex)
		{
			cscene = Sequence::getSingleton().cutscenes[i];
		}
	}
		cscene->start();
		}
		return 1;
	}

	static int enableTimer(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Run3Timer* cscene=0;
			for (unsigned int i=0; i!=Sequence::getSingleton().timers.size();i++)
			{
				if (Sequence::getSingleton().timers[i]->mName == tex)
				{
					cscene = Sequence::getSingleton().timers[i];
				}
			}
			if (cscene)
			cscene->enable();
		}
		return 1;
	}

	static int rotateFuzzyObject(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			LogManager::getSingleton().logMessage("Rotation...");
			Quaternion tex = Quaternion(Radian(Degree(StringConverter::parseReal(lua_tostring(pL, 2)))),Vector3(1,0,0))*Quaternion(Radian(Degree(StringConverter::parseReal(lua_tostring(pL, 3)))),Vector3(0,0,1));
			String name = lua_tostring(pL, 1);
			FuzzyTest* tst2=0;
			LogManager::getSingleton().logMessage("Ok.");
			for (unsigned int i=0; i!=Sequence::getSingleton().fobj.size();i++)
			{
				LogManager::getSingleton().logMessage("Name: "+Sequence::getSingleton().fobj[i]->getname());
				if (Sequence::getSingleton().fobj[i]->getname() == name)
				{
					tst2 = (FuzzyTest*)Sequence::getSingleton().fobj[i];
					LogManager::getSingleton().logMessage("Applying rotation...");
				}
			}
			if (tst2)
			{
				LogManager::getSingleton().logMessage("Ok.");
				FuzzyTest2* tst = (FuzzyTest2*)tst2;
				tst->rotateBody(tex);
			}		
		}
		return 1;
	}

	static int setNearClipDist(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			global::getSingleton().getCamera()->setNearClipDistance(tex);
		}
		return 1;
	}
	static int resetNearClipDist(lua_State* pL)
	{
		/*int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{*/
			//Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			global::getSingleton().getPlayer()->resetNearClip();
		//}
		return 1;
	}

	static int setFarClipDist(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			global::getSingleton().getCamera()->setFarClipDistance(tex);
		}
		return 1;
	}

	static int setFov(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			global::getSingleton().getCamera()->setFOVy(Degree(tex));
		}
		return 1;
	}

	static int getFov(lua_State* pL)
	{
		lua_pushnumber(pL,global::getSingleton().getCamera()->getFOVy().valueDegrees());
		return 1;
	}

	static int exitAllComputers(lua_State* pL)
	{
		vector<Computer*>::iterator itc = Sequence::getSingleton().computers.begin();
		//for (itc = Sequence::getSingleton().computers.begin();itc!=Sequence::getSingleton().computers.end();itc++)
			(*itc)->logoff();
		return 1;
	}

	static int processDotScene(lua_State* pL)
	{
		
		return 1;
	}

	static int processSequence(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().SetSceneSeq(tex);
		}
		return 1;
	}

	static int resetFov(lua_State* pL)
	{ 
		global::getSingleton().getPlayer()->resetFov();
		return 1;
	}

	static int resetFarClipDist(lua_State* pL)
	{ 
		global::getSingleton().getPlayer()->resetFarClip();
		return 1;
	}

	static int shockCutScene(lua_State* pL)
	{ 
		global::getSingleton().getPlayer()->shockPlayerCutScene();
		return 1;
	}

	static int SkyX_setTimeMultiplier(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setTimeMultiplier(tex);
		}
		return 1;
	}

	static int SkyX_setHeightPosition(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setHeightPosition(tex);
		}
		return 1;
	}

	static int SkyX_setOuterRadius(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setOuterRadius(tex);
		}
		return 1;
	}

	static int SkyX_setInnerRadius(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setInnerRadius(tex);
		}
		return 1;
	}

	static int SkyX_setExposure(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setExposure(tex);
		}
		return 1;
	}

	static int SkyX_setMie(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setMie(tex);
		}
		return 1;
	}

	static int SkyX_setRayleighMultiplier(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			SkyManager::getSingleton().setRayleighMultiplier(tex);
		}
		return 1;
	}

	static int SkyX_setSampleNumber(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			int tex = StringConverter::parseInt(lua_tostring(pL, 1));
			SkyManager::getSingleton().setSampleNumber(tex);
		}
		return 1;
	}

	static int SkyX_updateParameters(lua_State* pL)
	{
		SkyManager::getSingleton().updateParameters();
		return 1;
	}

	static int disableTimer(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Run3Timer* cscene=0;
			for (unsigned int i=0; i!=Sequence::getSingleton().timers.size();i++)
			{
				if (Sequence::getSingleton().timers[i]->mName == tex)
				{
					cscene = Sequence::getSingleton().timers[i];
				}
			}
			if (cscene)
			cscene->disable();
		}
		return 1;
	}

	static int toggleTimer(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Run3Timer* cscene;
			for (unsigned int i=0; i!=Sequence::getSingleton().timers.size();i++)
			{
				if (Sequence::getSingleton().timers[i]->mName == tex)
				{
					cscene = Sequence::getSingleton().timers[i];
				}
			}
			cscene->toggle();
		}
		return 1;
	}


	static int createParticleSystem(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=4)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4))
		{
			String tex = lua_tostring(pL, 1);
			Vector3 pos = StringConverter::parseVector3(lua_tostring(pL, 3));
			Vector3 scale = StringConverter::parseVector3(lua_tostring(pL, 4));
			ParticleSystem* p = global::getSingleton().getSceneManager()->createParticleSystem(lua_tostring(pL, 2),tex);
			SceneNode* n = global::getSingleton().getSceneManager()->getRootSceneNode()->createChildSceneNode(pos);
			n->setScale(scale);
			n->attachObject(p);
		}
		return 1;
	}

	static int setSkyBox(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			if (tex=="none")
			{
				global::getSingleton().getSceneManager()->setSkyBox(false, tex, 6000, true, Quaternion::IDENTITY, "General");
			}
			else
			{		
				global::getSingleton().getSceneManager()->setSkyBox(true, tex, 6000, true, Quaternion::IDENTITY, "General");
			}
		}
		return 1;
	}

	static int deleteParticleSystem(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			if (global::getSingleton().getSceneManager()->hasParticleSystem(tex))
			{
			ParticleSystem* p = global::getSingleton().getSceneManager()->getParticleSystem(tex);
			SceneNode* n = p->getParentSceneNode();
			if (p)
				global::getSingleton().getSceneManager()->destroyParticleSystem(p);
		if (n)
		global::getSingleton().getSceneManager()->destroySceneNode(n);
			}

		}
		return 1;
	}

	static int transformToPhys(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
					pobj[i]->transformToPhysObj();

			}
			

		}
		return 1;
	}

	static int freezeBod(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if ((pobj[i]->getName()==tex)||(tex=="*"))
					pobj[i]->freezeBod();

			}
			

		}
		return 1;
	}

	static int unfreezeBod(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
					pobj[i]->unfreezeBod();

			}
			

		}
		return 1;
	}

	static int forceDelete(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
					pobj[i]->DeleteObject();

			}
			

		}
		return 1;
	}

	static int hideHUD(lua_State* pL)
	{
		HUD::getSingleton().Hide();
		return 1;
	}

	static int disableHUD(lua_State* pL)
	{
		HUD::getSingleton().Hide();
		HUD::getSingleton().setHUDAllowed(false);
		return 1;
	}

	static int enableHUD(lua_State* pL)
	{
		HUD::getSingleton().setHUDAllowed(true);
		HUD::getSingleton().Show();
		return 1;
	}

	static int stripWeapons(lua_State* pL)
	{
		CWeapon::getSingleton().stripall();
		return 1;
	}
	static int toggleDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().toggleDoor(tex);
		}
		return 1;
	}

	static int setAccDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			Real acc = StringConverter::parseReal(lua_tostring(pL, 2));
			SequenceLuaCallback::getSingleton().setAccDoor(tex,acc);
		}
		return 1;
	}

	static int deleteDoor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			vector<func_door*> pobj;
			pobj = Sequence::getSingleton().doors;
			vector<func_door*>::iterator i;
			for (i=pobj.begin();i!=pobj.end();i++)
			{
				if ((*i)->getname()==tex)
				{
					(*i)->unload();
					delete (*i);
					pobj.erase(i);
					break;
				}
			}
		}
		return 1;
	}

	static int setRotSpeed(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			Real tex2 = StringConverter::parseReal(lua_tostring(pL, 2));
			SequenceLuaCallback::getSingleton().setSpeedRot(tex,tex2);
		}
		return 1;
	}


	static int runScript(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			RunLuaScript(pL,tex.c_str());
		}
		return 1;
	}

	static int COMSendData(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			//tex="e";
			if (global::getSingleton().port)
			{
				LogManager::getSingleton().logMessage("Serial port: sending "+tex);
				global::getSingleton().port->SendData(tex.c_str(),tex.length());
			}
		}
		return 1;
	}
	
	static int inc__register(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			//String tex1 = lua_tostring(pL, 2);
			//tex="e";
			LogManager::getSingleton().logMessage("(Lua): RUN3 REGISTER OPERATION");
			if (tex=="rx1")
				global::getSingleton().rx1++;
			if (tex=="rx2")
				global::getSingleton().rx2++;
			if (tex=="ry1")
				global::getSingleton().ry1++;
			if (tex=="ry2")
				global::getSingleton().ry2++;
			if (tex=="rz1")
				global::getSingleton().rz1++;
			if (tex=="rz2")
				global::getSingleton().rz2++;
			if (tex=="a")
				global::getSingleton().a++;
			if (tex=="ra")
				global::getSingleton().ra++;
			if (tex=="rb")
				global::getSingleton().rb++;
			if (tex=="rc")
				global::getSingleton().rc++;
			
		}
		return 1;
	}

	static int dec__register(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			//String tex1 = lua_tostring(pL, 2);
			//tex="e";
			LogManager::getSingleton().logMessage("(Lua): RUN3 REGISTER OPERATION");
			if (tex=="rx1")
				global::getSingleton().rx1--;
			if (tex=="rx2")
				global::getSingleton().rx2--;
			if (tex=="ry1")
				global::getSingleton().ry1--;
			if (tex=="ry2")
				global::getSingleton().ry2--;
			if (tex=="rz1")
				global::getSingleton().rz1--;
			if (tex=="rz2")
				global::getSingleton().rz2--;
			if (tex=="a")
				global::getSingleton().a--;
			if (tex=="ra")
				global::getSingleton().ra--;
			if (tex=="rb")
				global::getSingleton().rb--;
			if (tex=="rc")
				global::getSingleton().rc--;
			
		}
		return 1;
	}

	static int set__register(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			//tex="e";
			LogManager::getSingleton().logMessage("(Lua): RUN3 REGISTER OPERATION");
			if (tex=="rx1")
				global::getSingleton().rx1=StringConverter::parseReal(tex1);
			if (tex=="rx2")
				global::getSingleton().rx2=StringConverter::parseReal(tex1);
			if (tex=="ry1")
				global::getSingleton().ry1=StringConverter::parseReal(tex1);
			if (tex=="ry2")
				global::getSingleton().ry2=StringConverter::parseReal(tex1);
			if (tex=="rz1")
				global::getSingleton().rz1=StringConverter::parseReal(tex1);
			if (tex=="rz2")
				global::getSingleton().rz2=StringConverter::parseReal(tex1);
			if (tex=="a")
				global::getSingleton().a=StringConverter::parseReal(tex1);
			if (tex=="ra")
				global::getSingleton().ra=StringConverter::parseReal(tex1);
			if (tex=="rb")
				global::getSingleton().rb=StringConverter::parseReal(tex1);
			if (tex=="rc")
				global::getSingleton().rc=StringConverter::parseReal(tex1);
			
		}
		return 1;
	}

	static int add__register(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			//tex="e";
			LogManager::getSingleton().logMessage("(Lua): RUN3 REGISTER OPERATION");
			if (tex=="rx1")
				global::getSingleton().rx1+=StringConverter::parseReal(tex1);
			if (tex=="rx2")
				global::getSingleton().rx2+=StringConverter::parseReal(tex1);
			if (tex=="ry1")
				global::getSingleton().ry1+=StringConverter::parseReal(tex1);
			if (tex=="ry2")
				global::getSingleton().ry2+=StringConverter::parseReal(tex1);
			if (tex=="rz1")
				global::getSingleton().rz1+=StringConverter::parseReal(tex1);
			if (tex=="rz2")
				global::getSingleton().rz2+=StringConverter::parseReal(tex1);
			if (tex=="a")
				global::getSingleton().a+=StringConverter::parseReal(tex1);
			if (tex=="ra")
				global::getSingleton().ra+=StringConverter::parseReal(tex1);
			if (tex=="rb")
				global::getSingleton().rb+=StringConverter::parseReal(tex1);
			if (tex=="rc")
				global::getSingleton().rc+=StringConverter::parseReal(tex1);
			
		}
		return 1;
	}

	static int get__register(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			LogManager::getSingleton().logMessage("(Lua): RUN3 REGISTER OPERATION");
			String tex = lua_tostring(pL, 1);
			if (tex=="rx1")
				lua_pushnumber(pL,global::getSingleton().rx1);
			if (tex=="rx2")
				lua_pushnumber(pL,global::getSingleton().rx2);
			if (tex=="ry1")
				lua_pushnumber(pL,global::getSingleton().ry1);
			if (tex=="ry2")
				lua_pushnumber(pL,global::getSingleton().ry2);
			if (tex=="rz1")
				lua_pushnumber(pL,global::getSingleton().rz1);
			if (tex=="rz2")
				lua_pushnumber(pL,global::getSingleton().rz2);
			if (tex=="a")
				lua_pushnumber(pL,global::getSingleton().a);
			if (tex=="ra")
				lua_pushnumber(pL,global::getSingleton().ra);
			if (tex=="rb")
				lua_pushnumber(pL,global::getSingleton().rb);
			if (tex=="rc")
				lua_pushnumber(pL,global::getSingleton().rc);
		}
		return 1;
	}

	static int random_number(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			lua_pushnumber(pL,Ogre::Math::RangeRandom(0,StringConverter::parseReal(tex)));
		}
		return 1;
	}

	static int killEverybody(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=0)
		{
			return 0;
		}
	
		NPCManager::getSingleton().npcs_kill();

		return 1;
	}
static int addWeapon(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			CWeapon::getSingleton().addWeapon(tex);
		}
		return 1;
	}

	static int flashText(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SequenceLuaCallback::getSingleton().flashT(tex);
		}
		return 1;
	}

	static int gameText(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			HUD::getSingleton().flashText(tex,StringConverter::parseReal(lua_tostring(pL, 2)),StringConverter::parseReal(lua_tostring(pL, 3)));
		}
		return 1;
	}

	static int gameTextColor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			HUD::getSingleton().setTextColor(StringConverter::parseColourValue(tex));
		}
		return 1;
	}

	static int gameTextSize(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			HUD::getSingleton().setTextSize(StringConverter::parseReal(tex));
		}
		return 1;
	}

	static int gameText2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			HUD::getSingleton().flashText2(tex,StringConverter::parseReal(lua_tostring(pL, 2)),StringConverter::parseReal(lua_tostring(pL, 3)));
		}
		return 1;
	}

	static int emitSound(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			Real dur = StringConverter::parseReal(lua_tostring(pL, 2));
			Run3SoundRuntime::getSingleton().emitSound(tex,dur,false);
		}
		return 1;
	}

	static int enableAmbientSound(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Run3SoundRuntime::getSingleton().enableAmbientSound(tex);
		}
		return 1;
	}

	static int disableAmbientSound(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Run3SoundRuntime::getSingleton().disableAmbientSound(tex);
		}
		return 1;
	}

static int emitSound2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=5)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5))
		{
			String tex = lua_tostring(pL, 1);
			Real dur = StringConverter::parseReal(lua_tostring(pL, 2));
			Run3SoundRuntime::getSingleton().emitSound(tex,dur,false,Vector3(StringConverter::parseReal(lua_tostring(pL, 3)),StringConverter::parseReal(lua_tostring(pL, 4)),StringConverter::parseReal(lua_tostring(pL, 5))),50,1,100);
			//Run3SoundRuntime::getSingleton().emitSound(tex,dur,false);
		}
		return 1;
	}

	// GAME ENGINE LUA FEATURES!!!!
	static int toggleLight(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(!mgr->getLight(tex)->getVisible());
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int enableLight(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(true);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int getPixelCount1(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			CustomSceneManager* mgr = (CustomSceneManager*)global::getSingleton().getSceneManager();

			if (mgr->hasEntity(tex))
			{
				/*Camera* _camera = global::getSingleton().getCamera();

				Sequence::getSingleton().m_RenderableOcclusionPair =
				new RenderableOcclusionPair(mgr->getEntity(tex)->getSubEntity(0));
				assert (m_RenderableOcclusionPair);
		 
				///
				/// ADD TO EXTERN LIST
				///
				
				Sequence::getSingleton().pushCounter();
				//Ogre::Vector3 hcsPosition = _camera->getProjectionMatrix() * _camera->getViewMatrix() * _position;
				//return Ogre::Vector2(0.5 - hcsPosition.x * 0.5f, hcsPosition.y * 0.5f + 0.5);
				LogManager::getSingleton().logMessage(StringConverter::toString(Sequence::getSingleton().m_RenderableOcclusionPair->GetPixelCount()));
				*/

				mgr->inspectRenderable(mgr->getEntity(tex)->getSubEntity(0));
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}
	
	void pushCounter()
	{
		g_RenderableOcclusionPairList.push_back(m_RenderableOcclusionPair);
	}

	static int getPixelCount2(lua_State* pL)
	{
		CustomSceneManager* mgr = (CustomSceneManager*)global::getSingleton().getSceneManager();
		LogManager::getSingleton().logMessage(StringConverter::toString(mgr->getPixelCount()));
		return 1;
	}

	static int disableLight(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(false);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}


	static int npcEvent(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			String tex2 = lua_tostring(pL, 3);
			//SceneManager* mgr = global::getSingleton().getSceneManager();
			/*if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(!mgr->getLight(tex)->getVisible());
			}
			else
			{
				return 0;
			}*/
			NPCManager::getSingleton().npc_event(tex,tex1,tex2);
		}
		return 1;
	}

	static int npcEvent2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=4)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			String tex2 = lua_tostring(pL, 3);
			String tex3 = lua_tostring(pL, 4);
			//SceneManager* mgr = global::getSingleton().getSceneManager();
			/*if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(!mgr->getLight(tex)->getVisible());
			}
			else
			{
				return 0;
			}*/
			NPCManager::getSingleton().npc_event(tex,tex1,tex2,tex3);
		}
		return 1;
	}

	static int __all_npcEvent(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			String tex2 = lua_tostring(pL, 3);
			//SceneManager* mgr = global::getSingleton().getSceneManager();
			/*if (mgr->hasLight(tex))
			{
			mgr->getLight(tex)->setVisible(!mgr->getLight(tex)->getVisible());
			}
			else
			{
				return 0;
			}*/
			NPCManager::getSingleton().all_npc_event(tex,tex1,tex2);
		}
		return 1;
	}

	static int commandLeaderGroup(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex1 = lua_tostring(pL, 2);
			String tex2 = lua_tostring(pL, 3);
			AIManager::getSingleton().commandGroup(tex,tex1,tex2); //Do all AI calculations - in real world there is no second step
			AIManager::getSingleton().processGroupCommands(tex); //Output them (as we are in the 3d reality)
		}
		return 1;
	}

	static int changeLevel(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			global::getSingleton().changemap_now = true;
								global::getSingleton().mMap = tex;
		}
		return 1;
	}

	static int toggleEntity(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasEntity(tex))
			{
			mgr->getEntity(tex)->setVisible(!mgr->getEntity(tex)->getVisible());
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int toggleParticleSystem(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasParticleSystem(tex))
			{
			mgr->getParticleSystem(tex)->setVisible(!mgr->getParticleSystem(tex)->getVisible());
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int toggleEmitter(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasParticleSystem(tex))
			{
			mgr->getParticleSystem(tex)->getEmitter(0)->setEnabled(!mgr->getParticleSystem(tex)->getEmitter(0)->getEnabled());
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int enableEmitter(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasParticleSystem(tex))
			{
			mgr->getParticleSystem(tex)->getEmitter(0)->setEnabled(true);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int disableEmitter(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasParticleSystem(tex))
			{
			mgr->getParticleSystem(tex)->getEmitter(0)->setEnabled(false);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int enableTrigger(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Trigger* cscene;
		for (unsigned int i=0; i!=Sequence::getSingleton().triggers.size();i++)
	{
		if (Sequence::getSingleton().triggers[i]->getname() == tex)
		{
			cscene = Sequence::getSingleton().triggers[i];
		}
	}
		cscene->setEnabled(true);
		}
		return 1;
	}

static int disableTrigger(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Trigger* cscene;
			for (unsigned int i=0; i!=Sequence::getSingleton().triggers.size();i++)
	{
		if (Sequence::getSingleton().triggers[i]->getname() == tex)
		{
			cscene = Sequence::getSingleton().triggers[i];
		}
	}
		cscene->setEnabled(false);
		}
		return 1;
	}


	static int fireFire(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Fire* firee;
			for (unsigned int i=0; i!=Sequence::getSingleton().fires.size();i++)
			{
				if (Sequence::getSingleton().fires[i]->getName() == tex)
				{
					firee = Sequence::getSingleton().fires[i];
				}
			}
			firee->fire();
		}
		return 1;
	}

	static int fireToggle(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Fire* firee;
			for (unsigned int i=0; i!=Sequence::getSingleton().fires.size();i++)
			{
				if (Sequence::getSingleton().fires[i]->getName() == tex)
				{
					firee = Sequence::getSingleton().fires[i];
				}
			}
			firee->toggle();
		}
		return 1;
	}

	static int fireExtinguish(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Fire* firee;
			for (unsigned int i=0; i!=Sequence::getSingleton().fires.size();i++)
			{
				if (Sequence::getSingleton().fires[i]->getName() == tex)
				{
					firee = Sequence::getSingleton().fires[i];
				}
			}
			firee->extinguish();
		}
		return 1;
	}

static int enableInventory(lua_State* pL)
	{
		Inventory::getSingleton().enableInventory();
		return 1;
	}

static int disableInventory(lua_State* pL)
	{
		Inventory::getSingleton().disableInventory();
		return 1;
	}

	static int showEntity(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasEntity(tex))
			{
			mgr->getEntity(tex)->setVisible(true);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int materialEntity(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		LogManager::getSingleton().logMessage("MatEnt");
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasEntity(tex))
			{
				LogManager::getSingleton().logMessage("Setting material to entity:"+tex);
				LogManager::getSingleton().logMessage(tex2);
				mgr->getEntity(tex)->getSubEntity(0)->setMaterial(MaterialManager::getSingleton().getByName(tex2));
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int hideEntity(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasEntity(tex))
			{
			mgr->getEntity(tex)->setVisible(false);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int spawnPhysObject(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			SceneManager* mgr = global::getSingleton().getSceneManager();
			if (mgr->hasEntity(tex))
			{
			mgr->getEntity(tex)->setVisible(!mgr->getEntity(tex)->getVisible());
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}

	static int setCompositorEnab(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 2);
			bool enabled = StringConverter::parseBool(lua_tostring(pL, 1));
			if (enabled)
				CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),tex);
			
			CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),tex,enabled);
			if (!enabled)
				CompositorManager::getSingleton().removeCompositor(global::getSingleton().getCamera()->getViewport(),tex);
		}
		return 1;
	}

	static int destroyNPC(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			NPCManager::getSingleton().npc_flush(tex);
		}
		return 1;
	}


	static int fragmentGPUProgramParams(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			LogManager::getSingleton().logMessage("Setting fragment program param...");
			String tex = lua_tostring(pL, 1);
			/*Ogre::HighLevelGpuProgramPtr mFragmentProgram = GpuProgramManager::getSingleton().getByName(tex);
			LogManager::getSingleton().logMessage(mFragmentProgram->getName());
			GpuProgramParametersSharedPtr gprpar = mFragmentProgram->getDefaultParameters();
			//LogManager::getSingleton().logMessage(gprpar->getNamedConstant
			gprpar->setNamedConstant("sater", 0.1f);*/
			MaterialPtr mActiveMaterial = Ogre::MaterialManager::getSingleton().getByName(tex);
			GpuProgramPtr mActiveFragmentProgram = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgram();
			GpuProgramParametersSharedPtr mActiveFragmentParameters = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgramParameters();
			mActiveFragmentParameters->setNamedConstant(lua_tostring(pL, 2),StringConverter::parseReal(lua_tostring(pL, 3)));
			//LogManager::getSingleton().logMessage(StringConverter::toString(mFragmentProgram->setParameter(lua_tostring(pL, 2),lua_tostring(pL, 3))));
			//mFragmentProgram->setParameter(lua_tostring(pL, 2),lua_tostring(pL, 3));
		}
		return 1;
	}

	static int fragmentGPUProgramTime(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			LogManager::getSingleton().logMessage("Getting fragment program param...");
			String tex = lua_tostring(pL, 1);
			/*Ogre::HighLevelGpuProgramPtr mFragmentProgram = GpuProgramManager::getSingleton().getByName(tex);
			LogManager::getSingleton().logMessage(mFragmentProgram->getName());
			GpuProgramParametersSharedPtr gprpar = mFragmentProgram->getDefaultParameters();
			//LogManager::getSingleton().logMessage(gprpar->getNamedConstant
			gprpar->setNamedConstant("sater", 0.1f);*/
			MaterialPtr mActiveMaterial = Ogre::MaterialManager::getSingleton().getByName(tex);
			GpuProgramPtr mActiveFragmentProgram = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgram();
			GpuProgramParametersSharedPtr mActiveFragmentParameters = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgramParameters();
			//mActiveFragmentParameters->setAutoConstantReal(0,Ogre::GpuProgramParameters::AutoConstantType::ACT_TIME,0.0f);
			LogManager::getSingleton().logMessage("");
			//mActiveFragmentParameters->setNamedAutoConstantReal("time",Ogre::GpuProgramParameters::AutoConstantType::ACT_TIME_0_X,7.0f);
			Ogre::ControllerManager::getSingleton().setElapsedTime(0);
			//LogManager::getSingleton().logMessage(StringConverter::toString(mFragmentProgram->setParameter(lua_tostring(pL, 2),lua_tostring(pL, 3))));
			//mFragmentProgram->setParameter(lua_tostring(pL, 2),lua_tostring(pL, 3));
		}
		return 1;
	}




	static int fadeoutMusic(lua_State* pL)
	{
		MusicPlayer::getSingleton().playMusic("",false);
		return 1;
	}

	static int playMusic(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
		 bool loop = StringConverter::parseBool(lua_tostring(pL, 2));
			MusicPlayer::getSingleton().playMusic(tex,loop);
		}
		return 1;
	}

	static int setMusicVolume(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			MusicPlayer::getSingleton().setVolume(StringConverter::parseReal(tex));
		}
		return 1;
	}

	static int addAmbientSound(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=4)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4))
		{
			String tex = lua_tostring(pL, 1);
			Vector3 pos = StringConverter::parseVector3(lua_tostring(pL, 2));
			Real dist = StringConverter::parseReal(lua_tostring(pL, 3));
			Real fall = StringConverter::parseReal(lua_tostring(pL, 4));
			Run3SoundRuntime::getSingleton().addAmbientSound(tex,pos,dist,fall);
		}
		return 1;
	}


	static int bod__getpos(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					if (tex2=="x")
					lua_pushnumber(pL,pobj[i]->get_pos().x);
					if (tex2=="y")
					lua_pushnumber(pL,pobj[i]->get_pos().y);
					if (tex2=="z")
					lua_pushnumber(pL,pobj[i]->get_pos().z);
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Get Object not found");
		return 1;
	}
	
	static int bod__getpos2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))//&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			//String tex2 = lua_tostring(pL, 2);
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					/*if (tex2=="x")
					lua_pushnumber(pL,pobj[i]->get_pos().x);
					if (tex2=="y")
					lua_pushnumber(pL,pobj[i]->get_pos().y);
					if (tex2=="z")
					lua_pushnumber(pL,pobj[i]->get_pos().z);*/
					lua_pushstring(pL,StringConverter::toString(pobj[i]->get_pos()).c_str());
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Get Object not found");
		return 1;
	}

	static int nod__getpos(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			if (global::getSingleton().getSceneManager()->hasSceneNode(tex))
			{
					if (tex2=="x")
					lua_pushnumber(pL,(int)global::getSingleton().getSceneManager()->getSceneNode(tex)->getPosition().x);
					if (tex2=="y")
					lua_pushnumber(pL,(int)global::getSingleton().getSceneManager()->getSceneNode(tex)->getPosition().y);
					if (tex2=="z")
					lua_pushnumber(pL,(int)global::getSingleton().getSceneManager()->getSceneNode(tex)->getPosition().z);
					return 1;

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Get Object not found");
		return 1;
	}

	static int nod__getpos2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))//&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			//String tex2 = lua_tostring(pL, 2);
			
				if (global::getSingleton().getSceneManager()->hasSceneNode(tex))
				{
					/*if (tex2=="x")
					lua_pushnumber(pL,pobj[i]->get_pos().x);
					if (tex2=="y")
					lua_pushnumber(pL,pobj[i]->get_pos().y);
					if (tex2=="z")
					lua_pushnumber(pL,pobj[i]->get_pos().z);*/
					lua_pushstring(pL,StringConverter::toString(global::getSingleton().getSceneManager()->getSceneNode(tex)->getPosition()).c_str());
					return 1;
				}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Get Object not found");
		return 1;
	}

	static int logMessage(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			LogManager::getSingleton().logMessage(tex);
		}
		return 1;
	}

	static int bod__setpos(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			Real tex3 = StringConverter::parseReal(lua_tostring(pL, 3)); 
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			LogManager::getSingleton().logMessage("(Lua): Set Object Pos "+StringConverter::toString(tex3));
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					
					Vector3 pso = pobj[i]->get_pos();
					Quaternion ori = pobj[i]->get_orient();
					if (tex2=="x")
					pobj[i]->bod->setPositionOrientation(Vector3(tex3,pso.y,pso.z),ori);
					if (tex2=="y")
					pobj[i]->bod->setPositionOrientation(Vector3(pso.x,tex3,pso.z),ori);
					if (tex2=="z")
					pobj[i]->bod->setPositionOrientation(Vector3(pso.x,pso.y,tex3),ori);
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Set Object not found");
		return 1;
	}

	static int bod__setpos2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))//&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2); 
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
//			LogManager::getSingleton().logMessage("(Lua): Set Object Pos "+StringConverter::toString(tex3));
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					
					//Vector3 pso = pobj[i]->get_pos();
					Quaternion ori = pobj[i]->get_orient();
					pobj[i]->bod->setPositionOrientation(StringConverter::parseVector3(tex2),ori);
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Set Object not found");
		return 1;
	}

	
	static int nod__setpos2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))//&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2); 
			

				if (global::getSingleton().getSceneManager()->hasSceneNode(tex))
				{
					global::getSingleton().getSceneManager()->getSceneNode(tex)->setPosition(StringConverter::parseVector3(tex2));
					return 1;
				}

		}
		LogManager::getSingleton().logMessage("(Lua): Set Object not found");
		return 1;
	}

	static int bod__getor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					if (tex2=="pitch")
					lua_pushnumber(pL,pobj[i]->get_orient().getPitch().valueDegrees());
					if (tex2=="yaw")
					lua_pushnumber(pL,pobj[i]->get_orient().getYaw().valueDegrees());
					if (tex2=="roll")
					lua_pushnumber(pL,pobj[i]->get_orient().getRoll().valueDegrees());
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Get Object not found");
		return 1;
	}

	static int bod__setor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			Real tex3 = StringConverter::parseReal(lua_tostring(pL, 3)); 
			vector<PhysObject*> pobj = global::getSingleton().pobjects;
			LogManager::getSingleton().logMessage("(Lua): Set Object Pos "+StringConverter::toString(tex3));
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==tex)
				{
					
				/*	Vector3 pso = pobj[i]->get_pos();
					Quaternion ori1 = pobj[i]->get_orient();
					Quaternion ori2 = Quaternion(ori1.getPitch(),Vector3::UNIT_X);
					Quaternion ori3 = Quaternion(ori1.getYaw(),Vector3::UNIT_Y);
					Quaternion ori4 = Quaternion(ori1.getRoll(),Vector3::UNIT_Z);
					if (tex2=="pitch")
					pobj[i]->bod->setPositionOrientation(pso,ori3*ori4*Quaternion(Degree(tex3),Vector3::UNIT_X));
					if (tex2=="yaw")
					pobj[i]->bod->setPositionOrientation(pso,ori2*ori4*Quaternion(Degree(tex3),Vector3::UNIT_Y));
					if (tex2=="roll")
					pobj[i]->bod->setPositionOrientation(pso,ori2*ori3*Quaternion(Degree(tex3),Vector3::UNIT_Z));
					return 1;*/
					Vector3 pso = pobj[i]->get_pos();
					Quaternion ori1 = pobj[i]->nod->getOrientation();
					Quaternion ori2 = Quaternion(ori1.getPitch(),Vector3::UNIT_X);
					Quaternion ori3 = Quaternion(ori1.getYaw(),Vector3::UNIT_Y);
					Quaternion ori4 = Quaternion(ori1.getRoll(),Vector3::UNIT_Z);
					//pobj[i].nod->pitch(ori1.getPitch().valueDegrees()-tex3);
					LogManager::getSingleton().logMessage("(Lua): DEBUG"+StringConverter::toString(ori1.getPitch().valueDegrees()));
					LogManager::getSingleton().logMessage(StringConverter::toString(ori1.getYaw().valueDegrees()));
					LogManager::getSingleton().logMessage(StringConverter::toString(ori1.getRoll().valueDegrees()));
					if (tex2=="pitch")
					pobj[i]->nod->pitch(Degree(tex3-ori1.getPitch().valueDegrees()));
					if (tex2=="yaw")
					pobj[i]->nod->yaw(Degree(tex3-ori1.getYaw().valueDegrees()));
					if (tex2=="roll")
					pobj[i]->nod->roll(Degree(tex3-ori1.getRoll().valueDegrees()));
					ori1 = pobj[i]->nod->getOrientation();
					LogManager::getSingleton().logMessage("(Lua): DEBUG"+StringConverter::toString(ori1.getPitch().valueDegrees()));
					LogManager::getSingleton().logMessage(StringConverter::toString(ori1.getYaw().valueDegrees()));
					LogManager::getSingleton().logMessage(StringConverter::toString(ori1.getRoll().valueDegrees()));
					pobj[i]->bod->setPositionOrientation(pso,pobj[i]->nod->getOrientation());
					return 1;
				}

			}
			

		}
		LogManager::getSingleton().logMessage("(Lua): Set Object not found");
		return 1;
	}

	static int player__getposx(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().x);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}
static int player__getposy(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}

	static int cursor__getposx(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().x);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}
	static int cursor__getposy(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}

	static int player__getposz(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}
static int tostringl(lua_State* pL)
	{

		lua_pushstring(pL,StringConverter::toString(int(lua_tonumber(pL,1))).c_str());
		return 1;
	}

	static int tostringl2(lua_State* pL)
	{

		lua_pushstring(pL,StringConverter::toString(Vector3(int(lua_tostring(pL,1)),int(lua_tostring(pL,2)),int(lua_tostring(pL,3)))).c_str());
		return 1;
	}

		static int player__getdirx(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_direction().x);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}



static int player__getdiry(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_direction().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}
	static int player__getdirz(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->get_direction().z);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().y);
		//lua_pushnumber(pL,global::getSingleton().getPlayer()->get_location().z);
		return 1;
	}

		static int player__getdistf(lua_State* pL)
	{

		lua_pushnumber(pL,global::getSingleton().getPlayer()->getDistFObject());
		return 1;
	}

	static int player__allowFlash(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			bool tex = StringConverter::parseBool(lua_tostring(pL, 1));
			global::getSingleton().getPlayer()->allowFlashLight(tex);
		}
			
		return 1;
	}

	static int player__destroyFlash(lua_State* pL)
	{
		
		global::getSingleton().getPlayer()->destroyFlashLight();
		return 1;
	}

	static int player__toggleFlash(lua_State* pL)
	{
		
		global::getSingleton().getPlayer()->toggleFlashLight();
		return 1;
	}

	static int turnoff__Sequence(lua_State* pL)
	{
		LogManager::getSingleton().logMessage("Sequence turnoff!");
		Sequence::getSingleton().rest = !Sequence::getSingleton().rest;
		return 1;
	}

	static int turnoff__Module(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			int mod = StringConverter::parseInt(lua_tostring(pL, 1));
			LogManager::getSingleton().logMessage("Module turnoff!");
			global::getSingleton().restData[mod]=!global::getSingleton().restData[mod];
		}
		return 1;
	}

	static int turnoff__NPCManager(lua_State* pL)
	{
		LogManager::getSingleton().logMessage("NPCManager turnoff!");
		NPCManager::getSingleton().rest = !NPCManager::getSingleton().rest;
		return 1;
	}

	static int setNPCManagerStep(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex1 = StringConverter::parseReal(lua_tostring(pL, 1));
			NPCManager::getSingleton().setStep(tex1);
		}
			
		return 1;
	}

	static int player__setRegeneration(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			Real tex1 = StringConverter::parseReal(lua_tostring(pL, 1));
			Real tex2 = StringConverter::parseReal(lua_tostring(pL, 2));
			global::getSingleton().getPlayer()->setHealthRegeneration(tex1,tex2);
		}
			
		return 1;
	}
//Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation();
	static int stopMusic(lua_State* pL)
	{
		
			MusicPlayer::getSingleton().cleanup();
		return 1;
	}

	static int activateMPReg(lua_State* pL)
	{
		
			MusicPlayer::getSingleton().activatePReg();
		return 1;
	}

	static int deactivateMPReg(lua_State* pL)
	{
		
			MusicPlayer::getSingleton().deactivatePReg();
		return 1;
	}

	static int setMPRegVolume(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Real tex = StringConverter::parseReal(lua_tostring(pL, 1));
			MusicPlayer::getSingleton().modulatePRegToValue(tex);
		}
		return 1;
	}

	static int reverseTrain(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_trainReverse(tex);
		}
			
		return 1;
	}

	static int startTrain(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_trainStart(tex);
		}
			
		return 1;
	}

	static int stopTrain(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_trainStop(tex);
		}
			
		return 1;
	}

	static int startEvent(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			vector<Event*>::iterator i;
			for (i=Sequence::getSingleton().s_events.begin();i!=Sequence::getSingleton().s_events.end();i++)
			{
				if ((*i)->getName()==tex)
				{
					(*i)->trigger();
				}
			}
		}
			
		return 1;
	}

	static int powerComputer(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_powerComputer(tex);
		}
			
		return 1;
	}

static int setSpeedTrain(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1),lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			Sequence::getSingleton().s_setSpeedTrain(tex,StringConverter::parseReal(tex2));
		}
			
		return 1;
	}

	static int enableTrainSaveMode(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_enableSaveMode(tex);
		}
			
		return 1;
	}

	static int disableTrainSaveMode(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			Sequence::getSingleton().s_disableSaveMode(tex);
		}
			
		return 1;
	}

	static int setAccTrain(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1),lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
			String tex2 = lua_tostring(pL, 2);
			Sequence::getSingleton().s_setAccTrain(tex,StringConverter::parseReal(tex2));
		}
			
		return 1;
	}

	static int toggleMusic(lua_State* pL)
	{
		
int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			String tex = lua_tostring(pL, 1);
		 bool loop = StringConverter::parseBool(lua_tostring(pL, 2));
			MusicPlayer::getSingleton().toggleMusic(tex,loop);
		}
		return 1;
	}

	static int dssao(lua_State* pL)
	{
		Loader::getSingleton().disableSSAO();
		return 1;
	}

	static int setCameraParent(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			global::getSingleton().getPlayer()->setParentRelation(tex);
		}
		
		return 1;
	}

	static int setFullCameraParent(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			global::getSingleton().getPlayer()->setFullParentRelation(StringConverter::parseBool(tex));
		}
		
		return 1;
	}

	static int resetCameraParent(lua_State* pL)
	{
		global::getSingleton().getPlayer()->stopParentRelation();
		return 1;
	}

	static int teleport(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Vector3 tex = StringConverter::parseVector3(lua_tostring(pL, 1));
			global::getSingleton().getPlayer()->teleport(tex);
		}
		
		return 1;
	}

	static int teleport_rel(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Vector3 tex = StringConverter::parseVector3(lua_tostring(pL, 1));
			global::getSingleton().getPlayer()->teleport(global::getSingleton().getPlayer()->get_location()+tex);
		}
		
		return 1;
	}


	static int Fade(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
		Real duration = StringConverter::parseReal(lua_tostring(pL, 1));
		Real speed_f =  StringConverter::parseReal(lua_tostring(pL, 2));
		
		FadeListener::getSingleton().setDuration(duration);
		FadeListener::getSingleton().setDurationF(speed_f);
		FadeListener::getSingleton().startIN();
		}
		return 1;
	}

	static int FadeOut(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
		Real duration = StringConverter::parseReal(lua_tostring(pL, 1));
		Real speed_f =  StringConverter::parseReal(lua_tostring(pL, 2));
		
		FadeListener::getSingleton().setDuration(duration);
		FadeListener::getSingleton().setDurationF(speed_f);
		FadeListener::getSingleton().startIN();
		}
		return 1;
	}

	static int playerShake(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=3)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3))
		{
		Real a0 = StringConverter::parseReal(lua_tostring(pL, 1));
		Real b =  StringConverter::parseReal(lua_tostring(pL, 2));
		Real w =  StringConverter::parseReal(lua_tostring(pL, 3));

		global::getSingleton().getPlayer()->playerShake(a0,b,w);
		}
		return 1;
	}
	static int setTimeK(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
		Real a0 = StringConverter::parseReal(lua_tostring(pL, 1));

		Timeshift::getSingleton().setTimeK(a0);
		}
		return 0;
	}
	static int removeCutScene2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			Sequence::getSingleton().removeCutScene(lua_tostring(pL, 1));
		}
		return 0;
	}
	static int timeShift(lua_State* pL)
	{
		Timeshift::getSingleton().toggleTStop();
		return 0;
	}
   //utilites
   	String getAttrib(TiXmlElement *XMLNode, const String &parameter, const String &defaultValue = "");
	Real getAttribReal(TiXmlElement *XMLNode, const String &parameter, Real defaultValue = 0);
	bool getAttribBool(TiXmlElement *XMLNode, const String &parameter, bool defaultValue = false);

	Vector3 parseVector3(TiXmlElement *XMLNode);
	Quaternion parseQuaternion(TiXmlElement *XMLNode);
	ColourValue parseColour(TiXmlElement *XMLNode);

	bool hasCutScene(String name)
	{
		CutScene* cscene=0;
		for (unsigned int i=0; i!=cutscenes.size();i++)
	{
		if (cutscenes[i]->getname() == name)
		{
			cscene = cutscenes[i];
		}
	}
	if (cscene)
		return true;
	return false;
	}

	bool hasTimer(String name)
	{
		Run3Timer* cscene=0;
		for (unsigned int i=0; i!=timers.size();i++)
	{
		if (timers[i]->mName == name)
		{
			cscene = timers[i];
		}
	}
	if (cscene)
		return true;
	return false;
	}

	bool removeCutScene(String name)
	{
		LogManager::getSingleton().logMessage("1");
		CutScene* cscene=0;
		LogManager::getSingleton().logMessage("1");
		vector<CutScene*>::iterator i;
		LogManager::getSingleton().logMessage("1");
		for (i=cutscenes.begin(); i!=cutscenes.end();i++)
		{
			if ((*i)->getname() == name)
			{
				cscene = (*i);
				break;
			}
		}
		LogManager::getSingleton().logMessage("2");
		if (cscene)
		{
			LogManager::getSingleton().logMessage("1");
			cscene->dispose();
			LogManager::getSingleton().logMessage("2");
			cutscenes.erase(i);
			LogManager::getSingleton().logMessage("3");
			return true;
		}
		return false;
	}

   virtual bool frameStarted(const Ogre::FrameEvent &evt);
   virtual bool frameEnded(const Ogre::FrameEvent &evt);

   vector<Trigger*> triggers;
   vector<PhysObject*> pobjs;
   vector<CutScene*> cutscenes;
   vector<SeqScript*> seqscripts;
   vector<func_door*> doors;
   vector<Event*> s_events;
   vector<String> events;
   vector<Computer*> computers;
   vector<Run3::Button*> buttons;
   vector<Pickup*> pickups;
   vector<LensFlare*> flares;
   vector<Train*> trains;
   vector<Run3Timer*> timers;
   vector<DarkZone*> darkzones;
   vector<Pendulum*> pends;
   vector<Ladder*> ladders;
   vector<Fire*> fires;
   vector<FuzzyObject*> fobj;
   RenderableOcclusionPair* m_RenderableOcclusionPair;
private:
	
	ElementGroupGenerator* grpgen;
	bool rest;
	lua_State* pLuaState;
	Ogre::Root* mRoot;
	Real sec;
	String sequence;
	SceneManager* mSceneMgr;
	SoundManager* mSoundManager;
	TiXmlDocument   *SequenceDoc;
	TiXmlElement   *SequenceRoot;
	TrainMat* mPhysCallback;
	const OgreNewt::MaterialID* mMatDefault;
	const OgreNewt::MaterialID* trainMat;
	OgreNewt::MaterialPair* mMatPair;
};