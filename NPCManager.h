///////////////////////////////////////////////////////////////////////
//		NPCManager class for Run3									 //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		use for controlling NPCs									 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "AIR3.h"
#include "global.h"
#include "NPCSpawnProps.h"
#include "AIManager.h"

// BASE NPCs
//basic examle NPC
#include "npc_enemy.h"
#include "npc_friend.h"
#include "npc_neutral.h"
#include "npc_aerial.h"
#include "enemyMatCallback.h"
#include "NPCFactory.h"
#include "neutralMatCallback.h"

using namespace Ogre;
using namespace std;

class NPCManager:public Singleton<NPCManager>
{
public:
	NPCManager();
	~NPCManager();
	void init();
	void npc_spawn(NPCSpawnProps npc);
	void npcs_kill()
	{
		std::vector<npc_enemy*>::iterator i;
			for (i=enemies.begin();i!=enemies.end();i++)
			{
				npc_enemy* rd = (*i);
				rd->processEvent(KILL_NPC,"","");
			}
		std::vector<npc_friend*>::iterator j;
			for (j=friends.begin();j!=friends.end();j++)
			{
				npc_friend* rd = (*j);
				rd->processEvent(KILL_NPC,"","");
			}
		std::vector<npc_neutral*>::iterator k;
			for (k=neutrals.begin();k!=neutrals.end();k++)
			{
				npc_neutral* rd = (*k);
				rd->processEvent(KILL_NPC,"","");
			}
		std::vector<npc_aerial*>::iterator l;
			for (l=aerials.begin();l!=aerials.end();l++)
			{
				npc_aerial* rd = (*l);
				rd->processEvent(KILL_NPC,"","");
			}
	}
	void npc_kill(String name);
	void npc_event(String name, String param1,String param2);
	void all_npc_event(String name, String param1,String param2);
	void npc_event(String name,String type, String param1,String param2);
	void npc_flush(String name)
	{
		LogManager::getSingleton().logMessage("DestroyNPC:"+name);
			std::vector<npc_enemy*>::iterator i;
			for (i=enemies.begin();i!=enemies.end();i++)
			{
				npc_enemy* rd = (*i);
				if (rd->getName()==name)
				{
					enemies.erase(i);
					delete rd;
				}
			}

			std::vector<npc_friend*>::iterator j;
			for (j=friends.begin();j!=friends.end();j++)
			{
				npc_friend* rd = (*j);
				if (rd->getName()==name)
				{
					friends.erase(j);
					delete rd;
				}
			}

			std::vector<npc_neutral*>::iterator k;
			for (k=neutrals.begin();k!=neutrals.end();k++)
			{
				npc_neutral* rd = (*k);
				if (rd->getName()==name)
				{
					neutrals.erase(k);
					delete rd;
				}
			}
			
			std::vector<npc_aerial*>::iterator l;
			for (l=aerials.begin();l!=aerials.end();l++)
			{
				npc_aerial* rd = (*l);
				if (rd->getName()==name)
				{
					aerials.erase(l);
					delete rd;
				}
			}
			LogManager::getSingleton().logMessage("EndDestroyNPC:"+name);
	}
	void npcs_flush()
	{
			std::vector<npc_enemy*>::iterator i;
			for (i=enemies.begin();i!=enemies.end();i++)
			{
				npc_enemy* rd = (*i);
				delete rd;
			}
			enemies.clear();
			std::vector<npc_friend*>::iterator j;
			for (j=friends.begin();j!=friends.end();j++)
			{
				npc_friend* rd = (*j);
				delete rd;
			}
			friends.clear();
			std::vector<npc_neutral*>::iterator k;
			for (k=neutrals.begin();k!=neutrals.end();k++)
			{
				npc_neutral* rd = (*k);
				delete rd;
			}
			neutrals.clear();
			std::vector<npc_aerial*>::iterator l;
			for (l=aerials.begin();l!=aerials.end();l++)
			{
				npc_aerial* rd = (*l);
				delete rd;
			}
			aerials.clear();
	}
	bool frameStarted(const Ogre::FrameEvent &evt);
	inline Real getStep(){return mStep;}
	inline void setStep(Real step){mStep = step;}
	/////
	vector<npc_enemy*> enemies;
	vector<npc_neutral*> neutrals;
	vector<npc_friend*> friends;
	vector<npc_aerial*> aerials;
	bool rest;
private:
	Real dt;
	Real mStep;
	const OgreNewt::MaterialID* mMatDefault;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* enemyMat;
	enemyMatCallback* mPhysCallback;
	const OgreNewt::MaterialID* neutralMat;
	neutralMatCallback* mPhysCallback2;
	int AIType;
	PRIVATE_NODELIST
};