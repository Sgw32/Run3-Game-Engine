/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2014 Fyodor Zagumennov		   //////////
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

using namespace Ogre;
using namespace std;

#ifndef __NPC_FRIEND_RUN3_H__
#define __NPC_FRIEND_RUN3_H__


#define __NPC_FRIEND_VELOCITY__ 5
/*namespace Ogre
{
class Node
{
public:
	

};
}*/

class npc_friend
{
public:
	npc_friend();
	~npc_friend();
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

	// feedback from NPCManager
	bool frameStarted(const Ogre::FrameEvent &evt);
private:
	PRIVATE_NODELIST
	AnimationState* mAnimState;
	std::vector<NPCNode*> mPath;
	Real mTime,mRealTime,mVel;
	NPCNode* pPos;
	Vector3 direction,pPosit,mSpawn,mStartPos;
	SceneNode* niNode;
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
	OgreNewt::Body* npcBody;
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
