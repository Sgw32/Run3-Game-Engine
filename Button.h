///////////////////////////////////////////////////////////////////////
//		BUTTON														  //
//		Started 25.02.2010											 //
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
#include "ButtonContactMatCallback.h"

using namespace Ogre;
using namespace OgreNewt;
using namespace std;

namespace Run3
{
class Button
{
public:
	Button();
	~Button();
	void init(SceneNode* node,int type);
	void initMovable(SceneNode* node,int type,Real mass);
	void init(SceneNode* node,int type,SceneNode* parent);
	void use();
	void setCallback(String luaScript);
	void Button::camera_force_callback( OgreNewt::Body* me);
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
	void setAlarmOnContact()
	{
		if (bod)
		{
		// Bomb, for example. 
		// "Use" on contact.
		mPhysCallback = new ButtonContactMatCallback();
		OgreNewt::World* mWorld = global::getSingleton().getWorld();
		mMatDefault = global::getSingleton().getPlayer()->getPlayerMat();
		neutralMat= new OgreNewt::MaterialID( mWorld );
		mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, neutralMat);
		mMatPair->setContactCallback( mPhysCallback );
		mMatPair = new OgreNewt::MaterialPair( mWorld, mWorld->getDefaultMaterialID(), neutralMat );
		mMatPair->setContactCallback( mPhysCallback );
		mMatPair->setDefaultFriction(0,0);

		bod->setMaterialGroupID(neutralMat);
		}
	}
private:
	const OgreNewt::MaterialID* mMatDefault;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* neutralMat;
	ButtonContactMatCallback* mPhysCallback;
	bool showFrame;
	World* mWorld;
	SceneManager* mSceneMgr;
	OgreNewt::Body* bod;
	String mLuaScript;
	lua_State* pLuaState;
	SceneNode* mNode;
	SceneNode* mparent;
	bool p;
	Vector3 d_b;
};
}

