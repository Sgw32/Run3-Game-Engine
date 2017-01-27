#include "npc_enemy.h"


npc_enemy::npc_enemy()
{
blizko=false;
stopAtDist=false;
health=100;
dyeNow=false;
notsetIdle=false;
//mDebugLines=0;
}

npc_enemy::~npc_enemy()
{
	if (animated)
	{
mAnimState->setEnabled(false);
	}
delete npcBody;
heliEnt->detatchFromParent();
mSceneMgr->destroyEntity(heliEnt);
mSceneMgr->destroySceneNode(mHeliNode);
int s = mDebugPathNodes.size();
	for (unsigned int i=0;i!=s;i++)
	{
		mSceneMgr->destroyEntity(mDebugPathNodes[i]->getName());
		mSceneMgr->destroySceneNode(mDebugPathNodes[i]);
	}
	mDebugPathNodes.clear();
//mSceneMgr->destroySceneNode(mDebugPathNode);
//mSceneMgr->destroyManualObject(mDebugLines);
/*while (mPath.size() > 0)
{
		RagBone* bone = mBones.back();

		delete bone;

		mBones.pop_back();
	}*/
mPath.clear();
}

void npc_enemy::init(NPCSpawnProps props)
{
			mRoot=global::getSingleton().getRoot();
			going=false;
			mSceneMgr=mRoot->getSceneManagerIterator().getNext();
			mName=props.mName;
			ttcomplete=false;
			i=1;
			iter=0;
			mTime=40;
			mRealTime=0;
			npc_mesh=props.mesh;
			npc_scale=props.scale;
			mStartPos=props.spCPos;
			//mVel=__NPC_ENEMY_VELOCITY__*100;
			mVel=props.velocity;
			stopAtDist=props.stopAtDist;
			stopDist=props.stopDist;
			farFind=props.farFind;
			mWorld=global::getSingleton().getWorld();
			steps=0;
			straight=false;
			pPosit=global::getSingleton().getPlayer()->get_location();
			yShift=props.yShift;
			animated=props.animated;
			physPos=props.physPosit;
			physSize=props.physSize;
			//mDebugPathNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("npc_enemy_path_"+props.mName);
			//mDebugLines = new Ogre::ManualObject("npc_enemy_path_"+props.mName);
			//mRoot->addFrameListener(this);
 }

void npc_enemy::setPlayerPosition(Vector3 pos)
{
pPosit=pos;
}

void npc_enemy::start()
{
going=true;
}

void npc_enemy::suspend()
{
going=false;
}

 void npc_enemy::resume()
{

}


void npc_enemy::npc_force_callback( OgreNewt::Body* me)
{
 //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Vector3 stdAdd = Vector3::ZERO;
   health-=me->getDamage();
   if(health<0)
	   dyeNow=true;
   me->getMassMatrix(mass, inertia); 
   if (me->getStdAddForce()!=Ogre::Vector3::ZERO)
	stdAdd = me->getStdAddForce()/me->getStdAddForce().length();
   //Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,0,steps.z*1000+stdAdd.z*5*mVel); 
   Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,0,steps.z*mVel+stdAdd.z*5*mVel);
   Ogre::Vector3 force(0,-98,0); 
   force *= mass; 
   force = force;
   me->addForce( force );
   me->setStandartAddForce(Ogre::Vector3::ZERO);
  // me->addForce(  ); 
  // if (isOnEarth() && going)
  // {
	    if (!stopAtDist)
		{
			me->setVelocity( force2 );
		}
		else
		{
			if (!blizko)
				me->setVelocity( force2 );
		}
   //}
   if (!going && !dyeNow)
   {
			if (animated)
				mAnimState->setEnabled(false);
	   if (!notsetIdle)
	   {
		   if (animated)
		   {
	   mAnimState = heliEnt->getAnimationState("Idle1");
	   mAnimState->setEnabled(true);
		   }
	   }
	   me->setVelocity(Vector3::ZERO);
   }
   pPosit=global::getSingleton().getPlayer()->get_location();
}


bool npc_enemy::isOnEarth()
{
        
   Ogre::Vector3 myPos = getpos(npcBody);

   
   OgreNewt::BasicRaycast floorRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Y*100);
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
      distToFloor = hit.mDistance * 10; // calculale the distance to the floor
	  AxisAlignedBox aab = heliEnt->getBoundingBox();
			
		Vector3	size  = aab.getSize()*Vector3(0.25f,0.25f,0.25f)*npc_scale;
      distToFloor -= size.y;// remove char's height; 
      
        
      if(distToFloor > 0.05) //this much over the floor is considered on the floor
      {
         return false;
      }
      else
      {
         return true;
      }
   } 
   else
   {
      return false;
   }
   
}


