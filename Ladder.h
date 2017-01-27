///////////////////////////////////////////////////////////////////////
//		LADDER														  //
//		Started 02.06.2015											 //
//		by Sgw32								(all rights reserved)//
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////


#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include "global.h"

using namespace Ogre;
using namespace OgreNewt;
using namespace std;

/*class ladderMatCallback :
	public OgreNewt::ContactCallback
{
public:
	ladderMatCallback(void);
	~ladderMatCallback(void);

	// in this example we only need the Process() function.
	int userProcess();
};*/



//namespace Run3
//{
class Ladder
{
public:
	Ladder();
	~Ladder();
	void init(SceneNode* node,int type);
	void init(SceneNode* node,int type,SceneNode* parent);
	
	
	void dispose()
	{
		showFrame=false;
		String name = mNode->getName();

		mNode->detachAllObjects();
		if (mSceneMgr->hasEntity(name))
			mSceneMgr->destroyEntity(name);
		mSceneMgr->destroySceneNode(mNode);
		delete bod;
		
		
	}
	bool frameStarted(const Ogre::FrameEvent &evt);
private:
	bool showFrame;
	World* mWorld;
	SceneManager* mSceneMgr;
	OgreNewt::Body* bod;
	SceneNode* mNode;
	SceneNode* mparent;
	bool p;
	Vector3 d_b;
};
