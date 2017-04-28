#include "npc_aerial.h"
#include "AIManager.h"


npc_aerial::npc_aerial()
{
blizko=false;
stopAtDist=false;
health=20;
dyeNow=false;
notsetIdle=false;
rotation_needed=false;
body_rot_y=0;
lastAngle=0;
previous_player=false;
reach_zero=false;
rot_dir=false;
cmdMode=false;
fstTimer=0;
fstindex=1;
moveActivity=1.0f;
takingOff=false;
landing_aer=false;
behavior_type=0;
aiel=0;
//mDebugLines=0;
}

npc_aerial::~npc_aerial()
{
	if (animated)
	{
mAnimState->setEnabled(false);
	}
	if (npcBody)
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

void npc_aerial::init(NPCSpawnProps props)
{
	//LogManager::getSingleton().logMessage("Npc manager: npc enemy initializing");
	mProps=props;
			mRoot=global::getSingleton().getRoot();
			going=false;
			mSceneMgr=mRoot->getSceneManagerIterator().getNext();
			mName=props.mName;
			ttcomplete=false;
			goal_achieved=true;
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
			sounds=props.stupidsounds;
			mWorld=global::getSingleton().getWorld();
			steps=0;
			straight=false;
			pPosit=global::getSingleton().getPlayer()->get_location();
			yShift=props.yShift;
			animated=props.animated;
			physPos=props.physPosit;
			physSize=props.physSize;
			rangle=props.angle;
			axis=props.ax;
			attackAnimDist=props.attackAnimDist;
			//LogManager::getSingleton().logMessage("Npc manager: npc enemy ended initializing");
			//mDebugPathNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("npc_aerial_path_"+props.mName);
			//mDebugLines = new Ogre::ManualObject("npc_aerial_path_"+props.mName);
			//mRoot->addFrameListener(this);
			health=props.health;
 }

void npc_aerial::setPlayerPosition(Vector3 pos)
{
pPosit=pos;
}

void npc_aerial::start()
{
	if (mProps.landed==false)
	going=true;
}

void npc_aerial::suspend()
{
going=false;
}

 void npc_aerial::resume()
{

}

void npc_aerial::headshot()
{
	if (mProps.sounds)
	Run3SoundRuntime::getSingleton().emitSound("run3/sounds/bodysplat_1.wav",2,false,npcBody->damagePosition,200,50,800);
		mProps.headshot=false;
		LogManager::getSingleton().logMessage("HEADSHOT!!!");
		
		for (unsigned int i=0;i!=skel->getNumAnimations();i++)
		{
		Animation* anim = skel->getAnimation(i);
		MAKE_SOME_GORE(mProps.headBone)
		}
		LogManager::getSingleton().logMessage("Headshot processing finished.");
}


void npc_aerial::npc_force_callback( OgreNewt::Body* me)
{
 //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Vector3 stdAdd = Vector3::ZERO;
   bool onEarth = isOnEarth();
   Vector3 myPosN=getpos(me);
   Real health1=health;
   health-=me->getDamage();
	
   //P-reg
   Vector3 veln=Vector3::ZERO;
	if (takingOff)
	{
		veln = (takeOffPos-myPosN);
		if (veln.length()<20)
		{
			start();
			takingOff=false;
		}
		veln/=mProps.takeOffBypass;
	}
	if (landing_aer)
	{
		veln=Vector3(0,-100,0);
		if (onEarth)
		{
			landing_aer=false;
			veln=0;
		}
	}

	if (health1!=health)
	{
		LogManager::getSingleton().logMessage("NPC DAMAGE!!!");
		 if (((me->damagePosition-myPosN).y>mProps.headshotDist)&&(mProps.headshot))
   {
	   BloodEmitter::getSingleton().emitBlood(me->damagePosition,Vector3(0.7,0.7,0.7));
	   GibManager::getSingleton().spawnGib(mProps.headMesh,me->damagePosition,mHeliNode->getScale(),4.0f);
		headshot();
   }
		BloodEmitter::getSingleton().emitBlood(me->damagePosition,Vector3(0.3,0.3,0.3));
		if (mProps.exploding)
		{
			ExplosionManager::getSingleton().spawnExplosionNosound(me->damagePosition,Vector3(2,2,2));
			if (mProps.sounds)
Run3SoundRuntime::getSingleton().emitSound("run3/sounds/explode_3.wav",4,false,me->damagePosition,200,50,800);
			kill();
			mProps.exploding=false;
		}
	}
   if(health<0)
   {
	   dyeNow=true;
		return;
   }
	   me->getMassMatrix(mass, inertia); 
   if (me->getStdAddForce()!=Ogre::Vector3::ZERO)
   {
	stdAdd = me->getStdAddForce();
	stdAdd.normalise();
   }
   //Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,0,steps.z*1000+stdAdd.z*5*mVel);
  // //LogManager::getSingleton().logMessage(StringConverter::toString(steps));
   Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,steps.y*mVel,steps.z*mVel+stdAdd.z*5*mVel);
  //LogManager::getSingleton().logMessage(StringConverter::toString(steps));
   Ogre::Vector3 force;
   if (mProps.applyGravity)
  force=Ogre::Vector3(0,-98,0); 
   else
	 force=Vector3::ZERO;
	
   force *= mass; 
   //force = force;
   if (!onEarth)
   me->addForce( force*TIME_SHIFT*TIME_SHIFT*moveActivity );
   me->setStandartAddForce(Ogre::Vector3::ZERO);
   //me->setPositionOrientation(getpos(me),Quaternion::ID
  // me->addForce(  ); 
  // me->setVelocity(Vector3::ZERO);
   //if ((onEarth)||(!mProps.applyGravity))
   me->setVelocity( force2*TIME_SHIFT *moveActivity+veln*TIME_SHIFT);
   /*if (isOnEarth() && going)
   {
	   if (me->getUsing())
		   force2=-force2;
	    if (!stopAtDist)
		{

			me->setVelocity( force2 );
		}
		else
		{
			if (!blizko)
				me->setVelocity( force2 );
		}
   }*/
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
	   	if ((!takingOff)&&(!landing_aer)) //Aircraft landing and taking off exception
		{
	   me->setVelocity(Vector3::ZERO);
		}
   }
   pPosit=global::getSingleton().getPlayer()->get_location();
   if (!dyeNow)
   {
   me->setOmega(Vector3(0,body_rot_y*TIME_SHIFT,0));
   }
   else
   {
		me->setOmega(Vector3(0,0,0));
   }
}


