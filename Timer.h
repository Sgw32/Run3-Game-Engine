///////////////////////////////////////////////////////////////////////
//		Trigger volume without using OgreNewt! <(OgreNewt and NGD)OMG!>  //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		leave this comment if you want to use it in your own code!	 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "global.h"
#include "Timeshift.h"
//#include "DotScene.h"

class Run3Timer
{
public:
	Run3Timer()
	{
		enabled=false;
		noiseAmount=0;
	}
	~Run3Timer(){};
	void init(String name,String mlua, Real period)
	{
		mName=name;
		lua=mlua;
		mPeriod=period;
	}

	inline void setNoiseAmount(Real amount)
	{
		noiseAmount=amount;
	}

	void enable()
	{
		mCurPeriod=mPeriod;
		enabled=true;
	}

	void disable()
	{
		enabled=false;
	}

	void toggle()
	{
		enabled=!enabled;
	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt)
	{
		if (enabled)
		{
			mCurPeriod-=evt.timeSinceLastFrame*Timeshift::getSingleton().getTimeK();
			
			if (mCurPeriod<0)
			{
				
				mCurPeriod=mPeriod;
				if (noiseAmount)
				{
				mCurPeriod+=Ogre::Math::RangeRandom(0,noiseAmount);
				}
				RunLuaScript(global::getSingleton().getLuaState(),lua.c_str());
			}
		}
		return true;
	}
	String mName;
private:
	
	String lua;
	Real mPeriod;
	Real noiseAmount;
	Real mCurPeriod;
	bool enabled;
};