#include "npc_neutral.h"
#include "PhysObject.h"
#include "FacialAnimationManager.h"
#include "LoadMap.h"

npc_neutral::npc_neutral()
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
	parentRelation=false;
	pRelPosition=Vector3::ZERO;
	luaToExec="";
	luaOnUse="";
	mTransitAnimState=0;
	transitState=1.0f;
	time256=0;
	goalHeadYaw=0;
	gravity=-98;
	mFac=NULL;
	notice=true;
	flashLight=0;
	dead=false;
	LogManager::getSingleton().logMessage("NPC neutral init.");
	//mDebugLines=0;
}

npc_neutral::~npc_neutral()
{
	flush();
}

inline void npc_neutral::flush()
{
	if (animated&&mAnimState)
	{
		mAnimState->setEnabled(false);
		mAnimState=0;
	}
	if (npcBody)
	{
		delete npcBody;
		npcBody = 0;
	}
	if (heliEnt)
	{
	heliEnt->detatchFromParent();
	mSceneMgr->destroyEntity(heliEnt);
	heliEnt=0;
	}
	if (mHeliNode)
	{
		mSceneMgr->destroySceneNode(mHeliNode);
		mHeliNode=0;
	}
	int s = mDebugPathNodes.size();
	for (unsigned int i=0;i!=s;i++)
	{
		mSceneMgr->destroyEntity(mDebugPathNodes[i]->getName());
		mSceneMgr->destroySceneNode(mDebugPathNodes[i]);
	}
	mDebugPathNodes.clear();


	mPath.clear();
	if (mFac)
	{
		delete mFac;
		mFac=0;
	}
}

//////////////////////////////////////////
void npc_neutral::setParentRelation(SceneNode* pPar)
{
	parentRelation=true;
	mPar=pPar;
	pRelPosition=getpos(npcBody)-pPar->_getDerivedPosition();
	pRelQuat=getorient(npcBody).Inverse();
	pRelQuat=pPar->_getDerivedOrientation()*pRelQuat;
	pRelQuat2=pPar->_getDerivedOrientation().Inverse();
}
//////////////////////////////////////////
void npc_neutral::setParentRelation(String node)
{
	SceneNode* pPar = mSceneMgr->getSceneNode(node);
	setParentRelation(pPar);
}

///
void npc_neutral::init(NPCSpawnProps props)
{
			//LogManager::getSingleton().logMessage("Npc manager: npc neutral initializing");
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
			rotateSpeed = props.rotateSpeed;
			rangle=props.angle;
			axis=props.ax;
			attackAnimDist=props.attackAnimDist;
			//LogManager::getSingleton().logMessage("Npc manager: npc neutral ended initializing");
			//mDebugPathNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("npc_neutral_path_"+props.mName);
			//mDebugLines = new Ogre::ManualObject("npc_neutral_path_"+props.mName);
			//mRoot->addFrameListener(this);
			health=props.health;
 }

void npc_neutral::setPlayerPosition(Vector3 pos)
{
pPosit=pos;
}

void npc_neutral::headshot()
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

inline void npc_neutral::processHealthLoss(OgreNewt::Body* me)
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

