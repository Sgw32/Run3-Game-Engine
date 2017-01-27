/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include "global.h"
#include "SharedLuaCallback.h"

class SuperFX: public Singleton<SuperFX>
{
public:
	SuperFX();
	~SuperFX();
	void init();
	void turn_off_all();
	void setBloomEnabled(bool enabled);
	bool getBloomEnabled(){return BloomEnabled;}
	void setHDREnabled(bool enabled);
	bool getHDREnabled(){return HDREnabled;};
	void setOldTvEnabled(bool enabled);
	bool getOldTVEnabled(){return OldTVEnabled;};
	void setNightvisionEnabled(bool enabled);
	bool getNightvisionEnabled(){return NightvisionEnabled;}
	void allowMotionBlur(bool enabled){allowMb=enabled;}
	void toggleRadialBlur();
	void toggleNightvision();
	void toggleBloom();
	void toggleOldTV();
	void toggleMotionBlur();
//LUA
	static int toggleNightvision(lua_State* pL)
	{
		SharedLuaCallback::getSingleton().shared_toggleNightvision();
		return 1;
	}

private:
	bool allowMb;
	bool BloomEnabled,HDREnabled,NightvisionEnabled,OldTVEnabled,radBlurEnabled,motionBlur;
	Ogre::CompositorPtr comp3;
};