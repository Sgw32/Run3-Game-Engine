#include "GibManager.h"
#include "global.h"

template<> GibManager *Ogre::Singleton<GibManager>::ms_Singleton=0;

Gib::Gib(String meshFile,Vector3 pos, Vector3 scale)
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	World* mWorld = global::getSingleton().getWorld();

	SceneNode* gib = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	gib->setScale(scale);
	PhysObject* phys = new PhysObject;
	try
	{
		MeshManager::getSingleton().load(meshFile, "General");
		phys->init(mSceneMgr,mWorld);
		phys->CreateObject(gib->getName(),meshFile,gib,40.0f,false,"idle");
		phys->bod->setType(GIB_BODY);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading a physics object!");
	}
	physObject=phys;
}


Gib::~Gib()
{
}

void Gib::kill()
{
	PhysObject* phys = (PhysObject*)physObject;
	if (phys)
	{
		phys->forceDelete();
		delete phys;
		phys=0;
	}
}


GibManager::GibManager()
{
}

GibManager::~GibManager()
{
}

void GibManager::init()
{
	LogManager::getSingleton().logMessage("GibManager initialized!");
}

void GibManager::spawnGib(String meshFile,Vector3 pos,Vector3 scale, Real lifetime)
{
	if (lifetime>0)
	{
	Gib* gib = new Gib(meshFile,pos,scale);
	gibs[gib]=lifetime;
	}
}

void GibManager::spawnGibs(String meshFile,Vector3 pos,Vector3 scale,Real lifetime,unsigned int num, Real maxdist)
{
	LogManager::getSingleton().logMessage("Gibs:"+StringConverter::toString(pos)+" "+StringConverter::toString(num)+" "+StringConverter::toString(maxdist));
	for (unsigned int i=0;i!=num;i++)
	{
		Ogre::Vector3 p(
                pos.x + Ogre::Math::RangeRandom(-maxdist, maxdist),
                pos.y + Ogre::Math::RangeRandom(-maxdist, maxdist),
                pos.z + Ogre::Math::RangeRandom(-maxdist, maxdist));
		Gib* gib = new Gib(meshFile,p,scale);
		gibs[gib]=lifetime;
	}
}

void GibManager::deleteAllGibs()
{
	std::map<Gib*,Real>::iterator i;
	for (i=gibs.begin();i!=gibs.end();i++)
	{
		Gib* gib = (*i).first;
		gib->kill();
		delete gib;
	}
	gibs.clear();
}

void GibManager::upd(const FrameEvent& evt)
{
	map<Gib*,Real>::iterator i;
	for (i=gibs.begin();i!=gibs.end();i++)
	{
		(*i).second-=evt.timeSinceLastFrame*TIME_SHIFT;
		if ((*i).second<0)
		{
			Gib* gib = (*i).first;
			gibs.erase(i);
			gib->kill();
			delete gib;
		}
	}
}

void GibManager::cleanup()
{
}