void npc_neutral::npc_force_callback( OgreNewt::Body* me)
{
 //apply a simple gravity force.
	if (me->getUsing())
	{
		if (!(luaOnUse.empty()))
				RunLuaScript(global::getSingleton().getLuaState(),luaOnUse.c_str());
	}

	if (parentRelation)
	{
			me->setPositionOrientation(mPar->_getDerivedPosition()+(mPar->_getDerivedOrientation()*pRelQuat2)*pRelPosition,mPar->_getDerivedOrientation()*pRelQuat);//getorient(npcBody));
	}
	else
	{
			//Main behaviour
			Ogre::Real mass; 
			Ogre::Vector3 inertia; 
			Vector3 stdAdd = Vector3::ZERO;
			bool onEarth = isOnEarth();
			Real health1=health;
			health-=me->getDamage();
			  
			//Helath processing
			if (health1!=health)
			{
					processHealthLoss(me);
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
			   
			Ogre::Vector3 force2(steps.x*mVel+stdAdd.x*5*mVel,0,steps.z*mVel+stdAdd.z*5*mVel);
			Ogre::Vector3 force;
			if (mProps.applyGravity)
				force=Ogre::Vector3(0,gravity,0); 
			else
				force=Vector3::ZERO;

			force *= mass; 

			if (!onEarth)
					me->addForce( force*TIME_SHIFT*TIME_SHIFT*moveActivity );

			me->setStandartAddForce(Ogre::Vector3::ZERO);
			 
			if ((onEarth)||(!mProps.applyGravity))
			me->setVelocity( force2*TIME_SHIFT *moveActivity);
  
   }

   if (!going && !dyeNow)
   {
	   if (!cmdMode)
	   {
			//if (animated)
			//	mAnimState->setEnabled(false);
			if (!notsetIdle)
			{
				if (animated)
				{
			//mAnimState = heliEnt->getAnimationState("Idle1");
			//mAnimState->setEnabled(true);
				}
			}
	   }
	   me->setVelocity(Vector3::ZERO);
   }

	//FIX IT!!!
   //pPosit=mStartPos;
   if (!dyeNow)
   {
		me->setOmega(Vector3(0,body_rot_y*TIME_SHIFT,0));
   }
   else
   {
		me->setOmega(Vector3(0,0,0));
   }

}

void npc_neutral::spawnNPC()
{
			uniqueMult = 0.9f + Math::RangeRandom(0,100)/500;
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
			npcBody->setCustomForceAndTorqueCallback<npc_neutral>( &npc_neutral::npc_force_callback ,this);
			npcBody->setAutoFreeze(0);
			OgreNewt::BasicJoints::UpVector* uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Y);
		OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_Z);
		OgreNewt::BasicJoints::UpVector* uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,npcBody,Vector3::UNIT_X);
		skel = heliEnt->getSkeleton();
		pPosit=mStartPos;
		if (mProps.strange_look)
		{
			for (unsigned int i=0;i!=skel->getNumAnimations();i++)
			{
			Animation* anim = skel->getAnimation(i);
			 anim->destroyNodeTrack(skel->getBone(mProps.headBone)->getHandle()); 
			anim->destroyNumericTrack(skel->getBone(mProps.headBone)->getHandle()); 
			anim->destroyVertexTrack(skel->getBone(mProps.headBone)->getHandle()); 
			skel->getBone(mProps.headBone)->setManuallyControlled(true);
			}
			
		}
		niNode->rotate(Vector3::UNIT_Y,Degree(mProps.yShift));
			if (animated)
			{
			mAnimState = heliEnt->getAnimationState("Idle1");
			mAnimState->setEnabled(true);
			}
}

void npc_neutral::processEvent(int flag,String param1,String param2)
{
	processBaseEvent(flag,param1,param2);

	if (flag==TRANSIT_ANIMATION)
	{
		if (animated)
		{
			transitFromCurrentAnimation(param1);
		}
	}
	if (flag==SETGRAVITY_NPC)
	{
		gravity=StringConverter::parseReal(param1);
	}

	if (flag==TOGGLE_FLASHLIGHT)
	{
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/flash01.wav",2.0f,false);

		if (flashLight&&(mSceneMgr->hasLight("flashLighter"+niNode->getName())))
		{
			niNode->detachObject(flashLight);
			if (flashLight)
				mSceneMgr->destroyLight(flashLight);
			flashLight=0;
		}
		else
		{
			if (mSceneMgr->hasLight("flashLighter"+niNode->getName()))
			{
			flashLight=mSceneMgr->getLight("flashLighter"+niNode->getName());
			}
			else
			{
			flashLight = mSceneMgr->createLight("flashLighter"+niNode->getName());
			}
	
			flashLight->setDiffuseColour(Ogre::ColourValue(0.5, 0.5, 0.5));
			flashLight->setPosition(0,0,-20);
			flashLight->setSpecularColour(Ogre::ColourValue(0,0,0));
			flashLight->setType(Ogre::Light::LT_SPOTLIGHT);
			flashLight->setSpotlightInnerAngle(Ogre::Degree(60.0f));
			flashLight->setSpotlightOuterAngle(Ogre::Degree(80.0f));
			flashLight->setAttenuation(3000, 1, 1, 1); // meter range.  the others our shader ignores
			flashLight->setDirection(Ogre::Vector3(0, 0, -1));

			niNode->attachObject(flashLight);

			flashLight->setVisible(true);
		}
	}
	
	if (flag==SET_MOVEACTIVITY)
	{
		moveActivity=StringConverter::parseReal(param1);
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
		goal_achieved=true;
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
	if (flag==SETPARENT_NPC)
	{
		setParentRelation(param1);
	}
	if (flag==RESETPARENT_NPC)
	{
		stopParentRelation();
	}
	if (flag==SETGOAL_SCRIPT)
	{
		luaToExec=param1;
	}
	if (flag==SETUSE_SCRIPT)
	{
		luaOnUse=param1;
	}
	if (flag==ROTATE_OVERRIDE_NPC)
	{
		npcBody->setPositionOrientation(getpos(npcBody),StringConverter::parseQuaternion(param1));	
	}
	if (flag==TELEPORT_PARENT_NPC)
	{
		//mPar=pPar;
		if (mPar)
		{
			//It just adds to mPar position, so no more components
			pRelPosition=mPar->getOrientation()*StringConverter::parseVector3(param1)*mPar->getScale();
			teleport(StringConverter::parseVector3(param1)+mPar->getPosition());
		}
	}
	if (flag==TOGGLE_GRAVITY)
	{

	}
	if (flag==ATTACH_PHYSOBJECT)
	{
		Ogre::Node* hand = skel->getBone(mProps.handBone);
		vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==param1)
				{
					pobj[i]->attachToBone(hand,niNode);
				}

			}
	}	
	if (flag==DETACH_PHYSOBJECT)
	{
		vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==param1)
				{
					pobj[i]->detachFromBone();
				}

			}
	}	

	if (flag==ATTACH_PHYSOBJECT2)
	{
		Ogre::Node* hand = skel->getBone(mProps.handBone);
		vector<PhysObject*> pobj = global::getSingleton().pobjects;
			for (int i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->getName()==param1)
				{
					pobj[i]->attachToBone(hand,niNode);
					attachedPhysObjects.push_back(pobj[i]);
				}

			}
	}	
	if (flag==DETACH_PHYSOBJECT2)
	{
		vector<PhysObject*>::iterator i;
			for (i=attachedPhysObjects.begin();i!=attachedPhysObjects.end();)
			{
				if ((*i)->getName()==param1)
				{
					LogManager::getSingleton().logMessage("1");
					PhysObject* obj = (*i);
					LogManager::getSingleton().logMessage("2");
					attachedPhysObjects.erase(i);
					LogManager::getSingleton().logMessage("3");
					obj->detachFromBone();
					LogManager::getSingleton().logMessage("4");
				}
				else
				{
					i++;
				}

			}
			LogManager::getSingleton().logMessage("5");
	}
}

