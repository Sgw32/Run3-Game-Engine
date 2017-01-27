#include "npc_neutral.h"
#include "PhysObject.h"



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
//mDebugLines=0;
}

npc_neutral::~npc_neutral()
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
if (mFac)
delete mFac;
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
//////////////////////////////////////////
void npc_neutral::teleport(Vector3 pos)
{
	npcBody->setPositionOrientation(pos,getorient(npcBody));
	//mCamNode->setPosition(pos);
	//mCamera->setPosition(Vector3(mCamera->getPosition().x,get_body_position(bod).y,mCamera->getPosition().z));
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

void npc_neutral::start()
{
going=true;
}

void npc_neutral::suspend()
{
going=false;
}

 void npc_neutral::resume()
{

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
  force=Ogre::Vector3(0,gravity,0); 
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
	   }
   if (!going && !dyeNow)
   {
	   if (!cmdMode)
	   {
			if (animated)
				mAnimState->setEnabled(false);
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


bool npc_neutral::isOnEarth()
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


void npc_neutral::processEvent(int flag,String param1,String param2)
{
	if (flag==SPAWN_NPC)
	{
		if (mProps.facial_animation)
			mFac=new FacialAnimation();
		uniqueMult = 0.9f + Math::RangeRandom(0,100)/500;
			mHeliNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mStartPos);

			if (mName=="")
			{

				mName=mHeliNode->getName();
			}
			//Facial animation
			if (mProps.facial_animation)
				heliEnt=mFac->preInit(npc_mesh,mName);
			else
				heliEnt=mSceneMgr->createEntity(mName,npc_mesh);
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

	if (flag==FACIAL_ACTIVITY)
	{
		if (mFac)
		{
			mFac->init(param1);
			mFac->animate(getpos(npcBody));
		}
	}

	if (flag==SETANIM_NPC)
	{
		if (animated)
		{
			mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState(param1);
			mAnimState->setEnabled(true);
			cmdMode=true;
			suspend();
		}
	}

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
	if (flag==TELEPORT_NPC)
	{
		teleport(StringConverter::parseVector3(param1));
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
			for (i=attachedPhysObjects.begin();i!=attachedPhysObjects.end();i++)
			{
				if ((*i)->getName()==param1)
				{
					attachedPhysObjects.erase(i);
					(*i)->detachFromBone();
				}

			}
	}
}

Vector3 npc_neutral::getpos(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
}

Quaternion npc_neutral::getorient(OgreNewt::Body* bod)
{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return rot;
}

void npc_neutral::transitFromCurrentAnimation(String newAnim)
{
	mTransitAnimState = heliEnt->getAnimationState(newAnim);
	mTransitAnimState->setWeight(0);
	mTransitAnimState->setEnabled(true);
	transitState=1.0f;
}

void npc_neutral::step(const Ogre::FrameEvent &evt)
{
	// If time stopped - NPC stopped
	for (int j=0;j!=attachedPhysObjects.size();j++)
	{
		attachedPhysObjects[j]->updateAttached();
	}

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
	Real mulTime = evt.timeSinceLastFrame*TIME_SHIFT*moveActivity*uniqueMult;
#ifdef DEBUG_FACIAL
	LogManager::getSingleton().logMessage("Facial start.");
#endif
	if (mFac)
	mFac->upd(evt.timeSinceLastFrame*TIME_SHIFT);
#ifdef DEBUG_FACIAL
	LogManager::getSingleton().logMessage("Facial upd ok.");
#endif
	time256+=mulTime;
	if (time256>256.0f)
		time256=0;

	if (TIME_SHIFT==0.0f)
		return;

	// Animation in processing.

	if (animated)
	{
#ifdef DEBUG_FACIAL
		LogManager::getSingleton().logMessage("Animation add.");
#endif
	mAnimState->addTime(mulTime);
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
		//mAnimState->setWeight(transitState);
		mAnimState->setWeight(1.0f);
		mAnimState->setEnabled(false);
		mTransitAnimState->setWeight(1.0f);
		mAnimState=mTransitAnimState;
		mTransitAnimState=0;
		transitState=1.0f;
	}

	}

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
		//else
		//{
			
		//}
		
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
				/*if (animated)
				{
				mAnimState->setEnabled(false);
				}*/
				/*
				if (!notsetIdle&&!rotation_needed)
				{
							if (animated)
							{
								mAnimState = heliEnt->getAnimationState("Walk");
								mAnimState->setEnabled(true);
							}
				}
				*/

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


		if (!path_find || (!goal_achieved && stra)) // Either path is found, and NPC is going on it, or goal is not achieved, and it is a straight path.
		{
			#ifdef DEBUG_PATHFINDING
			LogManager::getSingleton().logMessage("Straight cycle.");
			#endif
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
				//Vector3 myPos = mHeliNode->getPosition();
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

bool npc_neutral::find_path()
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

bool npc_neutral::find_path(Vector3 destPos)
{
	//LogManager::getSingleton().logMessage("pathfind");
	Vector3 myPos = getpos(npcBody);
	bool straight = check_straight(myPos,destPos)&&(!cmdMode); // if cmdmode - bypass this
	
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

bool npc_neutral::check_straight()
{
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

bool npc_neutral::check_straight(Vector3 n1,Vector3 n2)
{
	OgreNewt::BasicRaycast ray(mWorld,n1,n2);
	OgreNewt::Body* bod_t = ray.getFirstHit().mBody;
	if (bod_t) 
		return false; 
	return true;
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
			if (animated)
		{
	mAnimState->setEnabled(false);
			mAnimState = heliEnt->getAnimationState("Death1");
			mAnimState->setLoop(false);
			mAnimState->setEnabled(true);
		}
}
bool npc_neutral::frameStarted(const Ogre::FrameEvent &evt)
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