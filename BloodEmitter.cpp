#include "BloodEmitter.h"

template<> BloodEmitter *Singleton<BloodEmitter>::ms_Singleton=0;

BloodEmitter::BloodEmitter()
{
	startemit=false;
}

BloodEmitter::~BloodEmitter()
{
}

void BloodEmitter::init()
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
bSys=mSceneMgr->createParticleSystem("BloodEmit", "blood01");
bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode("bloodEmitter");
bNode->attachObject(bSys);
bSys->setVisible(false);
}

void BloodEmitter::emitBlood(Vector3 pos,Vector3 size)
{
	bNode->setPosition(pos);
	bNode->setScale(size);
	bSys->setVisible(true);
	startemit=true;
	timePos=BLOOD_DURATION;
}


void BloodEmitter::upd(const Ogre::FrameEvent &evt)
{
if (startemit)
{
timePos-=evt.timeSinceLastFrame*TIME_SHIFT;
if (timePos<0)
{
	startemit=false;
	bSys->setVisible(false);
}
}
}