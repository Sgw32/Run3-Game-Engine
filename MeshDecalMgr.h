#pragma once
#include "Ogre.h"
#include "Manager_Template.h"

using namespace Ogre;
using namespace std;

#define MAX_DECALS 50

class MeshDecal
{
public:
	MeshDecal(Vector3 v1,Vector3 v2);
	~MeshDecal();
private:
	Entity* decal;
	SceneNode* bNode;
};

class MeshDecalMgr : public Singleton<MeshDecalMgr>, public managerTemplate
{
public:
	MeshDecalMgr();
	~MeshDecalMgr();
	virtual void init();
	void addHoleEffect(Vector3 v1,Vector3 v2,String matName);
	void addHoleEffect(Vector3 v1,Vector3 v2);
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	vector<MeshDecal*> meshDecals;
};