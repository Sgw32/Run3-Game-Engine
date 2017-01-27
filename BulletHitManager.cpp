#include "BulletHitManager.h"
#include "global.h"

template<> BulletHitManager *Ogre::Singleton<BulletHitManager>::ms_Singleton=0;

BulletHit::BulletHit(Vector3 pos)
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	bNode->setScale(0.1,0.1,0.1);
	bSys=mSceneMgr->createParticleSystem(bNode->getName(), "bulletHit2");
	bNode->attachObject(bSys);
}

BulletHit::BulletHit(Vector3 pos,Real m)
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	bNode->setScale(0.1*m,0.1*m,0.1*m);
	bSys=mSceneMgr->createParticleSystem(bNode->getName(), "bulletHit2");
	bNode->attachObject(bSys);
}

BulletHit::BulletHit(Vector3 pos,Real m,String f)
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	bNode->setScale(0.1*m,0.1*m,0.1*m);
	bSys=mSceneMgr->createParticleSystem(bNode->getName(), f);
	bNode->attachObject(bSys);
}

BulletHit::~BulletHit()
{
}

void BulletHit::start()
{
	
	bSys->setVisible(true);
}

void BulletHit::stopAndKill()
{
	bNode->detachAllObjects();
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	mSceneMgr->destroySceneNode(bNode);
	mSceneMgr->destroyParticleSystem(bSys);
}

BulletHitManager::BulletHitManager()
{

}

BulletHitManager::~BulletHitManager()
{

}

void BulletHitManager::init()
{

}

void BulletHitManager::addBulletEffect(Vector3 pos,Real sec)
{
	if (bullethits.size()>10)
		return;
	BulletHit* hit = new BulletHit(pos);
	hit->start();
	bullethits[hit]=sec;
}

void BulletHitManager::addBulletEffect(Vector3 pos,Real sec, Real m)
{
	if (bullethits.size()>10)
		return;
	BulletHit* hit = new BulletHit(pos,m);
	hit->start();
	bullethits[hit]=sec;
}

void BulletHitManager::addBulletEffect(Vector3 pos,Real sec, Real m,String f)
{
	if (bullethits.size()>10)
		return;
	BulletHit* hit = new BulletHit(pos,m,f);
	hit->start();
	bullethits[hit]=sec;
}

void BulletHitManager::upd(const FrameEvent& evt)
{
	map<BulletHit*, Real>::iterator i;
	for (i=bullethits.begin();i!=bullethits.end();i++)
	{
		(*i).second-=evt.timeSinceLastFrame*TIME_SHIFT;
		if ((*i).second<0)
		{
			BulletHit* gib = (*i).first;
			gib->stopAndKill();
			delete gib;
			bullethits.erase(i);
		}
	}
}

void BulletHitManager::cleanup()
{
map<BulletHit*, Real>::iterator i;
for (i=bullethits.begin();i!=bullethits.end();i++)
	{
		BulletHit* gib = (*i).first;
			bullethits.erase(i);
			gib->stopAndKill();
			delete gib;
	}
}


