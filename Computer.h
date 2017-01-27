#pragma once
#include "Ogre.h"
//#include "global.h" //already included in Display header
#include "Display.h"
#include "HUD.h"
#include "MagicManager.h"

extern "C"
{
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}

using namespace Ogre;
using namespace OgreNewt;

#ifndef _COMPUTER_H_
#define _COMPUTER_H_

class Computer  //: public FrameListener
{
public:
	Computer();
	~Computer();
	void init(SceneNode* cComp,int type,String fileName);
	void init(SceneNode* cComp,int type,String fileName,String dispMatName);
	void setName(String name)
	{
		mName=name;
	}
	String getName(void)
	{
		return mName;
	}
	void power();
	void setNearScript(String lua)
	{
		mNearLua=lua;
	}
	void setShutdownScript(String lua)
	{
		msLua=lua;
	}
	void logoff();
	void explode();
	void reset();
	void del();
	void setAllowVirtualDisplay(bool allow)
	{
		allowVirtualDisplay=allow;
	}
	SceneNode* getComp()
	{
		return mComp;
	}
	void activateMagic(bool act)
	{
		mActMagic=act;
	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);
	String mDispMatName;
private:
	bool mActMagic;
	bool notice;
	bool allowVirtualDisplay;
	String mNearLua;
	String mName;
	String msLua;
	SceneNode* mComp;
	OgreNewt::Body* bod;
	OgreNewt::World* mWorld;
	String luaFlName;
	Real mFov;
	lua_State* pLuaState;
};

#endif