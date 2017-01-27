///////////////////////////////////////////////////////////////////////
//		Dark zones darken the ambient light using a distrubution function//
//		by Sgw32													 //
//		leave this comment if you want to use it in your own code!	 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <map>
//#include "global.h"

using namespace Ogre;
using namespace std;

#define EXPONENT_CONSTANT_DARKEN 0.1 // for 2d, f(x,y)=1-darken*exp(-(((x-x0)/h)^2+((y-y0)/w)^2)/EXPONENT_CONSTANT_DARKEN)

class DarkZone
{
public:
	DarkZone(SceneManager* sceneMgr, Real x0, Real y0, Real z0,Real height, Real width, Real depth, Real darken, Real exp)
	{
		mSceneMgr=sceneMgr;
		//ColourValue mAmbiLight = mSceneMgr->getAmbientLight();
		mX0=x0;
		mY0=y0;
		mZ0=z0;
		mHeight=height;
		mWidth=width;
		mDepth=depth;
		mDarken=darken;
		mExp=exp;
	}
	DarkZone(SceneManager* sceneMgr, Vector3 pos,Vector3 scale, Real darken, Real exp)
	{
		mSceneMgr=sceneMgr;
		//ColourValue mAmbiLight = mSceneMgr->getAmbientLight();
		mX0=pos.x;
		mY0=pos.y;
		mZ0=pos.z;
		mHeight=scale.x;
		mWidth=scale.y;
		mDepth=scale.z;
		mDarken=darken;
		mExp=exp;
	}
	~DarkZone();
	Real computeDarken(Ogre::Vector3 pos)
	{
		return 1 - mDarken*exp(-(((pos.x-mX0)/mHeight)*((pos.x-mX0)/mHeight)+
								 ((pos.y-mY0)/mWidth)*((pos.y-mY0)/mWidth)+
								 ((pos.z-mZ0)/mDepth)*((pos.z-mZ0)/mDepth))/mExp);
	}
	void applyDarken(Ogre::Vector3 pos)
	{
		Real dken = computeDarken(pos);
		if (dken>0)
		{
			//LogManager::getSingleton().logMessage(StringConverter::toString(mAmbiLight));
			//LogManager::getSingleton().logMessage(StringConverter::toString(mAmbiLight*dken));
			ColourValue ambiLight = mSceneMgr->getAmbientLight()*dken;
			mSceneMgr->setAmbientLight(ambiLight);
			std::map<Light*,ColourValue>::iterator i;
			for (i=lights.begin();i!=lights.end();i++)
			{
				(*i).first->setDiffuseColour((*i).second*dken);
			}
		}
		else
		{
			LogManager::getSingleton().logMessage("Dark Zone cut!");
			mSceneMgr->setAmbientLight(ColourValue::Black);
		}
	}
	void addLight(Light* l)
	{
		lights[l]=l->getDiffuseColour();
	}
	void clearLights()
	{
		lights.clear();
	}
	void applyDarken(ColourValue &ambi,Ogre::Vector3 pos)
	{
		Real dken = computeDarken(pos);
		if (dken>0)
		{
			//LogManager::getSingleton().logMessage(StringConverter::toString(mAmbiLight));
			//LogManager::getSingleton().logMessage(StringConverter::toString(mAmbiLight*dken));
			ColourValue ambiLight = ambi*dken;
			ambi=ambiLight;
		}
		else
		{
			LogManager::getSingleton().logMessage("Dark Zone cut!");
			ambi=ColourValue::Black;
		}
	}
	String mName;
private:
	map<Light*,ColourValue> lights;
	SceneManager* mSceneMgr;
	ColourValue mAmbiLight;
	Real mX0,mY0,mZ0,mHeight,mWidth,mDepth,mDarken,mExp;
};