void npc_enemy::processEvent(int flag,String param1,String param2)
{
	if (flag==SPAWN_NPC)
	{
		//Vector3 pos = heliEnt->
		mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);
		heliEnt = mSceneMgr->createEntity(mName,npc_mesh);
		mHeliNode->attachObject(heliEnt);
		mHeliNode->setScale(npc_scale);
		SceneNode* node2 = mHeliNode->createChildSceneNode(physPos);
		Entity* coltest = mSceneMgr->createEntity(node2->getName(),"box.mesh");
				AxisAlignedBox aab = heliEnt->getBoundingBox();
			Vector3 min = aab.getMinimum()*npc_scale;
	Vector3 max = aab.getMaximum()*npc_scale;
	Vector3 center = aab.getCenter()*npc_scale;
	Vector3 size  = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
		node2->setScale(size);
		coltest->setMaterialName("GLASS_ER2Rt");
		node2->attachObject(coltest);

		//Vector3	size  = aab.getSize()*npc_scale;
		OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size);
		npcBody = new OgreNewt::Body(mWorld,col,NEWTBODYTYPE_NPC);
		npcBody->setName(mHeliNode->getName());
		
		
		npcBody->attachToNode(mHeliNode);
		npcBody->setPositionOrientation(  mHeliNode->getPosition()+physPos,  mHeliNode->getOrientation() );
		npcBody->setType(NEWTBODYTYPE_NPC);
		delete col;
		Vector3 inertia = OgreNewt::MomentOfInertia::CalcEllipsoidSolid( 120,size );
		npcBody->setMassMatrix( 120, inertia );
		npcBody->setCustomForceAndTorqueCallback<npc_enemy>( &npc_enemy::npc_force_callback ,this);
		npcBody->setAutoFreeze(0);
		OgreNewt::BasicJoints::UpVector* uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Y);
		OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Z);
		OgreNewt::BasicJoints::UpVector* uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_X);
		//mAnimState = heliEnt->getAnimationState("Walk");
		if (animated)
		{
		mAnimState = heliEnt->getAnimationState("Idle1");
		mAnimState->setEnabled(true);
		}
	}
	if (flag==RUNTO_NPC)
	{
		start();
	}
	if (flag==STOP_NPC)
	{
		suspend();
	}
	if (flag==KILL_NPC)
	{
		kill();
	}
}

Vector3 npc_enemy::getpos(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
}


void npc_enemy::step(const Ogre::FrameEvent &evt)
{
	if (animated)
	{
	mAnimState->addTime(evt.timeSinceLastFrame);
	}


	if (going)
	{
		if(dyeNow)
			kill();
		bool farFindB = getpos(npcBody).distance(pPosit) < farFind;
		bool path_find;
		if (stopAtDist)
		blizko = getpos(npcBody).distance(pPosit) < stopDist;
		if (farFindB)
		{
			path_find = find_path();
		}
		else
		{
			steps=Ogre::Vector3::ZERO;
		}
		if (farFindB && path_find)
		{
			Vector3 dest;
			if (animated)
			{
			mAnimState->setEnabled(false);
			}
			if (!notsetIdle)
			{
						if (animated)
						{
							mAnimState = heliEnt->getAnimationState("Walk");
							mAnimState->setEnabled(true);
						}
			}
			if (mPath.size()>2)
			dest = mPath[i]->getPosition();
			if (!(mHeliNode->getPosition().distance(mPath[i-1]->getPosition())>dest.distance(mPath[i-1]->getPosition())))
			{
			Vector3 myPos = mHeliNode->getPosition();
			Real dist = myPos.distance(dest); 
			Vector3 dir = dest-myPos;
			if (evt.timeSinceLastFrame!=0)
			{
				steps = dir/dir.length();
				mHeliNode->lookAt(Vector3(pPosit.x,mHeliNode->getPosition().y,pPosit.z),Node::TS_WORLD);
				npcBody->setPositionOrientation(getpos(npcBody),mHeliNode->getOrientation());
			}
			}
			if (mHeliNode->getPosition().distance(Vector3(mPath[i-1]->getPosition().x,mHeliNode->getPosition().y,mPath[i-1]->getPosition().z))>dest.distance(Vector3(mPath[i-1]->getPosition().x,mHeliNode->getPosition().y,mPath[i-1]->getPosition().z)))
			{
				int b;
				b = i+1;
			if (!(b>mPath.size()))
      				i++;
			if ((i)==mPath.size())
				going=false;
			}
		}
		else
		{
			if (!dyeNow)
			{
				if (!farFindB)
				{
					if (animated)
				mAnimState->setEnabled(false);
					if (!notsetIdle)
					{
							if (animated)
							{
		    						mAnimState = heliEnt->getAnimationState("Idle1");
									mAnimState->setEnabled(true);
							}
					}
				}
				Vector3 dest = pPosit;
				Vector3 myPos = mHeliNode->getPosition();
				Vector3 dir = dest-myPos;
				if (farFindB)
				{
							if (animated)
							{
								mAnimState->setEnabled(false);
								mAnimState = heliEnt->getAnimationState("Walk");
								mAnimState->setEnabled(true);
							}
							if (evt.timeSinceLastFrame!=0)
							{
								steps = dir/dir.length();
								//mHeliNode->lookAt(Vector3(dir.x,mHeliNode->getPosition().y,dir.z),Node::TS_WORLD);
								mHeliNode->lookAt(Vector3(pPosit.x,mHeliNode->getPosition().y,pPosit.z),Node::TS_WORLD);
								npcBody->setPositionOrientation(getpos(npcBody),mHeliNode->getOrientation());
							}
				}
			}
		}

	}
}

