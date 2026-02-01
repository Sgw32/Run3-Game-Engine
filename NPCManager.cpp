#include "NPCManager.h"
#include "NPCFactory.h"

template<> NPCManager *Singleton<NPCManager>::ms_Singleton=0;

NPCManager::NPCManager()
{
	rest=false;
	dt=0;
	mStep=0.05; // ~20FPS
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

  // Register default NPC types
  NPCFactory::getSingleton().registerType("npc_enemy", [](){ return new npc_enemy(); });
  NPCFactory::getSingleton().registerType("npc_friend", [](){ return new npc_friend(); });
  NPCFactory::getSingleton().registerType("npc_neutral", [](){ return new npc_neutral(); });
  NPCFactory::getSingleton().registerType("npc_aerial", [](){ return new npc_aerial(); });

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

        npc_template* baseNpc = NPCFactory::getSingleton().create(className);
        if(!baseNpc) {
                LogManager::getSingleton().logMessage("Unknown NPC type: "+className);
                return;
        }
        baseNpc->init(npc);
        baseNpc->setNodeList(mNodeList);
        baseNpc->processEvent(SPAWN_NPC,"","");
        if(className == "npc_neutral")
                baseNpc->setMaterialGroupID(neutralMat);
        else
                baseNpc->setMaterialGroupID(enemyMat);
        baseNpc->processEvent(RUNTO_NPC,"","");

        if(className == "npc_enemy")
                enemies.push_back(static_cast<npc_enemy*>(baseNpc));
        else if(className == "npc_friend")
                friends.push_back(static_cast<npc_friend*>(baseNpc));
        else if(className == "npc_neutral")
                neutrals.push_back(static_cast<npc_neutral*>(baseNpc));
        else if(className == "npc_aerial")
                aerials.push_back(static_cast<npc_aerial*>(baseNpc));
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
	if (rest)
		return true;
	dt+=evt.timeSinceLastFrame;
	if (dt<mStep)
		return true;
	dt=0.0f;
	Ogre::FrameEvent evt2;
	evt2.timeSinceLastFrame = mStep;
	for (unsigned int i=0;i!=enemies.size();i++)
		enemies[i]->frameStarted(evt2);
	for (unsigned int i=0;i!=friends.size();i++)
		friends[i]->frameStarted(evt2);
	for (unsigned int i=0;i!=neutrals.size();i++)
		neutrals[i]->frameStarted(evt2);
	for (unsigned int i=0;i!=aerials.size();i++)
		aerials[i]->frameStarted(evt2);
	AIManager::getSingleton().update(evt2.timeSinceLastFrame);
	#ifdef DEBUG_FACIAL
	LogManager::getSingleton().logMessage("neutral end step");
	#endif
	return true;
}