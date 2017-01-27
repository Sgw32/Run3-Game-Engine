#pragma once
#include "Ogre.h"
#include "Manager_Template.h"
#include "Timeshift.h"

using namespace Ogre;
using namespace std;

class BulletHit
{
public:
	BulletHit(Vector3 pos);
	BulletHit(Vector3 pos,Real m);
	BulletHit(Vector3 pos,String templName);
	BulletHit(Vector3 pos,Real m,String f);
	~BulletHit();
	void start();
	void stopAndKill();
private:
	ParticleSystem* bSys;
	SceneNode* bNode;
};

class BulletHitManager : public Singleton<BulletHitManager>, public managerTemplate
{
public:
	BulletHitManager();
	~BulletHitManager();
	virtual void init();
	void addBulletEffect(Vector3 pos,Real sec);
	void addBulletEffect(Vector3 pos,Real sec, Real m);
	void addBulletEffect(Vector3 pos,Real sec, Real m,String f);
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	map<BulletHit*, Real> bullethits;
};