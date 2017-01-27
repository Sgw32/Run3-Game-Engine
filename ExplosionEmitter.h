#pragma once
#include "global.h"
#include "Timeshift.h"

#define Explosion_DURATION 1.0f

class ExplosionEmitter
{
public:
	ExplosionEmitter();
	~ExplosionEmitter();
	void init();
	void emitExplosion(Vector3 pos,Vector3 size);
	void emitExplosion(Vector3 pos,Vector3 size,String explosionSound);
	//void emitExplosion(Vector3 pos,Vector3 size,String explosionSound);
	void init(String particle_file);
	void upd(const Ogre::FrameEvent &evt);
	bool isDispose(){return mDispose;}
	void destroyEmitter()
	{
		startemit=false;
		SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
		if (bSys)
		mSceneMgr->destroyParticleSystem(bSys);
		if (bNode)
		mSceneMgr->destroySceneNode(bNode);
	}
private:
	ParticleSystem* bSys;
	SceneNode* bNode;
	bool startemit;
	bool mDispose;
	Real timePos;
};