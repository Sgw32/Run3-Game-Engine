/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once

#include "Ogre.h"
#include "global.h"
#include "SharedLuaCallback.h"

using namespace Ogre;

class Credits: public Singleton<Credits>
{
public:
	Credits();
	~Credits();
	void init();
	void start(String fileName);
	void pause();
	void stop();
	void update(const Ogre::FrameEvent &evt);
	void cleanup();
	static int startCredits(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
			SharedLuaCallback::getSingleton().shared_startCredits(lua_tostring(pL, 1));//toWrite=
		//global::getSingleton().getRoot()->renderOneFrame();
		return 1;
	}
private:
	ConfigFile cf;
	bool started;
	Real char_height;
	Real speed;
	Real timeElapsed;
	Overlay* credOv;
	OverlayElement* creditTextItem;
	String cred_data;
};