void npc_neutral::transitFromCurrentAnimation(String newAnim)
{
	mTransitAnimState = heliEnt->getAnimationState(newAnim);
	mTransitAnimState->setWeight(0);
	mTransitAnimState->setEnabled(true);
	transitState=1.0f;
}

inline void npc_neutral::processAttachedObjects()
{
	for (int j=0;j!=attachedPhysObjects.size();j++)
	{
		attachedPhysObjects[j]->updateAttached();
	}
}

inline void npc_neutral::processAnimationTransition(Real mulTime)
{
	#ifdef DEBUG_FACIAL
				LogManager::getSingleton().logMessage("Animation ok.");
		#endif
		//Smooth transition part.
		if ((transitState>0)&&(mTransitAnimState)) //Transition started
		{
			mAnimState->setWeight(transitState);
			mTransitAnimState->setWeight(1-transitState);
			transitState-=mulTime; //1 Second average?
		}

		if ((transitState<=0)&&(mTransitAnimState)) //Transition ended
		{
			mAnimState->setWeight(1.0f);
			mAnimState->setEnabled(false);
			mTransitAnimState->setWeight(1.0f);
			mAnimState=mTransitAnimState;
			mTransitAnimState=0;
			transitState=1.0f;
		}
}

inline void npc_neutral::processLook(Real mulTime)
{
	if (mProps.strange_look)
	{
			Ogre::Node* m = skel->getBone(mProps.headBone);
			Vector3 dir1 = getNPCDirection();
			Degree angle1,angle2,angle3,angle4;
			Vector3 ppsit = global::getSingleton().getPlayer()->get_location()*(1-getCycle8())+getCycle8()*(getpos(npcBody)-Vector3(dir1.x,0,-dir1.z));
			Vector3 eyeDir = Vector3(getpos(npcBody).x-ppsit.x,0,getpos(npcBody).z-ppsit.z);
			//LogManager::getSingleton().logMessage(StringConverter::toString(getCycle8()));
			Quaternion newRot = eyeDir.getRotationTo(Vector3(dir1.x,0,-dir1.z),mProps.headAxis);
			newRot=newRot.Inverse();
			Real yawAngle;
			yawAngle = newRot.getYaw().valueDegrees();
			if ((fabs(newRot.getYaw().valueDegrees()))>45.0f)
			yawAngle *= Ogre::Math::Exp(-Ogre::Math::Sqr(fabs(newRot.getYaw().valueDegrees())/45.0f-1.0f));

			/*if ((newRot.getYaw().valueDegrees())>45.0f)
				newRot=Quaternion(Radian(Degree(45.0f)),mProps.headAxis);
			if ((newRot.getYaw().valueDegrees())<-45.0f)
				newRot=Quaternion(Radian(Degree(-45.0f)),mProps.headAxis);*/
			
			goalHeadYaw+=(yawAngle-goalHeadYaw)*mulTime;	//1 Second
			m->setOrientation(getorient(npcBody)*Quaternion(Radian(Degree(goalHeadYaw)),mProps.headAxis));
	}
}