bool npc_aerial::isOnEarth()
{
        /*if (mProps.applyGravity)
			return true;*/
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

void npc_aerial::takeoff()
{
	takeOffPos=getpos(npcBody)+Vector3(0,mProps.ascendFromGround,0);
	takingOff=true;
	landing_aer=false;
	going=false;
}

void npc_aerial::land()
{
	//takeOffPos=getpos(npcBody)+Vector3(0,mProps.ascendFromGround,0);
	takingOff=false;
	landing_aer=true;
	going=false;
}

void npc_aerial::processEvent(int flag,String param1,String param2)
{
	if (flag==SPAWN_NPC)
	{
		
			mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);

			if (mName=="")
			{

				mName=mHeliNode->getName();
			}
			
			heliEnt=mSceneMgr->createEntity(mName,npc_mesh);

			niNode = mHeliNode->createChildSceneNode();
			niNode->attachObject(heliEnt);

			mHeliNode->setScale(npc_scale);
			AxisAlignedBox aab = heliEnt->getBoundingBox();
			
		Vector3	size  = aab.getSize()*npc_scale;
			//LogManager::getSingleton().logMessage(StringConverter::toString(npc_scale)+" "+StringConverter::toString(size));
			goal_achieved=true;
			//Vector3 size=physSize;
			OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size*physSize);
			npcBody = new OgreNewt::Body( mWorld, col,NEWTBODYTYPE_NPC);
			npcBody->attachToNode( mHeliNode );
			//LogManager::getSingleton().logMessage("Center:"+StringConverter::toString(aab.getCenter()));
			npcBody->setPositionOrientation( mHeliNode->getPosition(), mHeliNode->getOrientation() );
			//mHeliNode->setPosition(mHeliNode->getPosition();
			niNode->translate(physPos);
			niNode->rotate(axis,Degree(rangle));
			npcBody->setType(PHYSOBJECT_NPC);
			delete col;
			Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 40, size );
			npcBody->setMassMatrix( 40, inertia );
			npcBody->setName(mHeliNode->getName());
			npcBody->setCustomForceAndTorqueCallback<npc_aerial>( &npc_aerial::npc_force_callback ,this);
			npcBody->setAutoFreeze(0);
			OgreNewt::BasicJoints::UpVector* uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Y);
		OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Z);
		OgreNewt::BasicJoints::UpVector* uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_X);
		skel = heliEnt->getSkeleton();
		if (mProps.strange_look)
		{
			Animation* anim = skel->getAnimation("Walk");
			 anim->destroyNodeTrack(skel->getBone(mProps.headBone)->getHandle()); 
			anim->destroyNumericTrack(skel->getBone(mProps.headBone)->getHandle()); 
			anim->destroyVertexTrack(skel->getBone(mProps.headBone)->getHandle()); 
			skel->getBone(mProps.headBone)->setManuallyControlled(true);
		}
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
	if (flag==ALERT_NPC)
	{
		start();
		LogManager::getSingleton().logMessage("NPC is CRAZY");
		moveActivity=2;
	}
	if (flag==SETAI_NPC)
	{
		aiel=AIManager::getSingleton().getAIElementByNPCName(mName);
		if (aiel)
		{
		LogManager::getSingleton().logMessage("Setting swarm behavior.");
		moveActivity=0.5;
		behavior_type=1;
		aiel->setPosition(getpos(npcBody));
		}
		else
		{
			LogManager::getSingleton().logMessage("NPC is not in group.");
		}
	}
	
	if (flag==FEAR_NPC)
	{
		start();
		LogManager::getSingleton().logMessage("NPC is trying to escape from this horrible situation");
		moveActivity=0.3f;
	}
	if (flag==CRAZY_NPC)
	{
		moveActivity=0.3f;
	}

	if (flag==TAKEOFF_NPC)
	{
		mProps.landed=false;
		takeoff();
	}
	if (flag==LAND_NPC)
	{
		mProps.landed=false;
		land();
	}

	if (flag==REACH_NPC)
	{
		LogManager::getSingleton().logMessage("NPC reached destination");
		if (!mProps.luaOnReach.empty())
			RunLuaScript(global::getSingleton().getLuaState(),mProps.luaOnReach.c_str());
	}
	if (flag==GOTO_NPC)
	{
		start();
		LogManager::getSingleton().logMessage("NPC goes to "+param1);
		moveActivity=0.7f;
		destCMD=StringConverter::parseVector3(param1);
		cmdMode=true;
	}
	if (flag==KILL_NPC)
	{
		if (mProps.sounds)
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/bodysplat_1.wav",2,false,npcBody->damagePosition,200,50,800);
		LogManager::getSingleton().logMessage("NPC KILL!!!");
		 if (((npcBody->damagePosition-getpos(npcBody)).y>mProps.headshotDist)&&(mProps.headshot))
   {
	   BloodEmitter::getSingleton().emitBlood(npcBody->damagePosition,Vector3(0.7,0.7,0.7));
	   GibManager::getSingleton().spawnGib(mProps.headMesh,npcBody->damagePosition,mHeliNode->getScale(),4.0f);
		headshot();
   }
		BloodEmitter::getSingleton().emitBlood(npcBody->damagePosition,Vector3(0.3,0.3,0.3));
		if (mProps.exploding)
		{
			ExplosionManager::getSingleton().spawnExplosionNosound(npcBody->damagePosition,Vector3(2,2,2));
			if (mProps.sounds)
Run3SoundRuntime::getSingleton().emitSound("run3/sounds/explode_3.wav",4,false,npcBody->damagePosition,200,50,800);
			
			mProps.exploding=false;
		}
		kill();
	}
}

