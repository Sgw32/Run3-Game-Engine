/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once

#ifndef __NPCSPAWNPROPS_H__
#define __NPCSPAWNPROPS_H__

class NPCSpawnProps
{
public:
	// standard constructor
	inline NPCSpawnProps()
	{
		cfg="NONE_RUN3";
		spCName="unknown";
		health=100;
		physPosit=Vector3::ZERO;
		animated=true;
		stupidsounds=true;
		exploding=false;
		strange_look=false;
		facial_animation=false;
		applyGravity=true;
		landed=true;
		takeOffBypass=1.0f;
		renderDist=10000.0f;
		transitSpeed=1;
		cNearScript="";
		ragdoll=false;
	}
	// overloaded constructor
	inline NPCSpawnProps(String className,Vector3 pos)
	{
		cfg="NONE_RUN3";
		spCName=className;
		spCPos=pos;
	}
	inline NPCSpawnProps(String conf)
	{
		cfg=conf;
	}
	// free fields
	String mName;
	String spCName;
	String script;
	String cfg;
	String mesh;
	Vector3 scale;
	Vector3 spCPos;
	Quaternion rot;
	Real health;
	Real farFind;
	Real velocity;
	bool stopAtDist;
	bool animated;
	bool stupidsounds;
	bool headshot;
	bool exploding;
	bool strange_look;
	bool sounds;
	bool facial_animation;
	bool ragdoll;
	Real takeOffBypass;
	Real headshotDist;
	Real rotateSpeed;
	String headMesh;
	String headBone;
	String handBone;
	String materialName;
	Real stopDist;
	Real yShift;
	Real renderDist;
	int relation;
	Vector3 physPosit;
	Vector3 physSize;
	Vector3 ax;
	Vector3 headAxis;
	Real angle;
	Real attackAnimDist;
	String luaOnDeath;
	String luaOnReach;
	bool applyGravity;
	Real ascendFromGround;
	bool landed;
	Real transitSpeed;
	String cNearScript;
	//SOUNDS
	String soundAttack;
	String soundHit;
	String soundFind;
	String soundRandom1;
	String soundSubRandom21;
	String soundSubRandom22;
	String soundSubRandom23;
	String soundFootstepPrefix;

};
#endif