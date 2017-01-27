///////////////////////////////////////////////////////////////////////
//		AIManager class for Run3									 //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		use for preparing AI										 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "AIR3.h"
#include "global.h"

using namespace Ogre;
using namespace std;

class AIManager:public Singleton<AIManager>
{
public:
	AIManager();
	~AIManager();
	void init(int AIType,Vector3 nodeGen,bool autoGenNodes);
	int getAIType(){return pAiVersion;}
	nodeGenerator* getGeneratorInstance(void){return mNodeGen;}
	AirPathFind* getPathFinder(void){return mPathFinder;}
	void addNPCNode(NPCNode* n){nList->addNPCNode(n);mPathFinder->setNodes(NPCNODES);}
	void addElementGroup(AIElementGroup* grp){grps.push_back(grp);}

	//FOR USE WITHIN NPC CLASS
	AIElementGroup* getNPCGroupByNPCName(String name);
	AIElement* getAIElementByNPCName(String name);

	void commandGroup(String name,String command1,String command2)
	{
		for (unsigned int i=0;i!=grps.size();i++)
		{
			if (grps[i]->getName()==name)
			{
				grps[i]->command(command1,command2);
				LogManager::getSingleton().logMessage("Commanding to group:"+name+"with commands 1"+command1+"commands 2"+command2);
			}
		}
	}
	void commandGroup(String name,String command1)
	{
		for (unsigned int i=0;i!=grps.size();i++)
		{
			if (grps[i]->getName()==name)
			{
				//grps[i]->command("",command);
			}
		}
	}
	void processGroupCommands(String name);
	void cleanupAllNodes(){nList->cleanupAllNodes();}
	void cleanupElementGroups()
	{
		grps.clear();
	}
	void update(Real evt);
private:
	NodeList* nList;
	vector<AIElementGroup*> grps;
	AirPathFind* mPathFinder;
	int pAiVersion;
	nodeGenerator* mNodeGen;
};