#include "Run3Batcher.h"

template<> Run3Batcher *Singleton<Run3Batcher>::ms_Singleton=0;

Run3Batcher::Run3Batcher()
{
	empty=true;
}

Run3Batcher::~Run3Batcher()
{
}

void Run3Batcher::prepareBatch()
{
	LogManager::getSingleton().logMessage("Run3Batcher: Preparing Static Batch for scene...");
	if (empty)
	{
		LogManager::getSingleton().logMessage("Run3Batcher: Created a new StaticGeom instance");
	geom = new StaticGeometry (mSceneMgr, "Run3StaticGeom");
	//Please fix if problems!!
	geom->setRegionDimensions (Vector3(100000, 100000, 100000));
	}
}

void Run3Batcher::addObject(Entity* ent,Vector3 pos,Vector3 scale,Quaternion quat)
{
	LogManager::getSingleton().logMessage("Run3Batcher: Adding object to batch...");
	//LogManager::getSingleton().logMessage(StringConverter::toString(pos)+" "+StringConverter::toString(scale)+StringConverter::toString(quat));
	empty=false;
	//LogManager::getSingleton().logMessage(geom->getName());
	geom->addEntity(ent, pos,quat,scale);
	LogManager::getSingleton().logMessage("Run3Batcher: Succesfully added.");
	ents.push_back(ent);
}

void Run3Batcher::buildBatch()
{
	if (!empty)
	{
	LogManager::getSingleton().logMessage("Run3Batcher: Building a static batch.");

	geom->setCastShadows(true);
	geom->build ();
	//geom
	LogManager::getSingleton().logMessage("Run3Batcher: Succesfully built!");
	vector<Entity*>::iterator i;
	for (i=ents.begin();i!=ents.end();i++)
		mSceneMgr->destroyEntity(*i);
	ents.clear();
	LogManager::getSingleton().logMessage("Run3Batcher: Succesfully built!");
	}
	
}

void Run3Batcher::destroyBatch()
{
	LogManager::getSingleton().logMessage("Run3Batcher: Deleting batch.");
	if (!empty)
	{
	if (geom)
	delete geom;
	empty=true;
	}
	LogManager::getSingleton().logMessage("Run3Batcher: Deleted.");

}