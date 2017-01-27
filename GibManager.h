#pragma once
#include "Ogre.h"
#include "Manager_Template.h"
#include "OgreNewt.h"
#include "Timeshift.h"

using namespace OgreNewt;
using namespace Ogre;
using namespace std;

#define GIB_BODY 31

class Gib
{
public:
	Gib(String meshFile,Vector3 pos, Vector3 scale);
	~Gib();
	void kill();
private:
	void* physObject;
};

class GibManager: public Singleton<GibManager>, public managerTemplate
{
public:
	GibManager(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
	GibManager();
	virtual ~GibManager();
	virtual void init();
	void spawnGib(String meshFile,Vector3 pos,Vector3 scale, Real lifetime);
	void spawnGibs(String meshFile,Vector3 pos,Vector3 scale,Real lifetime,unsigned int num, Real maxdist);
	void deleteAllGibs();
//	virtual void create
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	std::map<Gib*,Real> gibs;
	SceneNode* gib;
};