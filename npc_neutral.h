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

class npc_neutral
{
public:
	npc_neutral();
	~npc_neutral();
	// init NPC
	void init(NPCSpawnProps props);
	// set Node List
	void setNodeList(NodeList* nList){STORE_NODELIST}
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
	Vector3 getDirection()
	{
		return getorient(npcBody)*Vector3::NEGATIVE_UNIT_Z;
	}
	Vector3 getNPCDirection()
	{
		return niNode->getOrientation()*Vector3::NEGATIVE_UNIT_Z;
	}
	// returns position
	Ogre::Vector3 getpos(OgreNewt::Body* bod);
	
	// returns orientation
	Quaternion getorient(OgreNewt::Body* bod);
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
	// When NPC moves, rapid changing an animation causes it look strange. So, you need to do a transition between 2 animations.
	void transitFromCurrentAnimation(String newAnim);
	// Move npcBody
	void teleport(Vector3 dest);
	//set newt material
	void setMaterialGroupID(const OgreNewt::MaterialID* enemyMat)
	{
		if (npcBody)
		{
			npcBody->setMaterialGroupID(enemyMat);
		}
	}
	bool check_straight();
	bool check_straight(Vector3 n1,Vector3 n2);

	void setDirection(Node* t,const Vector3& vec, Node::TransformSpace relativeTo, 
        const Vector3& localDirectionVector)
    {
        // Do nothing if given a zero vector
        if (vec == Vector3::ZERO) return;

        // The direction we want the local direction point to
        Vector3 targetDir = vec.normalisedCopy();

        // Transform target direction to world space
        switch (relativeTo)
        {
		case Node::TS_PARENT:
                if (t->getParent())
                {
                    targetDir = t->getParent()->_getDerivedOrientation() * targetDir;
                }
            break;
        case Node::TS_LOCAL:
            targetDir = t->_getDerivedOrientation() * targetDir;
            break;
        case Node::TS_WORLD:
            // default orientation
            break;
        }

        // Calculate target orientation relative to world space
        Quaternion targetOrientation;
      
            const Quaternion& currentOrient = t->_getDerivedOrientation();

            // Get current local direction relative to world space
            Vector3 currentDir = currentOrient * localDirectionVector;

            if ((currentDir+targetDir).squaredLength() < 0.00005f)
            {
                // Oops, a 180 degree turn (infinite possible rotation axes)
                // Default to yaw i.e. use current UP
                targetOrientation =
                    Quaternion(-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x);
            }
            else
            {
                // Derive shortest arc to new direction
                Quaternion rotQuat = currentDir.getRotationTo(targetDir);
                targetOrientation = rotQuat * currentOrient;
            }

        // Set target orientation, transformed to parent space
        if (t->getParent())
           t->setOrientation(t->getParent()->_getDerivedOrientation().UnitInverse() * targetOrientation);
        else
           t->setOrientation(targetOrientation);
    }

void lookAt( Node* t,const Vector3& targetPoint, Node::TransformSpace relativeTo, 
        const Vector3& localDirectionVector)
    {
        // Calculate ourself origin relative to the given transform space
        Vector3 origin;
        switch (relativeTo)
        {
        default:    // Just in case
		case Node::TS_WORLD:
            origin = t->_getDerivedPosition();
            break;
        case Node::TS_PARENT:
            origin = t->getPosition();
            break;
        case Node::TS_LOCAL:
            origin = Vector3::ZERO;
            break;
        }

        setDirection(t,targetPoint - origin, relativeTo, localDirectionVector);
    }
	
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

	/*int getCycle8()
	{
		return (int)(((int)time256)%16>8);
	}*/

	float getCycle8()
	{
		return (1+sin(time256/16*3.14f))/2;//(int)(((int)time256)%8>4);
	}

	// feedback from NPCManager
	bool frameStarted(const Ogre::FrameEvent &evt);
	void setParentRelation(SceneNode* pPar);
	void setParentRelation(String node);
	void stopParentRelation(){parentRelation=false;mPar=0;}
	
private:
	//Private NPC events
	vector<Ogre::Node*> animatedBones;
	inline void processNoticeStep();
	inline void processAttachedObjects();
	inline void processAnimationTransition(Real time);
	inline void processLook(Real time);
	inline void processRandomMovements(Real time);
	PRIVATE_NODELIST
	AnimationState* mAnimState;
	AnimationState* mTransitAnimState;
	Real transitState;
	Real time256;
	Light* flashLight;
	//Real transitSpeed;
	std::vector<NPCNode*> mPath;
	Real mTime,mRealTime,mVel;
	NPCNode* pPos;
	Vector3 direction,pPosit,mSpawn,mStartPos;
	SceneNode* mPar;
	SceneNode* niNode;
	Real fstTimer;
	Real rotateSpeed;
	unsigned int fstindex;
	Real uniqueMult;
	int i,iter;
	bool ttcomplete;
	bool notice;
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
	bool parentRelation;
	Vector3 pRelPosition;
	Quaternion pRelQuat;
	Quaternion pRelQuat2;
	Vector3 destCMD;
	Real goalHeadYaw;
	SkeletonInstance* skel;
	//Ogre::SceneNode*		mDebugPathNode;
	//Ogre::ManualObject*		mDebugLines;
	vector<Ogre::SceneNode*>	mDebugPathNodes;
	vector<PhysObject*>	attachedPhysObjects;
	Real yShift;
	Real gravity;
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
	OgreNewt::Body* npcBody;
	String npc_mesh;
	String luaToExec;
	String luaOnUse;
	Vector3 npc_scale;
	Vector3 physPos;
	Vector3 physSize;
	Real farFind;
	Ogre::SceneManager* mSceneMgr;
	OgreNewt::World* mWorld;
	NPCSpawnProps mProps;
	FacialAnimation* mFac;
};

#endif
