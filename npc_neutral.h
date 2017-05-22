/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2014 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
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
#include "FacialAnimation.h"

#include "npc_template.h"

using namespace Ogre;
using namespace std;

#ifndef __NPC_NEUTRAL_RUN3_H__
#define __NPC_NEUTRAL_RUN3_H__


#define __NPC_NEUTRAL_VELOCITY__ 5
/*namespace Ogre
{
class Node
{
public:
	

};
}*/

// DEBUG OPTIONS

//#define DEBUG_TARGET_BLIZKO
//#define DEBUG_PATHFINDING
//#define DEBUG
//#define DEBUG_FACIAL
#define STRAIGHT_BYPASS

class npc_neutral : public npc_template
{
public:
	npc_neutral();
	~npc_neutral();
	// init NPC
	void init(NPCSpawnProps props);
	// Unused
	void setPlayerPosition(Vector3 pos);
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
	// When NPC moves, rapid changing an animation causes it look strange. So, you need to do a transition between 2 animations.
	void transitFromCurrentAnimation(String newAnim);
	//set newt material
	
	int getCycle128()
	{
		return (int)((int)time256>128);
	}

	int getCycle64()
	{
		return (int)(((int)time256)%128>64);
	}

	int getCycle32()
	{
		return (int)(((int)time256)%64>32);
	}

	int getCycle16()
	{
		return (int)(((int)time256)%32>16);
	}

	float getCycle8()
	{
		return (1+sin(time256/16*3.14f))/2;//(int)(((int)time256)%8>4);
	}
	
	inline void flush();
	// feedback from NPCManager
	bool frameStarted(const Ogre::FrameEvent &evt);
	void setParentRelation(SceneNode* pPar);
	void setParentRelation(String node);
	void stopParentRelation(){parentRelation=false;mPar=0;}
	void spawnNPC();
private:
	//Private NPC events
	vector<Ogre::Node*> animatedBones;
	inline void processHealthLoss(OgreNewt::Body* me);
	inline void processAttachedObjects();
	inline void processAnimationTransition(Real time);
	inline void processLook(Real time);
	inline void processRandomMovements(Real time);
	inline void spawnNPCRagdoll(Vector3 pos);
	//PRIVATE_NODELIST
	Real transitState;
	Real time256;
	Light* flashLight;
	//std::vector<NPCNode*> mPath;
	Real mTime,mRealTime,mVel;
	SceneNode* mPar;
	Real rotateSpeed;
	Real uniqueMult;
	int i,iter;
	bool parentRelation;
	bool dead;
	Vector3 pRelPosition;
	Quaternion pRelQuat;
	Quaternion pRelQuat2;
	Vector3 destCMD;
	Real goalHeadYaw;
	vector<PhysObject*>	attachedPhysObjects;
	Real yShift;
	Real gravity;
	Real stopDist;
	Real health;
	Real body_rot_y;
	Real rangle;
	Vector3 axis;
	Real lastAngle;
	Vector3 steps;
	String luaToExec;
	String luaOnUse;
	Vector3 physPos;
	Vector3 physSize;
	Real farFind;
};

#endif
