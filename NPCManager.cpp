#include "NPCManager.h"

template<> NPCManager *Singleton<NPCManager>::ms_Singleton=0;

NPCManager::NPCManager()
{

}

NPCManager::~NPCManager()
{

}

void NPCManager::init()
{
RUN3_AI_GET
AIType = AIManager::getSingleton().getAIType();
mPhysCallback = new enemyMatCallback();
mPhysCallback2 = new neutralMatCallback();
OgreNewt::World* mWorld = global::getSingleton().getWorld();
mMatDefault = global::getSingleton().getPlayer()->getPlayerMat();
  enemyMat= new OgreNewt::MaterialID( mWorld );
  mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, enemyMat );
  mMatPair->setContactCallback( mPhysCallback );
  mMatPair = new OgreNewt::MaterialPair( mWorld, mWorld->getDefaultMaterialID(), enemyMat );
  mMatPair->setContactCallback( mPhysCallback );
  mMatPair->setDefaultFriction(0,0);
  mMatPair->setDefaultSoftness(0.0f);
  mMatPair->setDefaultElasticity(0.0f);

  neutralMat= new OgreNewt::MaterialID( mWorld );
  mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, neutralMat );
  mMatPair->setContactCallback( mPhysCallback2 );
  mMatPair = new OgreNewt::MaterialPair( mWorld, mWorld->getDefaultMaterialID(), neutralMat );
  mMatPair->setContactCallback( mPhysCallback2 );
  mMatPair->setDefaultFriction(0,0);
  mMatPair->setDefaultSoftness(0.0f);
  mMatPair->setDefaultElasticity(0.0f);
  mMatPair = new OgreNewt::MaterialPair( mWorld, neutralMat, enemyMat );
  mMatPair->setContactCallback( mPhysCallback );
	
}

void NPCManager::npc_spawn(NPCSpawnProps npc)
{
	NPCSpawnProps details = npc;
	/*Vector3 pos = details.spCPos;
	Quaternion rot = details.rot;
	String name = details.mName;
	Real health = details.health;
	int relation = details.relation;*/
	String className = details.spCName;

	if (className == "npc_enemy")
	{
		LogManager::getSingleton().logMessage("Npc Manager: npc enemy requested for a spawn:");
		npc_enemy* enemy = new npc_enemy();
		enemy->init(npc);
		LogManager::getSingleton().logMessage("Setting node list..."+StringConverter::toString(mNodeList->getNode().size()));
		enemy->setNodeList(mNodeList);
		LogManager::getSingleton().logMessage("Spawning...");
		enemy->processEvent(SPAWN_NPC,"","");
		LogManager::getSingleton().logMessage("Setting material!");
		enemy->setMaterialGroupID(enemyMat);
		LogManager::getSingleton().logMessage("Running started!");
		enemy->processEvent(RUNTO_NPC,"","");
		LogManager::getSingleton().logMessage("Npc Manager: end of NPC spawn");
		enemies.push_back(enemy);
	}
	if (className == "npc_neutral")
	{
		LogManager::getSingleton().logMessage("Npc Manager: npc neutral requested for a spawn:");
		npc_neutral* neutral = new npc_neutral();
		neutral->init(npc);
		LogManager::getSingleton().logMessage("Setting node list..."+StringConverter::toString(mNodeList->getNode().size()));
		neutral->setNodeList(mNodeList);
		LogManager::getSingleton().logMessage("Spawning...");
		neutral->processEvent(SPAWN_NPC,"","");
		LogManager::getSingleton().logMessage("Setting material!");
		neutral->setMaterialGroupID(neutralMat);
		LogManager::getSingleton().logMessage("Running started!");
		neutral->processEvent(RUNTO_NPC,"","");
		LogManager::getSingleton().logMessage("Npc Manager: end of NPC spawn");
		neutrals.push_back(neutral);
	}
	if (className == "npc_friend")
	{
		LogManager::getSingleton().logMessage("Npc Manager: npc friend requested for a spawn:");
		npc_friend* frnd = new npc_friend();
		frnd->init(npc);
		LogManager::getSingleton().logMessage("Setting node list..."+StringConverter::toString(mNodeList->getNode().size()));
		frnd->setNodeList(mNodeList);
		LogManager::getSingleton().logMessage("Spawning...");
		frnd->processEvent(SPAWN_NPC,"","");
		LogManager::getSingleton().logMessage("Setting material!");
		frnd->setMaterialGroupID(enemyMat);
		LogManager::getSingleton().logMessage("Running started!");
		frnd->processEvent(RUNTO_NPC,"","");
		LogManager::getSingleton().logMessage("Npc Manager: end of NPC spawn");
		friends.push_back(frnd);
	}
	if (className == "npc_aerial")
	{
		LogManager::getSingleton().logMessage("Npc Manager: npc aerial requested for a spawn:");
		npc_aerial* frnd = new npc_aerial();
		frnd->init(npc);
		LogManager::getSingleton().logMessage("Setting node list..."+StringConverter::toString(mNodeList->getNode().size()));
		frnd->setNodeList(mNodeList);
		LogManager::getSingleton().logMessage("Spawning...");
		frnd->processEvent(SPAWN_NPC,"","");
		LogManager::getSingleton().logMessage("Setting material!");
		frnd->setMaterialGroupID(enemyMat);
		LogManager::getSingleton().logMessage("Running started!");
		frnd->processEvent(RUNTO_NPC,"","");
		LogManager::getSingleton().logMessage("Npc Manager: end of NPC spawn");
		aerials.push_back(frnd);
	}
}

