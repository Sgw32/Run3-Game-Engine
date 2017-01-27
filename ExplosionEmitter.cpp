#include "ExplosionEmitter.h"
#include "Run3SoundRuntime.h"

ExplosionEmitter::ExplosionEmitter()
{
	startemit=false;
	mDispose=false;
}

ExplosionEmitter::~ExplosionEmitter()
{
}

void ExplosionEmitter::init()
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
//bSys=mSceneMgr->createParticleSystem("ExplosionEmit", "explosion01");
bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode();
bSys=mSceneMgr->createParticleSystem(bNode->getName(), "explosion01");
bNode->attachObject(bSys);
bSys->setVisible(false);
}

void ExplosionEmitter::emitExplosion(Vector3 pos,Vector3 size)
{
	bNode->setPosition(pos);
	bNode->setScale(size);
	bSys->setVisible(true);
	startemit=true;
	timePos=bSys->getEmitter(0)->getMaxDuration()+bSys->getEmitter(0)->getMaxTimeToLive();
	Run3SoundRuntime::getSingleton().emitSound("run3/sounds/exp1.wav",timePos,false,pos,size.x*100,size.x*100);
	global::getSingleton().mBw->blast(pos,size*100,size.x*40);
}

void ExplosionEmitter::emitExplosion(Vector3 pos,Vector3 size,String explosionSound)
{
	bNode->setPosition(pos);
	bNode->setScale(size);
	bSys->setVisible(true);
	startemit=true;
	timePos=bSys->getEmitter(0)->getMaxDuration()+bSys->getEmitter(0)->getMaxTimeToLive();
	Run3SoundRuntime::getSingleton().emitSound(explosionSound,timePos,false,pos,size.x*100,size.x*100);
	global::getSingleton().mBw->blast(pos,size*100,size.x*40);
}

void ExplosionEmitter::init(String particle_file)
{
SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
//bSys=mSceneMgr->createParticleSystem("ExplosionEmit", "explosion01");
bNode=mSceneMgr->getRootSceneNode()->createChildSceneNode();
bSys=mSceneMgr->createParticleSystem(bNode->getName(), particle_file);
bNode->attachObject(bSys);
bSys->setVisible(false);
}

void ExplosionEmitter::upd(const Ogre::FrameEvent &evt)
{
if (startemit)
{
Real tShift = TIME_SHIFT;
timePos-=evt.timeSinceLastFrame*tShift;
if (tShift<1.0f)
{
	bSys->getEmitter(0)->setEmissionRate(bSys->getEmitter(0)->getEmissionRate()*tShift);
	bSys->getEmitter(0)->setTimeToLive(bSys->getEmitter(0)->getTimeToLive()*(2-tShift));
	bSys->getEmitter(0)->setParticleVelocity(bSys->getEmitter(0)->getParticleVelocity()*tShift);
	bSys->getEmitter(0)->setDuration(bSys->getEmitter(0)->getDuration()*(2-tShift));
}
if (tShift>1.0f)
{
	bSys->getEmitter(0)->setEmissionRate(bSys->getEmitter(0)->getEmissionRate()*tShift);
	bSys->getEmitter(0)->setTimeToLive(bSys->getEmitter(0)->getTimeToLive()/tShift);
	bSys->getEmitter(0)->setParticleVelocity(bSys->getEmitter(0)->getParticleVelocity()*tShift);
	bSys->getEmitter(0)->setDuration(bSys->getEmitter(0)->getDuration()/tShift);
}
if (timePos<0)
{
	bSys->setVisible(false);
	destroyEmitter();
}
}
}