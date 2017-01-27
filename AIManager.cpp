#include "AIManager.h"
#include "NPCManager.h"

template<> AIManager *Singleton<AIManager>::ms_Singleton=0;

AIManager::AIManager()
{
mNodeGen=0;
}

AIManager::~AIManager()
{

}

void AIManager::init(int AIType,Vector3 nodeGen,bool autoGenNodes)
{
	nList=global::getSingleton().getNodeList();
	pAiVersion=AIType;
	switch (AIType)
	{
	case AIR3_SIMPLE:
		
		break;
	case AIR3_EXTENDED:
		break;
	default:
		break;
	}
	mPathFinder=new AirPathFind();
	mPathFinder->setWorld(global::getSingleton().getWorld());
	mPathFinder->setNodes(NPCNODES);
	if (autoGenNodes)
	{
		this->mNodeGen=new nodeGenerator(nodeGen,global::getSingleton().getSceneManager(),300,true);
		mNodeGen->setNodeList(nList);
		mNodeGen->generate_nodes();
	}
}

void AIManager::processGroupCommands(String name)
{
	LogManager::getSingleton().logMessage("processing group events...");
		for (unsigned int i=0;i!=grps.size();i++)
		{
			if (grps[i]->getName()==name)
			{
				AIElementGroup* grp=grps[i];
				for (unsigned int j=0;j!=grp->nodes.size();j++)
				{
					AIElement* aiel = grp->nodes[j];
					LogManager::getSingleton().logMessage("processing group element..."+aiel->getName());
					LogManager::getSingleton().logMessage(aiel->getCommand1());
					LogManager::getSingleton().logMessage(aiel->getCommand2());
					NPCManager::getSingleton().npc_event(aiel->getName(),aiel->getCommand1(),aiel->getCommand2());
					
				}
			}
		}
}

AIElementGroup* AIManager::getNPCGroupByNPCName(String name)
{
	for (unsigned int i=0;i!=grps.size();i++)
		{
			if (grps[i]->getName()==name)
			{
				AIElementGroup* grp=grps[i];
				for (unsigned int j=0;j!=grp->nodes.size();j++)
				{
					AIElement* aiel = grp->nodes[j];
					if (name==aiel->getName())
					{
						return grp;
					}
				}
			}
		}
		return 0;
}

AIElement* AIManager::getAIElementByNPCName(String name)
{
	for (unsigned int i=0;i!=grps.size();i++)
		{
				AIElementGroup* grp=grps[i];
				for (unsigned int j=0;j!=grp->nodes.size();j++)
				{
					AIElement* aiel = grp->nodes[j];
					if (name==aiel->getName())
					{
						return aiel;
					}
				}
		}
		return 0;
}

void AIManager::update(Real evt)
{
	for (unsigned int i=0;i!=grps.size();i++)
	{
		//LogManager::getSingleton().logMessage("step group element...");
		grps[i]->step(evt);
	}
}