#include "npc_friend.h"


npc_friend::npc_friend()
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
//mDebugLines=0;
}

npc_friend::~npc_friend()
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

void npc_friend::init(NPCSpawnProps props)
{
	//LogManager::getSingleton().logMessage("Npc manager: npc friend initializing");
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
			//LogManager::getSingleton().logMessage("Npc manager: npc friend ended initializing");
			//mDebugPathNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("npc_friend_path_"+props.mName);
			//mDebugLines = new Ogre::ManualObject("npc_friend_path_"+props.mName);
			//mRoot->addFrameListener(this);
			health=props.health;
 }

void npc_friend::setPlayerPosition(Vector3 pos)
{
pPosit=pos;
}

void npc_friend::start()
{
going=true;
}

void npc_friend::suspend()
{
going=false;
}

 void npc_friend::resume()
{

}

void npc_friend::headshot()
{
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


void npc_friend::npc_force_callback( OgreNewt::Body* me)
{
 //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Vector3 stdAdd = Vector3::ZERO;
   bool onEarth = isOnEarth();
   Real health1=health;
   health-=me->getDamage();
  
	if (health1!=health)
	{
		LogManager::getSingleton().logMessage("NPC DAMAGE!!!");
		 if (((me->damagePosition-getpos(me)).y>mProps.headshotDist)&&(mProps.headshot))
   {
	   BloodEmitter::getSingleton().emitBlood(me->damagePosition,Vector3(0.7,0.7,0.7));
	   GibManager::getSingleton().spawnGib(mProps.headMesh,me->damagePosition,mHeliNode->getScale(),4.0f);
		headshot();
   }
		BloodEmitter::getSingleton().emitBlood(me->damagePosition,Vector3(0.3,0.3,0.3));
		if (mProps.exploding)
		{
			ExplosionManager::getSingleton().spawnExplosionNosound(me->damagePosition,Vector3(2,2,2));
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
   Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,0,steps.z*mVel+stdAdd.z*5*mVel);
  // //LogManager::getSingleton().logMessage(StringConverter::toString(steps));
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
   if ((onEarth)||(!mProps.applyGravity))
   me->setVelocity( force2*TIME_SHIFT *moveActivity);
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
	   me->setVelocity(Vector3::ZERO);
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


bool npc_friend::isOnEarth()
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


void npc_friend::processEvent(int flag,String param1,String param2)
{
	if (flag==SPAWN_NPC)
	{
		
			mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);

			if (mName=="")
			{

				mName=mHeliNode->getName();
			}
			
			heliEnt=mSceneMgr->createEntity(mName,npc_mesh);
			if (!(mProps.materialName.empty()))
				heliEnt->setMaterialName(mProps.materialName);
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
			npcBody->setCustomForceAndTorqueCallback<npc_friend>( &npc_friend::npc_force_callback ,this);
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
	if (flag==GOTO_NPC)
	{
		LogManager::getSingleton().logMessage("NPC goes to "+param1);
		moveActivity=0.7f;
		destCMD=StringConverter::parseVector3(param1);
		cmdMode=true;
	}
	if (flag==KILL_NPC)
	{
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
Run3SoundRuntime::getSingleton().emitSound("run3/sounds/explode_3.wav",4,false,npcBody->damagePosition,200,50,800);
			
			mProps.exploding=false;
		}
		kill();
	}
}

Vector3 npc_friend::getpos(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
}

Quaternion npc_friend::getorient(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return rot;
}

void npc_friend::step(const Ogre::FrameEvent &evt)
{
	if (TIME_SHIFT==0.0f)
		return;
	if (animated)
	{
	mAnimState->addTime(evt.timeSinceLastFrame*TIME_SHIFT*moveActivity);
	}
	

	if (going)
	{
		


		fstTimer+=evt.timeSinceLastFrame*TIME_SHIFT*moveActivity;
		Vector3 myPos = mHeliNode->getPosition();
		if (fstTimer>0.5f)
		{
				fstindex++;
				//fstindex=rand() % 4;
				/*sound->setSoundPosition(footsteps[fstindex],get_location(),Vector3::ZERO,Vector3::ZERO);
				sound->stopAudio(footsteps[fstindex]);
				sound->playAudio(footsteps[fstindex],true);
				fstindex++;
				if (fstindex>footsteps.size()-1)
					fstindex=0;
				fstTimer=0.0f;*/
				String fname = "run3/sounds/footsteps/foot"+StringConverter::toString(fstindex-1)+".wav";
				Run3SoundRuntime::getSingleton().emitSound(fname,1.0f,false,myPos,200,50,800);
				
				if (fstindex==4)
					fstindex=1;
				fstTimer=0.0f;
		}

		//mHeliNode->setPosition(mHeliNode->getPosition()-Vector3(0,600.0f,0));
		if(dyeNow)
		{
			kill();
			return;
		}
		//Animation* anim = skel->getAnimation("Walk");
		if (mProps.strange_look)
		{
			Ogre::Node* m = skel->getBone(mProps.headBone);
			lookAt(m,pPosit,Node::TS_WORLD,Vector3::NEGATIVE_UNIT_Z);
		}
		

		if (cmdMode) 
		{
			pPosit=destCMD;
			/*bool farFindB = getpos(npcBody).distance(destCMD) < farFind;
			bool path_find = !goal_achieved;
			if (stopAtDist)
			blizko = getpos(npcBody).distance(pPosit) < stopDist;*/

		}
		//else
		//{

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
			//bool rotation_needed=false;

			if (in_rad_precision)
			{
				//body_rot_y=0;
				int b,s;
				s=mPath.size();
				b = i+1;
				if (!(b>mPath.size()))
      					i++;
			if ((i)==mPath.size())
			{
				cmdMode=false;
				goal_achieved=true;
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
				////LogManager::getSingleton().logMessage(StringConverter::toString(dir1)+" "+StringConverter::toString(dir)+" "+StringConverter::toString(angle));
				////LogManager::getSingleton().logMessage("Rotating in Player not found");
				//if (angle>Degree(10.0f))
									reach_zero=angle.valueDegrees()>lastAngle;  //приблизился ли угол к нулю.если да, то не приблизился
									lastAngle=angle.valueDegrees();
									
									if (reach_zero)
									{
										rot_dir=!rot_dir;
									}
									//LogManager::getSingleton().logMessage(StringConverter::toString(reach_zero));
									
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
				/*else
				niNode->rotate(Vector3::UNIT_Y,angle);*/
				steps=0;
				////LogManager::getSingleton().logMessage(StringConverter::toString(dir1)+" "+StringConverter::toString(dir)+" "+StringConverter::toString(angle));
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
				if (sounds)
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

					Run3SoundRuntime::getSingleton().emitSound("run3/sounds/attack_growl3.wav",6,false,myPos,200,50,800);

				previous_player=false;
				}

				if (farFindB)
				{


							if (dir.length()<attackAnimDist)
							{
								steps = dir.normalisedCopy();

								if ((rand() % 20)==4)
									Run3SoundRuntime::getSingleton().emitSound("run3/sounds/attack_growl3.wav",3,false,myPos,200,50,800);
			
								if ((rand() % 5)==4)
									Run3SoundRuntime::getSingleton().emitSound("run3/sounds/iceaxe_swing1.wav",3,false,myPos,200,50,800);
								
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
							/*if (rot_needed2!=rotation_needed)
							{
								if ((rand() % 3)==2)
									angle=-angle;
							}*/

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
	////LogManager::getSingleton().logMessage("friend end step");
} 

/*Radian angleBetweenR(const Vector3& dest)
{
			Real lenProduct = length() * dest.length();

			// Divide by zero check
			if(lenProduct < 1e-6f)
				lenProduct = 1e-6f;

			Real f = dotProduct(dest) / lenProduct;

			return Math::ACos(f);

}*/

bool npc_friend::find_path()
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
	//LogManager::getSingleton().logMessage("to friend"+StringConverter::toString(pos->getPosition()));
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

bool npc_friend::find_path(Vector3 destPos)
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
	//LogManager::getSingleton().logMessage("to friend"+StringConverter::toString(pos->getPosition()));
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

bool npc_friend::check_straight()
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

bool npc_friend::check_straight(Vector3 n1,Vector3 n2)
{
	OgreNewt::BasicRaycast ray(mWorld,n1,n2);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t) 
		return false; 
	else 
		return true;
}

void npc_friend::kill()
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
	/*if (mProps.headshot)
	{
		LogManager::getSingleton().logMessage("HEADSHOT!!!");
		//mAnimState
		Skeleton* skel = heliEnt->getSkeleton();
		Animation* anim = skel->getAnimation("Death1");
		
/*#define MAKE_SOME_GORE(s) anim->destroyNodeTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyNumericTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyVertexTrack(skel->getBone(s)->getHandle()); \
		skel->getBone(s)->setManuallyControlled(true); \
		skel->getBone(s)->setScale(0.01,0.01,0.01); \*/

		/*heliEnt->getSkeleton()->getBone("BMan0002-Head")->setScale(0.01,0.01,0.01);
		heliEnt->getSkeleton()->getBone("BMan0002-Spine2")->setScale(0.01,0.01,0.01);
		heliEnt->getSkeleton()->getBone("BMan0002-HeadNub")->setScale(0.01,0.01,0.01);
		heliEnt->getSkeleton()->getBone("BMan0002-R Forearm")->setScale(0.01,0.01,0.01);*/
		//MAKE_SOME_GORE("BMan0002-Head")
		/*MAKE_SOME_GORE("BMan0002-Spine2")
		MAKE_SOME_GORE("BMan0002-HeadNub")
		MAKE_SOME_GORE("BMan0002-R Forearm")*/
	//}*/
			if (animated)
		{
	mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState("Death1");
			mAnimState->setLoop(false);
			mAnimState->setEnabled(true);
		}
}
bool npc_friend::frameStarted(const Ogre::FrameEvent &evt)
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