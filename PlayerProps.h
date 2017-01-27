/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "Ogre.h"
#ifndef __PlayerProps_H__
#define __PlayerProps_H__

class PlayerProps
{
public:
	// standard constructor
	inline PlayerProps()
	{
		health=100;
		upvectors=true;
		inertia_active=true;
		allowflashlight=true;
		kinematic_movement=false;
	}
	// overloaded constructor
	inline PlayerProps(String playerName)
	{
		health=100;
		upvectors=true;
		inertia_active=true;
		kinematic_movement=false;
		mPlayerName=playerName;
	}
	
	String mPlayerName;
	Real health;
	bool upvectors;
	bool inertia_active;
	bool kinematic_movement;
	bool splitvectors;
	bool dfoz;
	bool rotate_hor;
	bool allowflashlight;
	bool allowsubtitles;
	Real hor_sens;
	Real vert_sens;
	bool resetVelocityOnRelease;
	Real elastic;
	Real softeness;
	Real mouseSens;
	Real runVel;
	Real stdVel;
	Real fov;
	Real stairEffect;
	Real flashConeI,flashConeU,range;
	Real trainForce;
	Real nearClip;
	Real farClip;
	int contcoll;
	
};
#endif