void NPCManager::npc_kill(String name)
{
	/*for (unsigned int i=0;i!=enemies.size();i++)
	{
		if (enemies[i]->getName())
	}*/

}

void NPCManager::npc_event(String name, String param1,String param2)
{
	for (unsigned int i=0;i!=enemies.size();i++)
	{
		if (enemies[i]->getName()==name)
		{
			enemies[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
		}
	}
	for (unsigned int i=0;i!=friends.size();i++)
	{
		if (friends[i]->getName()==name)
		{
			friends[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
		}
	}
	for (unsigned int i=0;i!=neutrals.size();i++)
	{
		if (neutrals[i]->getName()==name)
		{
			neutrals[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
		}
	}
	for (unsigned int i=0;i!=aerials.size();i++)
	{
		if (aerials[i]->getName()==name)
		{
			aerials[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
		}
	}
}

void NPCManager::npc_event(String name, String type, String param1,String param2)
{
	for (unsigned int i=0;i!=enemies.size();i++)
	{
		if (enemies[i]->getName()==name)
		{
			enemies[i]->processEvent(StringConverter::parseInt(type),param1,param2);
		}
	}
	for (unsigned int i=0;i!=friends.size();i++)
	{
		if (friends[i]->getName()==name)
		{
			friends[i]->processEvent(StringConverter::parseInt(type),param1,param2);
		}
	}
	for (unsigned int i=0;i!=neutrals.size();i++)
	{
		if (neutrals[i]->getName()==name)
		{
			neutrals[i]->processEvent(StringConverter::parseInt(type),param1,param2);
		}
	}
	for (unsigned int i=0;i!=aerials.size();i++)
	{
		if (aerials[i]->getName()==name)
		{
			aerials[i]->processEvent(StringConverter::parseInt(type),param1,param2);
		}
	}
}

void NPCManager::all_npc_event(String name, String param1,String param2)
{
	for (unsigned int i=0;i!=enemies.size();i++)
	{
			enemies[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
	}
	for (unsigned int i=0;i!=neutrals.size();i++)
	{
			neutrals[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
	}
	for (unsigned int i=0;i!=friends.size();i++)
	{
			friends[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
	}
	for (unsigned int i=0;i!=aerials.size();i++)
	{
			aerials[i]->processEvent(StringConverter::parseInt(param1),param2,param2);
	}
}

bool NPCManager::frameStarted(const Ogre::FrameEvent &evt)
{
	for (unsigned int i=0;i!=enemies.size();i++)
		enemies[i]->frameStarted(evt);
	for (unsigned int i=0;i!=friends.size();i++)
		friends[i]->frameStarted(evt);
	for (unsigned int i=0;i!=neutrals.size();i++)
		neutrals[i]->frameStarted(evt);
	for (unsigned int i=0;i!=aerials.size();i++)
		aerials[i]->frameStarted(evt);
	AIManager::getSingleton().update(evt.timeSinceLastFrame);
	#ifdef DEBUG_FACIAL
	LogManager::getSingleton().logMessage("neutral end step");
	#endif
	return true;
}