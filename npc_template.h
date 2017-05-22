/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2017 Fyodor Zagumennov		   //////////
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
#include "FacialAnimationManager.h"

using namespace Ogre;
using namespace std;

#ifndef __NPC_TEMPLATE_RUN3_H__
#define __NPC_TEMPLATE_RUN3_H__


class npc_template
{
public:
	//npc_template();
	//virtual ~npc_template();
	virtual void init(NPCSpawnProps props){};
	virtual bool frameStarted(const Ogre::FrameEvent &evt) {return 0;}
	virtual void processEvent(int flag,String param1,String param2) {}

	void processBaseEvent(int flag,String param1,String param2);
	void setNodeList(NodeList* nList){STORE_NODELIST}

	// starts movement and activity
	void start();
	// Unused
	void suspend();
	// Unused
	void resume();
	// return it's name
	String getName(){return mName;};
	void setMaterialGroupID(const OgreNewt::MaterialID* enemyMat)
	{
		if (npcBody)
		{
			npcBody->setMaterialGroupID(enemyMat);
		}
	}
	
	// returns position
	Ogre::Vector3 getpos(OgreNewt::Body* bod);
	
	// returns orientation
	Quaternion getorient(OgreNewt::Body* bod);
	
	Vector3 getDirection()
	{
		return getorient(npcBody)*Vector3::NEGATIVE_UNIT_Z;
	}

	void setDirection(Node* t,const Vector3& vec, Node::TransformSpace relativeTo, 
        const Vector3& localDirectionVector);
    

	void lookAt( Node* t,const Vector3& targetPoint, Node::TransformSpace relativeTo, 
        const Vector3& localDirectionVector);

	Vector3 getNPCDirection()
	{
		return niNode->getOrientation()*Vector3::NEGATIVE_UNIT_Z;
	}
	bool check_straight();
	//find path
	bool find_path();
	bool find_path(Vector3 destPos);
	//
	bool check_straight(Vector3 n1,Vector3 n2);
	// Move npcBody
	void teleport(Vector3 pos);
	// check can it move
	bool isOnEarth();

	void processNoticeStep();
	//
	void initFacialSystem();
	//
	virtual void spawnNPC(){};
protected:
	PRIVATE_NODELIST
	String mName; //Name of NPC
	String npc_mesh;
	OgreNewt::Body* npcBody; //Newton Physics body
	Ogre::SceneNode* mHeliNode; //Master NPC Ogre Node
	SceneNode* niNode; //Slave NPC Ogre Node
	Entity* heliEnt; //Main NPC Entity
	SkeletonInstance* skel; //NPC Skeleton(for random animations)
	bool ttcomplete; 
	bool going; //Is activity(goes/rotates)
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
	bool notice;
	bool reach_zero;
	bool rot_dir;
	bool sounds;
	Vector3 direction,pPosit,mSpawn,mStartPos;
	unsigned char fstindex; //Footstep counter
	Real moveActivity, fstTimer; //How fast NPC operates, footstep timer.
	Real attackAnimDist;
	Vector3 npc_scale;
	Ogre::Root* mRoot; //Ogre Root
	Ogre::SceneManager* mSceneMgr; //Ogre SceneManager
	OgreNewt::World* mWorld; //Newton World
	FacialAnimation* mFac; //Facial animation (common for every class!)
	NPCNode* pPos; //Destination closest node
	std::vector<NPCNode*> mPath; //Path for NPC to follow
	NPCSpawnProps mProps; //Input properties, common for all NPCs.
	AnimationState* mAnimState; //Current animation
	AnimationState* mTransitAnimState; //Following animation
	//Debug
	vector<Ogre::SceneNode*>	mDebugPathNodes;
};

#endif