Vector3 npc_aerial::getpos(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
}

Quaternion npc_aerial::getorient(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return rot;
}

void npc_aerial::behavior0(const Ogre::FrameEvent &evt)
{
		


		fstTimer+=evt.timeSinceLastFrame*TIME_SHIFT*moveActivity;
		Vector3 myPos = mHeliNode->getPosition();
		if (fstTimer>0.5f)
		{
				fstindex++;
				
				if (mProps.sounds)
				{
				String fname = "run3/sounds/footsteps/foot"+StringConverter::toString(fstindex-1)+".wav";
				Run3SoundRuntime::getSingleton().emitSound(fname,1.0f,false,myPos,200,50,800);
				}
				if (fstindex==4)
					fstindex=1;
				fstTimer=0.0f;
		}

		if(dyeNow)
		{
			kill();
			return;
		}

		if (mProps.strange_look)
		{
			Ogre::Node* m = skel->getBone(mProps.headBone);
			lookAt(m,pPosit,Node::TS_WORLD,Vector3::NEGATIVE_UNIT_Z);
		}
		

		if (cmdMode) 
		{
			pPosit=destCMD;
			

		}
		bool farFindB = getpos(npcBody).distance(pPosit) < farFind;
		bool path_find = !goal_achieved;
		if (stopAtDist)
		blizko = getpos(npcBody).distance(pPosit) < stopDist;
		if (farFindB && goal_achieved)
		{
			path_find = find_path();
			i=0;
			if (path_find)
				goal_achieved=false;
		}
		else
		{
			steps=Ogre::Vector3::ZERO;
		}
		


		bool stra=check_straight();
		if (farFindB && path_find && !stra)
		{
			previous_player=true;
			if (!dyeNow)
			{
			Vector3 dest;
				if (animated)
				{
				mAnimState->setEnabled(false);
				}
				if (!notsetIdle&&!rotation_needed)
				{
							if (animated)
							{
								mAnimState = heliEnt->getAnimationState("Walk");
								mAnimState->setEnabled(true);
							}
				}
			if (mPath.size()>=2)
				dest = mPath[i]->getPosition();
			

			bool in_rad_precision = (pow((dest.x-myPos.x),2)+pow((dest.z-myPos.z),2))<=50;
			
			Vector3 dir = Vector3(dest.x,0,dest.z)-Vector3(myPos.x,0,myPos.z);
			dir.normalise();

			if (in_rad_precision)
			{
				int b,s;
				s=mPath.size();
				b = i+1;
				if (!(b>mPath.size()))
      					i++;
			if ((i)==mPath.size())
			{
				cmdMode=false;
				goal_achieved=true;
				processEvent(REACH_NPC,"","");
			}
			else
			{
							
				dest = mPath[i]->getPosition();
				dir = Vector3(dest.x,0,dest.z)-Vector3(myPos.x,0,myPos.z);
				dir.normalise();
				Vector3 dir1 = getNPCDirection();
				Degree angle;
				angle = Vector3(dir1.x,0,dir1.z).angleBetween(dir);
				//LogManager::getSingleton().logMessage(StringConverter::toString(dir1)+" "+StringConverter::toString(dir)+" "+StringConverter::toString(angle));
				rotation_needed = angle>=Degree(5.0f);

			}
			}

			if (!in_rad_precision&&!rotation_needed)
			{
				steps=dir;
			}
			

			

			if (rotation_needed)
			{
				if (animated)
				{
								mAnimState->setEnabled(false);
		    						mAnimState = heliEnt->getAnimationState("Stealth");
									mAnimState->setEnabled(true);
				}
				Vector3 dir1 = getNPCDirection();
				Degree angle;
				angle = Vector3(dir1.x,0,dir1.z).angleBetween(dir);

									reach_zero=angle.valueDegrees()>lastAngle;  //приблизился ли угол к нулю.если да, то не приблизился
									lastAngle=angle.valueDegrees();
									
									if (reach_zero)
									{
										rot_dir=!rot_dir;
									}
									
									
									if (rot_dir)
									{
										//LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"+");
									niNode->rotate(Vector3::UNIT_Y,Degree(10.0f*evt.timeSinceLastFrame*TIME_SHIFT*5));
									}
									else
									{
										//LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"-");
										niNode->rotate(Vector3::UNIT_Y,Degree(-10.0f*evt.timeSinceLastFrame*TIME_SHIFT*5));
									}
				
				steps=0;
				
				rotation_needed = Degree(fabs(angle.valueDegrees()))>=Degree(5.0f);
			}

			}
		}


		if (!path_find || (!goal_achieved && stra)) 
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
				if ((sounds)&& (mProps.sounds))
				{
				if ((rand() % 50)==2)
					Run3SoundRuntime::getSingleton().emitSound("run3/sounds/breathing3.wav",3,false,myPos,200,50,800);
				if ((rand() % 50)==3)
					Run3SoundRuntime::getSingleton().emitSound("run3/sounds/cough"+StringConverter::toString(rand() % 3+1)+".wav",3,false,myPos,200,50,800);
				
				}
				Vector3 dest = Vector3(pPosit.x,0,pPosit.z);
				//Vector3 myPos = mHeliNode->getPosition();
				Vector3 dir = dest-Vector3(myPos.x,0,myPos.z);

				if (previous_player)
				{
				if (mProps.sounds)
					Run3SoundRuntime::getSingleton().emitSound("run3/sounds/attack_growl3.wav",6,false,myPos,200,50,800);

				previous_player=false;
				}

				if (farFindB)
				{


							if (dir.length()<attackAnimDist)
							{
								steps = dir.normalisedCopy();
								if (mProps.sounds)
								{
								if ((rand() % 20)==4)
									Run3SoundRuntime::getSingleton().emitSound("run3/sounds/attack_growl3.wav",3,false,myPos,200,50,800);
			
								if ((rand() % 5)==4)
									Run3SoundRuntime::getSingleton().emitSound("run3/sounds/iceaxe_swing1.wav",3,false,myPos,200,50,800);
								}
								if (animated)
								{
								mAnimState->setEnabled(false);
								mAnimState = heliEnt->getAnimationState("Attack1");
								mAnimState->setEnabled(true);
								}
							}
							else
							{
									dir.normalise();
							Vector3 dir1 = getNPCDirection();
							Degree angle;
							angle = Vector3(dir1.x,0,dir1.z).angleBetween(dir);

							/*if (dir1.z<dir.z)
								angle+=Degree(180);*/
							//bool rot_needed2 = rotation_needed;
							rotation_needed = Degree(fabs(angle.valueDegrees()))>=Degree(5.0f);


								if (!rotation_needed)
								{

									if (animated)
									{
									mAnimState->setEnabled(false);
								mAnimState = heliEnt->getAnimationState("Walk");
								mAnimState->setEnabled(true);
									}
									steps = dir;
								}
								else
								{
									if (animated)
									{
									mAnimState->setEnabled(false);
		    						mAnimState = heliEnt->getAnimationState("Stealth");
									mAnimState->setEnabled(true);
									}
									reach_zero=angle.valueDegrees()>lastAngle;  //приблизился ли угол к нулю.если да, то не приблизился
									lastAngle=angle.valueDegrees();
									
									if (reach_zero)
									{
										rot_dir=!rot_dir;
									}
									////LogManager::getSingleton().logMessage(StringConverter::toString(reach_zero));
									
									if (rot_dir)
									{
										////LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"+");
									niNode->rotate(Vector3::UNIT_Y,Degree(10.0f*evt.timeSinceLastFrame*TIME_SHIFT*5));
									}
									else
									{
										////LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"-");
										niNode->rotate(Vector3::UNIT_Y,Degree(-10.0f*evt.timeSinceLastFrame*TIME_SHIFT*5));
									}
								}
							}
				}
			}
		}
}

