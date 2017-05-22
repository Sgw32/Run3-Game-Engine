/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Run3 rules!!! It's a best game engine!/////////////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include "global.h"
//AIR3 Headers
#include "AIR3.h"
#include "NPCSpawnProps.h"
#include "AIManager.h"
#include "BloodEmitter.h"
#include "Run3SoundRuntime.h"
#include "ExplosionManager.h"
#include "GibManager.h"
#include "Timeshift.h"

#include "npc_template.h"

using namespace Ogre;
using namespace std;

#ifndef __NPC_ENEMY_RUN3_H__
#define __NPC_ENEMY_RUN3_H__


#define __NPC_ENEMY_VELOCITY__ 5
/*namespace Ogre
{
class Node
{
public:
	

};
}*/

class npc_enemy : public npc_template
{
public:
	npc_enemy();
	~npc_enemy();
	// init NPC
	void init(NPCSpawnProps props);
	// starts movement and activity
	void setPlayerPosition(Vector3 pos);
	bool check_straight();
	bool check_straight(Vector3 n1,Vector3 n2);
	// movement process here
	void step(const Ogre::FrameEvent &evt);
	// event callback
	void processEvent(int flag,String param1,String param2);
	// OgreNewt callback
	void npc_force_callback( OgreNewt::Body* me);
	// used in npc_physfollow
	void addPoint(NPCNode* a);
	// is NPC is killed
	void kill();
	//HEADSHOT!!!
	void headshot();
	// feedback from NPCManager
	bool frameStarted(const Ogre::FrameEvent &evt);
	//
	void spawnNPC();
private:
	Real mTime,mRealTime,mVel;
	int i,iter;
	Vector3 destCMD;
	Real yShift;
	Real stopDist;
	Real health;
	Real body_rot_y;
	Real rangle;
	Vector3 axis;
	Real lastAngle;
	Vector3 steps;
	Vector3 physPos;
	Vector3 physSize;
	Real farFind;
};

#endif
