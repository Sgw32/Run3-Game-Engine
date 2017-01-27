#pragma once

#include <Ogre.h>
#include <vector>

using namespace Ogre;
using namespace std;

#define maxObjectsPerBatch 80

class Run3Batcher: public Singleton<Run3Batcher>
{
public:
	Run3Batcher();
	~Run3Batcher();
	void init(SceneManager* sceneMgr)
	{
		mSceneMgr=sceneMgr;
	}
	void prepareBatch();
	void addObject(Entity* ent,Vector3 pos,Vector3 scale,Quaternion quat);
	void buildBatch();
	void destroyBatch();
private:
	bool empty;
	StaticGeometry* geom;
	vector<Entity*> ents;
	SceneManager* mSceneMgr;
};