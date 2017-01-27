#include "MeshDecalMgr.h"
#include "global.h"

template<> MeshDecalMgr *Ogre::Singleton<MeshDecalMgr>::ms_Singleton=0;

MeshDecal::MeshDecal(Vector3 v1,Vector3 v2)
{
	
	/*LogManager::getSingleton().logMessage("Decal sprayed!");
	
	Ogre::Vector3 dir = v2-v1;
	dir.y=0;

Ogre::Vector3 pos = v1+(v2-v1)/2;
LogManager::getSingleton().logMessage(StringConverter::toString(dir));
LogManager::getSingleton().logMessage(StringConverter::toString(v1)+" "+StringConverter::toString(v2));
	bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	bNode->setScale(0.3,0.3,0.3);
	decal=mSceneMgr->createEntity(bNode->getName(),"decal03_bullet01.mesh");
	bNode->attachObject(decal);
	Real zx = dir.x;
	/*dir.x=dir.z;
	dir.z=zx;
	bNode->setDirection(dir);
	SceneNode* n2 = mSceneMgr->getRootSceneNode()->createChildSceneNode(v1);
	SceneNode* n3 = mSceneMgr->getRootSceneNode()->createChildSceneNode(v2);
	Entity* box1 = mSceneMgr->createEntity(n2->getName(),"box.mesh");
	Entity* box2 = mSceneMgr->createEntity(n3->getName(),"box.mesh");
	n2->attachObject(box1);
	n3->attachObject(box2);
	n2->setScale(1,1,1);
	n3->setScale(2,2,2);*/
SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode(v1);
	bNode->setScale(0.3,0.3,0.3);
	decal=mSceneMgr->createEntity(bNode->getName(),"decal03_bullet01.mesh");
	bNode->attachObject(decal);
	bNode->setDirection(v2);
	//bNode->roll(Degree(-90));

}

MeshDecal::~MeshDecal()
{
	LogManager::getSingleton().logMessage("Decal destroyed!");
	bNode->detachAllObjects();
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	mSceneMgr->destroySceneNode(bNode);
	mSceneMgr->destroyEntity(decal);
}

MeshDecalMgr::MeshDecalMgr()
{

}

MeshDecalMgr::~MeshDecalMgr()
{

}

void MeshDecalMgr::init()
{

}

void MeshDecalMgr::addHoleEffect(Vector3 v1,Vector3 v2)
{
	//LogManager::getSingleton().logMessage(StringConverter::toString(pos));
	/*if (meshDecals.size()>MAX_DECALS)
	{
		MeshDecal* h = meshDecals.begin();
		delete h;
		meshDecals.erase(meshDecals.begin());
	}*/
	MeshDecal* hit = new MeshDecal(v1,v2);
	meshDecals.push_back(hit);

}

void MeshDecalMgr::upd(const FrameEvent& evt)
{
	
}

void MeshDecalMgr::cleanup()
{
	LogManager::getSingleton().logMessage("MeshDecal cleanup");
	vector<MeshDecal*>::iterator i;
	for (i=meshDecals.begin();i!=meshDecals.end();i++)
	{
		delete (*i);
	}
meshDecals.clear();
}