inline void npc_neutral::processRandomMovements(Real time)
{
	if (!animatedBones.size())
		return;
	vector<Ogre::Node*>::iterator i;
	for (i=animatedBones.begin();i!=animatedBones.end();i++)
	{
		
	}
		/*Ogre::Node* rforearm = skel->getBone(" R ForeArm");
		Ogre::Node* lforearm = skel->getBone(" L ForeArm");
		Ogre::Node* rthigh = skel->getBone(" R Thigh");
		Ogre::Node* lthigh = skel->getBone(" L Thigh");*/
		
}

void npc_neutral::step(const Ogre::FrameEvent &evt)
{
	// If time stopped - NPC stopped
	if (TIME_SHIFT==0.0f)
		return;

	processAttachedObjects();
	processNoticeStep();

	Real mulTime = evt.timeSinceLastFrame*TIME_SHIFT*moveActivity*uniqueMult;

	time256+=mulTime;
	if (time256>256.0f)
		time256=0;

	// Animation in processing.
	//LogManager::getSingleton().logMessage("adsd");
	if (animated)
	{
		//#ifdef DEBUG_FACIAL
		//LogManager::getSingleton().logMessage("Animation add."+mAnimState->getAnimationName()+"fsd"+StringConverter::toString(mAnimState->getEnabled()));
		//#endif
		mAnimState->addTime(mulTime);
		processAnimationTransition(mulTime);
	}
	processLook(mulTime);
	
	//There begins active code.
	if (going)
	{
		//If false, no real activity
		bool farFindB = getpos(npcBody).distance(pPosit) < farFind;
		
		//If there is no any other processes, NPC stays on place.
		steps=Ogre::Vector3::ZERO;

		if (farFindB)
		fstTimer+=evt.timeSinceLastFrame*TIME_SHIFT*moveActivity;

		Vector3 myPos = mHeliNode->getPosition();
		if (fstTimer>0.5f)
		{
				fstindex++;
				
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

		//Recieved die flag
		if(dyeNow)
		{
			kill();
			return;
		}
		
		//Old feature
		
		
		//If in command mode, go to destination.
		if (cmdMode) 
		{
			pPosit=destCMD;
		}

		// If true - we are free. If false - we are doing previous task.
		bool path_find = !goal_achieved;
		//Very close to target position
		if (stopAtDist)
		{
			#ifdef DEBUG_TARGET_BLIZKO
						LogManager::getSingleton().logMessage("Distance to destination: "+StringConverter::toString(pPosit)+" is "+StringConverter::toString(getpos(npcBody).distance(pPosit)));
			#endif
			blizko = getpos(npcBody).distance(pPosit) < stopDist;
		}
		// Execute finish script.
		if (blizko)
		{
			#ifdef DEBUG_TARGET_BLIZKO
			LogManager::getSingleton().logMessage("NPC is near target position. Exec script.");
			#endif
			if (!(luaToExec.empty()))
				RunLuaScript(global::getSingleton().getLuaState(),luaToExec.c_str());
		}
		
		if (farFindB && goal_achieved) // We continue pathfinding because it finished the previous path.
		{
			path_find = find_path();
			i=0;
			if (path_find)
				goal_achieved=false;
		}
		else
		{
#ifdef DEBUG_PATHFINDING
	LogManager::getSingleton().logMessage("No need of pathfinding.");
	LogManager::getSingleton().logMessage(StringConverter::toString(farFindB)+
	" "+StringConverter::toString(goal_achieved)+
	" "+StringConverter::toString(path_find));
#endif
		}
		
		
		if (!notsetIdle&&!rotation_needed)
		{
							if (animated)
							{
								mAnimState->setEnabled(false); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
								mAnimState = heliEnt->getAnimationState("Walk");
								mAnimState->setEnabled(true);
								mAnimState->setWeight(1.0f);
							}
		}

		bool stra=check_straight();
		#ifdef DEBUG_PATHFINDING
		LogManager::getSingleton().logMessage("Check straight returned "+StringConverter::toString(stra));
		#endif

#ifdef STRAIGHT_BYPASS
		if (!goal_achieved && cmdMode && farFindB && path_find)
			stra=false;
#endif
		if ((farFindB && path_find && !stra)||(cmdMode && farFindB && path_find && !stra))
		{
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("Direction select cycle.");
			#endif
			previous_player=true;
			if (!dyeNow)
			{
			Vector3 dest=pPosit;

			if (mPath.size()>=2)
				dest = mPath[i]->getPosition();
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("GOAL:"+StringConverter::toString(dest));
			#endif
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
				//cmdMode=false;
				LogManager::getSingleton().logMessage("NPC finished path!");
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
			
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("rotation needed 1:");
			LogManager::getSingleton().logMessage(StringConverter::toString(rotation_needed));
			#endif
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
				steps=0;
				////LogManager::getSingleton().logMessage(StringConverter::toString(dir1)+" "+StringConverter::toString(dir)+" "+StringConverter::toString(angle));
				rotation_needed = Degree(fabs(angle.valueDegrees()))>=Degree(5.0f);
			}

			}
		}


		if (!path_find || (!goal_achieved && stra)) // Either path is found, and NPC is going on it, or goal is not achieved, and it is a straight path.
		{
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("Straight cycle.");
			#endif
			if (!dyeNow)
			{
				if (!farFindB)
				{
					//if (animated)
				//mAnimState->setEnabled(false);
					if (!notsetIdle)
					{
							if (animated)
							{
							//	mAnimState->setEnabled(false);
		    				//		mAnimState = heliEnt->getAnimationState("Idle1");
								//	mAnimState->setEnabled(true);
							}
					}
				}

				if (mProps.sounds)
				{
								if ((rand() % 20)==4)
									Run3SoundRuntime::getSingleton().emitSound(mProps.soundAttack,3,false,myPos,200,50,800);
			
								if ((rand() % 5)==4)
									Run3SoundRuntime::getSingleton().emitSound(mProps.soundHit,3,false,myPos,200,50,800);
				}

				Vector3 dest = Vector3(pPosit.x,0,pPosit.z);
				Vector3 dir = dest-Vector3(myPos.x,0,myPos.z);

				if (previous_player)
				{

					if (mProps.sounds)
						Run3SoundRuntime::getSingleton().emitSound(mProps.soundFind,6,false,myPos,200,50,800);
					previous_player=false;
				}

				if (farFindB) // NPC found player in near place
				{


							if (dir.length()<attackAnimDist)
							{
								//steps = dir.normalisedCopy();

								steps=Vector3::ZERO;
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
							#ifdef DEBUG_PATHFINDING
							LogManager::getSingleton().logMessage("rotation needed 2:");
							LogManager::getSingleton().logMessage(StringConverter::toString(rotation_needed));
							#endif
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
										niNode->rotate(Vector3::UNIT_Y,Degree(rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
									}
									else
									{
										////LogManager::getSingleton().logMessage(StringConverter::toString(angle)+"-");
										niNode->rotate(Vector3::UNIT_Y,Degree(-rotateSpeed*evt.timeSinceLastFrame*TIME_SHIFT));
									}
								}
							}
				}
			}
		}


		
	}
	/*#ifdef DEBUG_FACIAL
	LogManager::getSingleton().logMessage("neutral end step");
	#endif*/
} 



inline void npc_neutral::spawnNPCRagdoll(Vector3 pos)
{
			LogManager::getSingleton().logMessage("Ragdoll spawn!");
			mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
			mHeliNode->setScale(npc_scale);
			DotSceneLoader* dsl = LoadMap::getSingleton().dsl;
			dsl->util_processRagDoll(mName,"auto.xml",npc_mesh,mHeliNode);
			mHeliNode=0;  //Test for memory leak.
}

void npc_neutral::kill()
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

	if (mProps.ragdoll)
	{
		Vector3 pos = getpos(npcBody);
		dead=true;
		flush();
		spawnNPCRagdoll(pos);
	}
	else
	{
		if (animated)
		{
			mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState("Death1");
			mAnimState->setLoop(false);
			mAnimState->setEnabled(true);
		}
	}

	
}
bool npc_neutral::frameStarted(const Ogre::FrameEvent &evt)
{
	if (!dead)
		step(evt);
	if (animated&&mAnimState)
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