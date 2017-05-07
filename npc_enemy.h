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
	// check can it move
	bool isOnEarth();
	// return it's name
	String getName(){return mName;};
	// starts movement and activity
	void start();
	// Unused
	void setPlayerPosition(Vector3 pos);
	// Unused
	void suspend();
	// Unused
	void resume();

	Vector3 getNPCDirection()
	{
		return niNode->getOrientation()*Vector3::NEGATIVE_UNIT_Z;
	}
	// movement process here
	void step(const Ogre::FrameEvent &evt);
	// event callback
	void processEvent(int flag,String param1,String param2);
	// OgreNewt callback
	void npc_force_callback( OgreNewt::Body* me);
	// used in npc_physfollow
	void addPoint(NPCNode* a);
	//find path
	bool find_path(); // return true if it isn't a straight path between player and npc, and false if it is.
	//find path
	bool find_path(Vector3 destPos); // return true if it isn't a straight path between player and npc, and false if it is.
	// is NPC is killed
	void kill();
	//HEADSHOT!!!
	void headshot();
	
	bool check_straight();
	bool check_straight(Vector3 n1,Vector3 n2);

	// feedback from NPCManager
	bool frameStarted(const Ogre::FrameEvent &evt);
private:
	AnimationState* mAnimState;
	std::vector<NPCNode*> mPath;
	Real mTime,mRealTime,mVel;
	NPCNode* pPos;
	Vector3 direction,pPosit,mSpawn,mStartPos;
	Real fstTimer;
	unsigned int fstindex;
	int i,iter;
	bool ttcomplete;
	bool going;
	bool straight;
	bool blizko;
	bool stopAtDist;
	bool dyeNow;
	bool notsetIdle;
	bool animated;
	bool goal_achieved;
	bool rotation_needed;
	bool previous_player;
	bool cmdMode;
	Vector3 destCMD;
	SkeletonInstance* skel;
	//Ogre::SceneNode*		mDebugPathNode;
	//Ogre::ManualObject*		mDebugLines;
	vector<Ogre::SceneNode*>	mDebugPathNodes;
	Real yShift;
	Real stopDist;
	Real health;
	Real body_rot_y;
	bool reach_zero;
	bool rot_dir;
	bool sounds;
	Real moveActivity;
	Real rangle;
	Vector3 axis;
	Real attackAnimDist;
	Real lastAngle;
	String mName;
	Real distToFloor;
	Entity* heliEnt;
	Vector3 steps;
	Ogre::Root* mRoot;
	Ogre::SceneNode* mHeliNode;
	String npc_mesh;
	Vector3 npc_scale;
	Vector3 physPos;
	Vector3 physSize;
	Real farFind;
	Ogre::SceneManager* mSceneMgr;
	OgreNewt::World* mWorld;
	NPCSpawnProps mProps;
};

#endif