void npc_aerial::behavior1(const Ogre::FrameEvent &evt)
{
	if (aiel==0)
		return;
	steps=aiel->getVelocity();
	aiel->setPosition(getpos(npcBody));
}

void npc_aerial::step(const Ogre::FrameEvent &evt)
{
	if (TIME_SHIFT==0.0f)
		return;
	if (animated)
	{
	mAnimState->addTime(evt.timeSinceLastFrame*TIME_SHIFT*moveActivity);
	}
	

	if (going)
	{
		switch(behavior_type)
		{
		case 0:
			behavior0(evt);
			break;
		case 1:
			behavior1(evt);
		default:
			break;
		}
	}
	////LogManager::getSingleton().logMessage("enemy end step");
} 

bool npc_aerial::find_path()
{
	//LogManager::getSingleton().logMessage("pathfind");
	Vector3 myPos = getpos(npcBody);
	bool straight = check_straight();
	
	int s = mDebugPathNodes.size();
	
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
			pos=nodes[i];
		}

		if(check_straight(pPosit,nodes[i]->getPosition()))
		{
		Real r3 = pPosit.distance(nodes[i]->getPosition());
		Real r4 = pPosit.distance(end->getPosition());
		if (r3 <r4 )
			end=nodes[i];
		}
	}
	//LogManager::getSingleton().logMessage(StringConverter::toString("found"));
	//LogManager::getSingleton().logMessage("to enemy"+StringConverter::toString(pos->getPosition()));
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

