#pragma once
#include "Ogre.h"

using namespace Ogre;

class Fire
{
public:
	Fire(SceneManager* scene,Vector3 pos,Vector3 size,String psys,Real renderDist)
	{
		mSceneMgr=scene;
		bNode=scene->getRootSceneNode()->createChildSceneNode(pos);
		bNode->setScale(size);
		fire_s=scene->createParticleSystem(bNode->getName(), psys);
		bNode->attachObject(fire_s);
		fire_s->getEmitter(0)->setEnabled(false);
		fire_s->setRenderingDistance(renderDist);
	}
	Fire(SceneManager* scene,SceneNode* pnode,String psys,Real renderDist)
	{
		mSceneMgr=scene;
		bNode=pnode;
		fire_s=scene->createParticleSystem(bNode->getName(), psys);
		bNode->attachObject(fire_s);
		fire_s->getEmitter(0)->setEnabled(false);
		fire_s->setRenderingDistance(renderDist);
	}
	~Fire(){};
	void fire()
	{
		fire_s->getEmitter(0)->setEnabled(true);
	}
	void extinguish()
	{
		fire_s->getEmitter(0)->setEnabled(false);
	}
	void toggle()
	{
		if (fire_s->getEmitter(0)->getEnabled())
		{
			fire_s->getEmitter(0)->setEnabled(false);
		}
		else
		{
			fire_s->getEmitter(0)->setEnabled(true);
		}
	}
	void destroy()
	{
		bNode->detachAllObjects();
		if (fire_s)
		mSceneMgr->destroyParticleSystem(fire_s);
		if (bNode)
		mSceneMgr->destroySceneNode(bNode);
	}
	String getName()
	{
		return mName;
	}
	void setName(String name)
	{
		mName=name;
	}
private:
	SceneManager* mSceneMgr;
	ParticleSystem* fire_s;
	SceneNode* bNode;
	String mName;
};