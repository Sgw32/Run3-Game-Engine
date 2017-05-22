#include "npc_template.h"

Vector3 npc_template::getpos(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
}

Quaternion npc_template::getorient(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return rot;
}

bool npc_template::find_path()
{
	//LogManager::getSingleton().logMessage("pathfind");
	Vector3 myPos = getpos(npcBody);
	bool straight = check_straight();
	
	int s;
	
	if (straight)
		return false;
	mPath.clear();
	vector<NPCNode*> nodes = mNodeList->getNode();
	//LogManager::getSingleton().logMessage("Nodes size:"+StringConverter::toString(nodes.size()));
	NPCNode* pos = nodes[0];
	NPCNode* end = nodes[0];
	s = nodes.size();
	
	//LogManager::getSingleton().logMessage("finding closest nodes..."+StringConverter::toString(s));
	for (unsigned int i=0;i!=s;i++)
	{
		if(check_straight(myPos,nodes[i]->getPosition()))
		{
		Real r1 = myPos.distance(nodes[i]->getPosition());
		Real r2 = myPos.distance(pos->getPosition());
		if (r1<r2)
		{
#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("Found closest to NPC pos");
			
#endif
			pos=nodes[i];
		}
		}

		if(check_straight(pPosit,nodes[i]->getPosition()))
		{
		Real r3 = pPosit.distance(nodes[i]->getPosition());
		Real r4 = pPosit.distance(end->getPosition());
		if (r3 <r4 )
		{
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("Found closest to end");
			
			#endif
			end=nodes[i];
		}
		}
	}
	//LogManager::getSingleton().logMessage(StringConverter::toString("found"));
	//LogManager::getSingleton().logMessage("to neutral"+StringConverter::toString(pos->getPosition()));
	//LogManager::getSingleton().logMessage("to player"+StringConverter::toString(end->getPosition()));
	//LogManager::getSingleton().logMessage("Starting pathfinding...");
	mPath = AIManager::getSingleton().getPathFinder()->search(DEPTH_FIRST_SEARCH,pos,end);
#ifdef _DEBUG
	vector<NPCNode*>::iterator i2;
	for (i2=mPath.begin();i2!=mPath.end();i2++)
	//LogManager::getSingleton().logMessage(StringConverter::toString((*i2)->getPosition()));
#endif
	

	return true;
}

bool npc_template::find_path(Vector3 destPos)
{
	//LogManager::getSingleton().logMessage("pathfind");
	Vector3 myPos = getpos(npcBody);
	bool straight = check_straight(myPos,destPos)&&(!cmdMode); // if cmdmode - bypass this
	
	int s;

	if (straight)
		return false;
	mPath.clear();
	vector<NPCNode*> nodes = mNodeList->getNode();
	//LogManager::getSingleton().logMessage("Nodes size:"+StringConverter::toString(nodes.size()));
	NPCNode* pos = nodes[0];
	NPCNode* end = nodes[0];
	s = nodes.size();
	
	//LogManager::getSingleton().logMessage("finding closest nodes..."+StringConverter::toString(s));
	for (unsigned int i=0;i!=s;i++)
	{
		if(check_straight(myPos,nodes[i]->getPosition()))
		{
		Real r1 = myPos.distance(nodes[i]->getPosition());
		Real r2 = myPos.distance(pos->getPosition());
		if (r1<r2)
		{
			LogManager::getSingleton().logMessage("Found closest to NPC pos");
			pos=nodes[i];
		}
		}

		if(check_straight(destPos,nodes[i]->getPosition()))
		{
		Real r3 = destPos.distance(nodes[i]->getPosition());
		Real r4 = destPos.distance(end->getPosition());
		if (r3 <r4 )
		{
			LogManager::getSingleton().logMessage("Found closest to end");
			end=nodes[i];
		}
		}
	}
	//LogManager::getSingleton().logMessage(StringConverter::toString("found"));
	//LogManager::getSingleton().logMessage("to neutral"+StringConverter::toString(pos->getPosition()));
	//LogManager::getSingleton().logMessage("to player"+StringConverter::toString(end->getPosition()));
	//LogManager::getSingleton().logMessage("Starting pathfinding...");
	mPath = AIManager::getSingleton().getPathFinder()->search(DEPTH_FIRST_SEARCH,pos,end);
#ifdef _DEBUG
	vector<NPCNode*>::iterator i2;
	for (i2=mPath.begin();i2!=mPath.end();i2++)
	//LogManager::getSingleton().logMessage(StringConverter::toString((*i2)->getPosition()));
#endif
	

	return true;
}

bool npc_template::check_straight()
{
	/*Vector3 myPos=getpos(npcBody);
	
	OgreNewt::BasicRaycast ray2(mWorld,myPos,pPosit);
	
	
	OgreNewt::Body* bod_t2 = ray2.getFirstHit().mBody;
	
	if (bod_t2)
	{
#define RAYS_PLAYER(b) b->getName()=="PLAYER1" && b->getType()==1 
		if ( RAYS_PLAYER(bod_t2) )
		{
			return true;
		}
		return false;
	}
	return false;*/
	Vector3 myPos=getpos(npcBody);
	
	OgreNewt::BasicRaycast ray2(mWorld,myPos,pPosit);
	
	
	OgreNewt::Body* bod_t2 = ray2.getFirstHit().mBody;
	
	if (bod_t2)
	{
		return false;
	}
	ray2= OgreNewt::BasicRaycast(mWorld,myPos-Vector3(-30,0,0),pPosit);
	
	
	bod_t2 = ray2.getFirstHit().mBody;
	
	if (bod_t2)
	{
		return false;
	}
	ray2= OgreNewt::BasicRaycast(mWorld,myPos+Vector3(30,0,0),pPosit);
	
	
	bod_t2 = ray2.getFirstHit().mBody;
	
	if (bod_t2)
	{
		return false;
	}
	
	return true;
}