bool npc_aerial::find_path(Vector3 destPos)
{
	//LogManager::getSingleton().logMessage("pathfind");
	Vector3 myPos = getpos(npcBody);
	bool straight = check_straight(myPos,destPos);
	
	int s = mDebugPathNodes.size();
	
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
			pos=nodes[i];
		}

		if(check_straight(destPos,nodes[i]->getPosition()))
		{
		Real r3 = destPos.distance(nodes[i]->getPosition());
		Real r4 = destPos.distance(end->getPosition());
		if (r3 <r4 )
			end=nodes[i];
		}
	}
	//LogManager::getSingleton().logMessage(StringConverter::toString("found"));
	//LogManager::getSingleton().logMessage("to enemy"+StringConverter::toString(pos->getPosition()));
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

bool npc_aerial::check_straight()
{
	Vector3 myPos=getpos(npcBody);
	
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
	return false;
}

bool npc_aerial::check_straight(Vector3 n1,Vector3 n2)
{
	OgreNewt::BasicRaycast ray(mWorld,n1,n2);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t) 
		return false; 
	else 
		return true;
}

void npc_aerial::kill()
{
	going=false;
	dyeNow=true;
	notsetIdle=true;
	//delete npcBody;
	//npcBody=0;
	npcBody->setName("DEAD_NPC");
	npcBody->freeze();
	if (!mProps.luaOnDeath.empty())
	RunLuaScript(global::getSingleton().getLuaState(),mProps.luaOnDeath.c_str());
	mProps.applyGravity=true;

			if (animated)
		{
	mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState("Death1");
			mAnimState->setLoop(false);
			mAnimState->setEnabled(true);
		}
}
bool npc_aerial::frameStarted(const Ogre::FrameEvent &evt)
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