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
	void setNodeList(NodeList* nList){STORE_NODELIST}
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
protected:
	PRIVATE_NODELIST
	OgreNewt::Body* npcBody;
	SceneNode* niNode;
};

#endif