bool npc_enemy::find_path()
{
	bool straight = check_straight();
	/*mDebugPathNode->detachAllObjects();
	if (mDebugLines)
		mDebugLines->clear(); */
	int s = mDebugPathNodes.size();
	for (unsigned int i=0;i!=s;i++)
	{
		mSceneMgr->destroyEntity(mDebugPathNodes[i]->getName());
		mSceneMgr->destroySceneNode(mDebugPathNodes[i]);
	}
	mDebugPathNodes.clear();
	if (straight)
		return false;
	mPath.clear();
	vector<NPCNode*> nodes = mNodeList->getNode();
	NPCNode* pos = nodes[0];
	NPCNode* end = nodes[0];
	Vector3 myPos = getpos(npcBody);
	s = nodes.size();
	for (unsigned int i=0;i!=s;i++)
	{
		Real r1 = myPos.distance(nodes[i]->getPosition());
		Real r2 = myPos.distance(pos->getPosition());
		Real r3 = pPosit.distance(nodes[i]->getPosition());
		Real r4 = pPosit.distance(end->getPosition());
		if (r1<r2)
			pos=nodes[i];
		if (r3 <r4 )
			end=nodes[i];
	}
	
	mPath = AIManager::getSingleton().getPathFinder()->search(DEPTH_FIRST_SEARCH,pos,end);
	for (unsigned int i=0;i!=mPath.size();i++)
	{
		SceneNode* n = mSceneMgr->getRootSceneNode()->createChildSceneNode(mPath[i]->getPosition());
		Entity* m = mSceneMgr->createEntity(n->getName(),"box.mesh");
		n->setScale(100,100,100);
		n->attachObject(m);
		mDebugPathNodes.push_back(n);
	}
	/*mDebugLines->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);
	for (unsigned int i=0;i!=mPath.size()-1;i++)
	{
		mDebugLines->position(mPath[i]->getPosition());
		mDebugLines->position(mPath[i+1]->getPosition());
	}
	mDebugLines->end();
	mDebugPathNode->attachObject(mDebugLines);*/

	return true;
}

bool npc_enemy::check_straight()
{
	Vector3 myPos=getpos(npcBody);
	OgreNewt::BasicRaycast ray(mWorld,myPos,pPosit);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t)
	{
		if (bod_t->getName()=="PLAYER1" && bod_t->getType()==1 )
		{
			return true;
		}
		return false;
	}
	return false;
}
void npc_enemy::kill()
{
	going=false;
	dyeNow=true;
	notsetIdle=true;
			if (animated)
		{
	mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState("Death1");
			mAnimState->setLoop(false);
			mAnimState->setEnabled(true);
		}
}
bool npc_enemy::frameStarted(const Ogre::FrameEvent &evt)
{
	step(evt);
			if (animated)
		{
	if (!mAnimState->getLoop())
	{
		if (mAnimState->getTimePosition()>mAnimState->getLength())
		{
			mAnimState->setEnabled(false);
		}
	}
		}
	return true;
}