void npc_template::start()
{
	going=true;
}

void npc_template::suspend()
{
	going=false;
}

 void npc_template::resume()
{

}

//////////////////////////////////////////
void npc_template::teleport(Vector3 pos)
{
	npcBody->setPositionOrientation(pos,getorient(npcBody));
	//mCamNode->setPosition(pos);
	//mCamera->setPosition(Vector3(mCamera->getPosition().x,get_body_position(bod).y,mCamera->getPosition().z));
}

void npc_template::processNoticeStep()
{
	if (notice)
	{
		Vector3 dist = global::getSingleton().getPlayer()->get_location()-getpos(npcBody);
		if (dist.length()<200.0f)
		{
			if (!mProps.cNearScript.empty())
			{
				RunLuaScript(global::getSingleton().getLuaState(), mProps.cNearScript.c_str());
			}
			notice=false;
		}
	}
}

void npc_template::setDirection(Node* t,const Vector3& vec, Node::TransformSpace relativeTo, 
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

void npc_template::lookAt( Node* t,const Vector3& targetPoint, Node::TransformSpace relativeTo, 
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

void npc_template::processBaseEvent(int flag,String param1,String param2)
{
	if (flag==SPAWN_NPC)
	{
		spawnNPC();
	}
	if (flag==RUNTO_NPC)
	{
		start();
	}
	if (flag==STOP_NPC)
	{
		suspend();
	}

	if (flag==SETANIM_NPC)
	{
		LogManager::getSingleton().logMessage("Proc event...");
		if (animated)
		{
			LogManager::getSingleton().logMessage("Setting anim..."+heliEnt->getName());
			mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState(param1);
			mAnimState->setEnabled(true);
			suspend();
			LogManager::getSingleton().logMessage("Animation test."+mAnimState->getAnimationName()+"fsd"+StringConverter::toString(mAnimState->getEnabled()));
		}
	}

	if (flag==ALERT_NPC)
	{
		LogManager::getSingleton().logMessage("NPC is CRAZY");
		moveActivity=2;
	}
	if (flag==FEAR_NPC)
	{
		LogManager::getSingleton().logMessage("NPC is trying to escape from this horrible situation");
		moveActivity=0.3f;
	}
	if (flag==CRAZY_NPC)
	{
		moveActivity=0.3f;
	}

	if (flag==FACIAL_ACTIVITY)
	{
		if (mFac)
		{
			mFac->init(param1);
			mFac->animate(getpos(npcBody));
		}
	}

	if (flag==TELEPORT_NPC)
	{
		teleport(StringConverter::parseVector3(param1));
	}
}

void npc_template::initFacialSystem()
{
			if (mProps.facial_animation)
			{
				mFac=new FacialAnimation();
				FacialAnimationManager::getSingleton().passFacial(mFac);
			}
			mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);
			
			//Facial animation
			if (mProps.facial_animation)
				heliEnt=mFac->preInit(npc_mesh,mName);
			else
				heliEnt=mSceneMgr->createEntity(mName,npc_mesh);
}

bool npc_template::isOnEarth()
{
	Real distToFloor;
   Ogre::Vector3 myPos = getpos(npcBody);
	AxisAlignedBox aab = heliEnt->getBoundingBox();
			
		Vector3	size  = aab.getSize()*npc_scale;
   
   OgreNewt::BasicRaycast floorRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Y*(size.y+50));
	int hits;
	hits=floorRay.getHitCount();
    if (hits > 0)
   {           
        OgreNewt::BasicRaycast::BasicRaycastInfo hit;
        hit.mBody = NULL;
      hit.mDistance = 500;
        for(int i=0;i<floorRay.getHitCount();i++)
        {
            OgreNewt::BasicRaycast::BasicRaycastInfo found = floorRay.getInfoAt(i);
         if(found.mBody != npcBody && found.mDistance < hit.mDistance)
            {
                //if the body I found is not my own, and is not water and is closer than last result
                //you will probably have a different water system than me...
                hit = found;
                //break;
            }
        }
        if(!hit.mBody)
            return false;
      distToFloor = hit.mDistance * (size.y+50); // calculale the distance to the floor
	 
      distToFloor -= size.y/2;// remove char's height; 
      
        
      if(distToFloor > 0.05) //this much over the floor is considered on the floor
      {
		  ////LogManager::getSingleton().logMessage(StringConverter::toString(distToFloor));
         return false;
      }
      else
      {
         return true;
      }
   } 
   else
   {
	  // //LogManager::getSingleton().logMessage("Not on floor!");
      return false;
   }
   
}

bool npc_template::check_straight(Vector3 n1,Vector3 n2)
{
	OgreNewt::BasicRaycast ray(mWorld,n1,n2);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t) 
		return false; 
	else 
		return true;
}
