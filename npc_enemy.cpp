#include "npc_enemy.h"


npc_enemy::npc_enemy()
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
LogManager::getSingleton().logMessage("NPC enemy init.");
//mDebugLines=0;
}

npc_enemy::~npc_enemy()
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

void npc_enemy::init(NPCSpawnProps props)
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
			//mDebugPathNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("npc_enemy_path_"+props.mName);
			//mDebugLines = new Ogre::ManualObject("npc_enemy_path_"+props.mName);
			//mRoot->addFrameListener(this);
			health=props.health;
 }

void npc_enemy::setPlayerPosition(Vector3 pos)
{
pPosit=pos;
}

void npc_enemy::headshot()
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


void npc_enemy::npc_force_callback( OgreNewt::Body* me)
{
 //apply a simple gravity force.
	/*if (parentRelation)
		{
			me->setPositionOrientation(mPar->getPosition()+pRelPosition,Quaternion(Radian(Degree(x_rotation_cont)),Vector3::UNIT_Y));
		}
	   else
	   {*/
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
  // }
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

void npc_enemy::spawnNPC()
{
			//mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);
			
			//heliEnt=mSceneMgr->createEntity(mName,npc_mesh);
			initFacialSystem();
			if (mName=="")
			{

				mName=mHeliNode->getName();
			}
			heliEnt->setRenderingDistance(mProps.renderDist);
			if (!(mProps.materialName.empty()))
				heliEnt->setMaterialName(mProps.materialName);
			niNode = mHeliNode->createChildSceneNode();
			niNode->attachObject(heliEnt);
			niNode->rotate(mProps.rot);
			mHeliNode->setScale(npc_scale);
			AxisAlignedBox aab = heliEnt->getBoundingBox();
			
		Vector3	size  = aab.getSize()*npc_scale;
			//LogManager::getSingleton().logMessage(StringConverter::toString(npc_scale)+" "+StringConverter::toString(size));
			goal_achieved=true;
			//Vector3 size=physSize;
			OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size*physSize,Quaternion(Radian(Degree(mProps.yShift)),Vector3::UNIT_Y),physPos);
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
			npcBody->setCustomForceAndTorqueCallback<npc_enemy>( &npc_enemy::npc_force_callback ,this);
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
			Ogre::Node* m = skel->getBone(mProps.headBone);
			Quaternion finalOrient = Quaternion::IDENTITY*Quaternion(Degree(180),Vector3::UNIT_X);
			m->setOrientation(finalOrient);
		}

		niNode->rotate(Vector3::UNIT_Y,Degree(mProps.yShift));

			if (animated)
			{
			mAnimState = heliEnt->getAnimationState("Idle1");
			mAnimState->setEnabled(true);
			}
}

void npc_enemy::processEvent(int flag,String param1,String param2)
{
	processBaseEvent(flag,param1,param2);

	if (flag==GOTO_NPC)
	{
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

void npc_enemy::step(const Ogre::FrameEvent &evt)
{
	Real angleSwitch = 0;
	if (TIME_SHIFT==0.0f)
		return;
	if (animated)
	{
	mAnimState->addTime(evt.timeSinceLastFrame*TIME_SHIFT*moveActivity);
	}
	

	if (going)
	{
		bool farFindB = getpos(npcBody).distance(pPosit) < farFind;
		if (farFindB)
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
				if (mProps.sounds)
				{
				String fname = "run3/sounds/footsteps/"+mProps.soundFootstepPrefix;
				fname=fname+""+StringConverter::toString(fstindex-1)+".wav";
				Run3SoundRuntime::getSingleton().emitSound(fname,1.0f,false,myPos,200,50,800);
				}
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
		//Look part!
		
		

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
				Vector3 dir1 = Quaternion(Radian(Degree(mProps.yShift)),Vector3::UNIT_Y)*getNPCDirection();
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
				Vector3 dir1 = Quaternion(Radian(Degree(mProps.yShift)),Vector3::UNIT_Y)*getNPCDirection();
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
									niNode->rotate(Vector3::UNIT_Y,Degree(mProps.rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
									}
									else
									{
										//LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"-");
										niNode->rotate(Vector3::UNIT_Y,Degree(-mProps.rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
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
				if ((sounds)&& (mProps.sounds))
				{
				if ((rand() % 50)==2)
					Run3SoundRuntime::getSingleton().emitSound(mProps.soundRandom1,3,false,myPos,200,50,800);
				if ((rand() % 50)==3)
				{
					int sosel = rand() % 3+1;
					if (sosel==1)
						Run3SoundRuntime::getSingleton().emitSound(mProps.soundSubRandom21,3,false,myPos,200,50,800);
					if (sosel==2)
						Run3SoundRuntime::getSingleton().emitSound(mProps.soundSubRandom22,3,false,myPos,200,50,800);
					if (sosel==3)
						Run3SoundRuntime::getSingleton().emitSound(mProps.soundSubRandom23,3,false,myPos,200,50,800);
				}
				}
				Vector3 dest = Vector3(pPosit.x,0,pPosit.z);
				//Vector3 myPos = mHeliNode->getPosition();
				Vector3 dir = dest-Vector3(myPos.x,0,myPos.z);

				if (previous_player)
				{
				if (mProps.sounds)
					Run3SoundRuntime::getSingleton().emitSound(mProps.soundFind,6,false,myPos,200,50,800);

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
									Run3SoundRuntime::getSingleton().emitSound(mProps.soundAttack,3,false,myPos,200,50,800);
			
								if ((rand() % 5)==4)
									Run3SoundRuntime::getSingleton().emitSound(mProps.soundHit,3,false,myPos,200,50,800);
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
							Vector3 dir1 = Quaternion(Radian(Degree(mProps.yShift)),Vector3::UNIT_Y)*getNPCDirection();
							Degree angle;
							angle = Vector3(dir1.x,0,dir1.z).angleBetween(dir);
							angleSwitch=angle.valueDegrees();
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
									niNode->rotate(Vector3::UNIT_Y,Degree(mProps.rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
									}
									else
									{
										////LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"-");
										niNode->rotate(Vector3::UNIT_Y,Degree(-mProps.rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
									}
								}
							}
				}
			}
		}


		
	}
	if (mProps.strange_look)
		{
			Ogre::Node* m = skel->getBone(mProps.headBone);
			Quaternion finalOrient = Quaternion::IDENTITY*Quaternion(Degree(angleSwitch),Vector3::UNIT_X);
			m->setOrientation(finalOrient);
			//Ogre::Node::
			//lookAt(m,Vector3(0,0,-500),Node::TS_PARENT,Vector3::NEGATIVE_UNIT_Z);
		}
	////LogManager::getSingleton().logMessage("enemy end step");
} 

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

bool npc_enemy::check_straight()
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

bool npc_enemy::check_straight(Vector3 n1,Vector3 n2)
{
	OgreNewt::BasicRaycast ray(mWorld,n1,n2);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t) 
		return false; 
	else 
		return true;
}

void npc_enemy::kill()
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