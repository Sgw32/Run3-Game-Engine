/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Player.h"
#include "global.h"
#include "Run3SoundRuntime.h"
#include "SuperFX.h"
#include "BloodEmitter.h"
#include "FadeListener.h"
#include "LoadMap.h"

#define MAX_JUMPTIME 0.15
#define MIN_JUMPPAUSE 0.1
//////////////////////////////////////////
Player::Player()
{
	initial=false;
	srand ( time(NULL) );
	razbros=0;
	st=-1.0f;
	tr_v=0;
	god=false;
	parentRelation=false;
	skippingCutScene=false;
	mFullP=false;
	mFootstep="tile";
	allowsubtitles=true;
	regen_Mul=1.0f;
	mLadder=false;
}
//////////////////////////////////////////
Player::~Player()
{
}
//////////////////////////////////////////
void Player::init(Ogre::SceneManager* scene,OgreNewt::World* world,Ogre::Camera* cam,SoundManager* soundMgr,Ogre::Root* _root)
{
		if( !scene )
	{
		throw Ogre::Exception( -1, "SceneMgr :  assertion failed at LoadMap::init","" );
	}
	mSceneMgr=scene;
		
	if( !world )
	{
		throw Ogre::Exception( -1, "World :  assertion failed at PhysObject::init","" );
	}
	mWorld=world;
	if( !cam )
	{
		throw Ogre::Exception( -1, "World :  assertion failed at PhysObject::init","" );
	}
	Real FOV;
	cf.load("run3/core/player.cfg",":",true);
	FOV=Ogre::StringConverter::parseReal(cf.getSetting("FOV","","90"));
	PlayerProps props;
	props.health=Ogre::StringConverter::parseReal(cf.getSetting("startHealth","","100"));
	props.elastic=Ogre::StringConverter::parseReal(cf.getSetting("elastic","","-1"));
	props.softeness=Ogre::StringConverter::parseReal(cf.getSetting("softness","","-1"));
	props.inertia_active=Ogre::StringConverter::parseBool(cf.getSetting("inertiaActive","","false"));
	props.kinematic_movement=Ogre::StringConverter::parseBool(cf.getSetting("kinematicMovement","","false"));
	props.upvectors=Ogre::StringConverter::parseBool(cf.getSetting("upVectors","","true"));
	props.splitvectors=Ogre::StringConverter::parseBool(cf.getSetting("splitVectors","","false"));
	props.dfoz=Ogre::StringConverter::parseBool(cf.getSetting("deactivateForcesOnZero","","false"));
	props.rotate_hor=Ogre::StringConverter::parseBool(cf.getSetting("rotateHotizon","","true"));
	props.resetVelocityOnRelease=Ogre::StringConverter::parseBool(cf.getSetting("resetVelocityOnRelease","","false"));
	props.runVel=Ogre::StringConverter::parseReal(cf.getSetting("runVel","","6"));
	props.stdVel=Ogre::StringConverter::parseReal(cf.getSetting("stdVel","","3"));
	props.flashConeI=Ogre::StringConverter::parseReal(cf.getSetting("flashConeI","","45"));
	props.flashConeU=Ogre::StringConverter::parseReal(cf.getSetting("flashConeO","","60"));
	props.range=Ogre::StringConverter::parseReal(cf.getSetting("Range","","2000"));
	props.stairEffect=Ogre::StringConverter::parseReal(cf.getSetting("stairEffect","","-1"));
	props.trainForce=Ogre::StringConverter::parseReal(cf.getSetting("trainForce","","400"));
	props.nearClip=Ogre::StringConverter::parseReal(cf.getSetting("nearClip","","10"));
	regen_Time=Ogre::StringConverter::parseReal(cf.getSetting("stdVel","","200"));
	allowsubtitles=props.allowsubtitles = Ogre::StringConverter::parseBool(cf.getSetting("allowSubtitles","","true"));
	props.allowflashlight=Ogre::StringConverter::parseBool(cf.getSetting("allowFlashLight","","true"));
	props.fov=FOV;
	stop_count=0;
	stop_count_std=Ogre::StringConverter::parseReal(cf.getSetting("stop_count_std","","1"));
	mProps=props;
	//props.mPlayerName=
	sound=soundMgr;
	fstindex=0;
	root=_root;
//	Ogre::Entity* ent;
	//Ogre::SceneNode* wtestnode;
	fstTimer=0.0f;
	mCamera=cam;
	mCamera->setPosition(0,0,0);
	mCamera->setNearClipDistance(props.nearClip);
	mProps.farClip = mCamera->getFarClipDistance();
	LogManager::getSingleton().logMessage("FarClip is:"+StringConverter::toString(mProps.farClip));
	mCamera->setFOVy(Degree(FOV));
	mCamNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mCamNode->setPosition(Vector3(0,100,500));
	mViewNode = mCamNode->createChildSceneNode();
	mNoclipNode = mCamNode->createChildSceneNode();
	mViewNode->attachObject(mCamera);
	mDirection = Vector3::ZERO;
	reset_vel=false;
	noclip=false;  
	awaitingunduck=false;
	no_movement=true;
	mRotate = Ogre::StringConverter::parseReal(cf.getSetting("mouseSensivity","","0.26"));
	mMove = 20;
	gravity = -GRAVITY;
	y_rotation_cont = 0;
    y_limit_a = 90;
    y_limit_b = -90;
	jump_power = 100;
	mForce = Vector3::ZERO;
	fps_speed = props.stdVel;
	debug=0;
	dump=true;
	front_jump=false;
	strafe_jump=false;
	m_FrontVelocity = 0;
	m_StrafeVelocity = 0;
	camera_rotation_x = 0;
	size = Vector3(20,100,20);
	size_dc = Vector3(20,50,20);
	mCamNode->setScale(size);
	mViewNode->setPosition(Vector3(0,0.5,0));
	col = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size);
	duckcol = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size_dc);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode(mCamNode);
	bod->setType(PHYSOBJECT_PLAYER);
	bod->setPositionOrientation( mCamNode->getPosition(), Ogre::Quaternion::IDENTITY );
	inertia=Ogre::StringConverter::parseVector3(cf.getSetting("inertia","","-1 -1 -1"));
	if (inertia==Vector3(-1,-1,-1))
		inertia = OgreNewt::MomentOfInertia::CalcEllipsoidSolid(Ogre::StringConverter::parseReal(cf.getSetting("mass","","40")),Ogre::StringConverter::parseVector3(cf.getSetting("sizeexp","","50 100 50")));
	bod->setMassMatrix( 40, inertia );
	bod->setCustomForceAndTorqueCallback<Player>( &Player::camera_force_callback ,this); // add the previous defined callback function as the body custom force and torque callback
	//bod->setStandardForceCallback();
	bod->setAutoFreeze(0);
	OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Y);
	/*uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_X);
	uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Z);*/
	uv1=uv3=0;
	if (props.upvectors)
	{
	uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_X);
	uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Z);
	}

	jump=false;
	bod->setOmega(Vector3(0,0,0));   //no rotation,for now  
	textbox=OverlayManager::getSingleton().createOverlayElement("TextArea","CameraText");
	textbox->setCaption("Fps Camera");
	textbox->setMetricsMode(GMM_RELATIVE);
	textbox->setPosition(0,0);
	textbox->setParameter("font_name","Console2");
	textbox->setParameter("colour_top","1 1 1");
	textbox->setParameter("colour_bottom","1 1 1");
	textbox->setParameter("char_height","0.02");
	HUD::getSingleton().Hide_crosshair();
	debugOverlay=OverlayManager::getSingleton().create("FPSCamDebug");
	debugOverlay->add2D((OverlayContainer*)textbox);
	debugOverlay->show();
	initial=true;
	cur_size=size;
	alive=true;
	jumping = false;
	jumpTime = 0;
	jumpLandedTime = 0;
	jump = false;
	jumpStartFinished = true;
	distToFloor=0;
	bod->setName("PLAYER1");
	bod->setType(NEWTBODYTYPE_PLAYER);
	bod->setCenterOfMass(Vector3(0,0,0));
	bod->reactByRot=true;
	mMaxHealth=100;
	health=mMaxHealth;
	unpressed=false;
	/*int i;
	for (i=1;i!=4;i++)
	{
		unsigned int audioId;
		String fname="";
		fname = fname + "run3/sounds/footsteps/"+"concrete"+StringConverter::toString(i)+".wav";
		soundMgr->loadAudio(fname,&audioId,false);
		soundMgr->setSound(audioId,Vector3(0,0,0),Vector3(0,0,0),Vector3(0,0,0),1000,false,true,1000,1000,1000);
		footsteps.push_back(audioId);
		fname.clear();
	}*/
	mPhysCallback = new PlayerContactCallback();
  mMatDefault = mWorld->getDefaultMaterialID();
  playerMat= new OgreNewt::MaterialID( mWorld );
//  playerMat
  mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, playerMat );
  mMatPair->setContactCallback( mPhysCallback );
  //mPhysCallback->setContactKineticFrictionCoef(0,playerMat->getID());
  if (Ogre::StringConverter::parseBool(cf.getSetting("frictionSet","","false")))
  {
	  LogManager::getSingleton().logMessage("Friction set!");
	 mMatPair->setDefaultFriction(Ogre::StringConverter::parseReal(cf.getSetting("stat","","0")),Ogre::StringConverter::parseReal(cf.getSetting("kinetic","","0")));
  }
  if (props.softeness!=-1.0f)
  {
  mMatPair->setDefaultSoftness(props.softeness);
	
  }
  if (props.elastic!=-1.0f)
  {
  mMatPair->setDefaultElasticity(props.elastic);
	
  }
 // mMatPair->
  bod->setMaterialGroupID(playerMat);
 /* if !(props.inertia_active)
	  mMatPair->setDefaultFriction(0.1*/
	///////Weapons setup/////
	/*ent= mSceneMgr->createEntity( "hands_v", "minigun_v.mesh" );
	wtestnode=mViewNode->createChildSceneNode(Vector3(0,0,0));
	wtestnode->attachObject(ent);
	wtestnode->setScale(0.2,0.1,0.2);
	wtestnode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
	wtestnode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));*/
	CWeapon::getSingleton().init(mSceneMgr,mWorld,mCamera,sound,mViewNode,root,sound);
	freezed = false;
	//flashlight section
	flashLight=0;
	/*flashLight = mSceneMgr->createLight("flashLight");

	
    flashLight->setDiffuseColour(Ogre::ColourValue(1, 1, 1));
    flashLight->setSpecularColour(Ogre::ColourValue(1, 1, 1));
  flashLight->setType(Ogre::Light::LT_SPOTLIGHT);
    flashLight->setSpotlightInnerAngle(Ogre::Degree(25));
    flashLight->setSpotlightOuterAngle(Ogre::Degree(45));
    flashLight->setAttenuation(1000, 1, 1, 1); // meter range.  the others our shader ignores
    flashLight->setDirection(Ogre::Vector3(0, 0, -1));
	mViewNode->attachObject(flashLight);
	flashLight->setVisible(true);*/
	LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLight"))+"PLAYER!!!");
	MaterialManager::getSingleton().load("Materials/BloodMaterial","General");
	LogManager::getSingleton().logMessage("Player initialized!");
}

void Player::setAllowSubtitles(bool enable)
{
	allowsubtitles=enable;
	LogManager::getSingleton().logMessage("Now allowsubtitles is:"+StringConverter::toString(allowsubtitles));
	String data = StringConverter::toString(allowsubtitles);
	String nam = "allowSubtitles";
	cf.setSetting(nam,data);
	cf.save();
}

bool Player::getAllowSubtitles()
{
	return allowsubtitles;
}


void Player::shockPlayerCutScene()
{
	if (!Sequence::getSingleton().hasCutScene("shock"))
		Sequence::getSingleton().SetSceneSeq("run3/game/shock.xml");

	SequenceLuaCallback::getSingleton().startCutScene("shock");
//	return out;
}

void Player::toggleFlashLight()
{
		if (mProps.allowflashlight)
		{
		/*if(flashLight)
		{*/
		/*LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLight"))+"PLAYER!!!");
			if (mSceneMgr->hasLight("flashLight"))
			{
			flashLight=mSceneMgr->getLight("flashLight");
			bool vis = flashLight->getVisible();
			flashLight->setVisible(!vis);
			}

		/*}*/
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/flash01.wav",2.0f,false);
		if (flashLight&&(mSceneMgr->hasLight("flashLighter")))
		{

			mViewNode->detachObject(flashLight);
			LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLighter"))+"PLAYERB!!!");
			if (flashLight)
				mSceneMgr->destroyLight(flashLight);
			flashLight=0;
		}
		else
		{
			if (mSceneMgr->hasLight("flashLighter"))
			{
			flashLight=mSceneMgr->getLight("flashLighter");
			}
			else
			{
			flashLight = mSceneMgr->createLight("flashLighter");
			}
	
			flashLight->setDiffuseColour(Ogre::ColourValue(0.5, 0.5, 0.5));
			flashLight->setPosition(0,0,-20);
			flashLight->setSpecularColour(Ogre::ColourValue(0,0,0));
			flashLight->setType(Ogre::Light::LT_SPOTLIGHT);
			flashLight->setSpotlightInnerAngle(Ogre::Degree(mProps.flashConeI));
			flashLight->setSpotlightOuterAngle(Ogre::Degree(mProps.flashConeU));
			flashLight->setAttenuation(mProps.range, 1, 1, 1); // meter range.  the others our shader ignores
			flashLight->setDirection(Ogre::Vector3(0, 0, -1));
			mViewNode->attachObject(flashLight);
			//flashLight->setCastShadows(false);
			flashLight->setVisible(true);
		}
		}
}


void Player::multiply(Real multyplier)
{
	/*delete bod;
	size = Vector3(50,100,50)*multyplier;
	size_dc = Vector3(50,50,50)*multyplier;
	mCamNode->setScale(size);
	col = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size);
	duckcol = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size_dc);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode(mCamNode);
	bod->setPositionOrientation( mCamNode->getPosition(), Ogre::Quaternion::IDENTITY );
	inertia = OgreNewt::MomentOfInertia::CalcEllipsoidSolid(40,size);
	bod->setMassMatrix( 40, inertia );
	bod->setCustomForceAndTorqueCallback<Player>( &Player::camera_force_callback ,this); // add the previous defined callback function as the body custom force and torque callback
	//bod->setStandardForceCallback();
	bod->setAutoFreeze(0);
	OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Y);
	bod->setOmega(Vector3(0,0,0));   //no rotation,for now  */
	size = Vector3(20,100,20)*multyplier;
	size_dc = Vector3(20,50,20)*multyplier;
	mCamNode->setScale(size);
	col = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size);
	duckcol = new OgreNewt::CollisionPrimitives::Ellipsoid(mWorld, size_dc);
	bod->setCollision(col);
	jump_power=100*multyplier;
	rot3 = Quaternion::IDENTITY;
	//inertia = OgreNewt::MomentOfInertia::CalcEllipsoidSolid(40,size);
	//bod->setMassMatrix( 40, inertia );
}
//////////////////////////////////////////
Ogre::Camera* Player::get_camera()
{
	return mCamera;
}
void Player::shutdown()
{
	if (!initial)
		return;
   mCamNode->detachAllObjects();
   debugOverlay->remove2D((OverlayContainer*)textbox);
   debugOverlay->hide();
   debugOverlay->clear();
}
//////////////////////////////////////////
OgreNewt::Body* Player::get_ray_shoot()
{
Ogre::Vector3 myPos = get_location()+Vector3(0,cur_size.y*mViewNode->getPosition().y,0);
Ogre::Vector3 direction,end;
direction = get_direction();
end=myPos + direction * 300.0f;
OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
return shootRay.getFirstHit().mBody;
}
//////////////////////////////////////////////////////////
Ogre::Vector3 Player::get_direction()
{
/*Ogre::Quaternion myOrient = get_orientation();
Ogre::Quaternion myOrient2 = mViewNode->_getDerivedOrientation();
Ogre::Vector3 direction2,down,down2,direction;
direction2 = myOrient2*Vector3::NEGATIVE_UNIT_Z;
down = Vector3(0,direction2.y,0);
direction = myOrient*Vector3::NEGATIVE_UNIT_Z+down;*/
//direction = myOrient*Vector3::NEGATIVE_UNIT_Z;
	Ogre::Quaternion myOrient2 = mViewNode->_getDerivedOrientation();
return myOrient2*Vector3::NEGATIVE_UNIT_Z;
}
//////////////////////////////////////////
void Player::processUse()
{
	OgreNewt::Body* bod;
	bod=0;
	bod=get_ray_shoot();
	if (bod)
	{
		bod->use();
	}
}
//////////////////////////////////////////
void Player::show_overlay()
{
	if (!HUD::getSingleton().IsVisibleCross())
		HUD::getSingleton().Show_crosshair();
}
//////////////////////////////////////////
void Player::hide_overlay()
{
	if (HUD::getSingleton().IsVisibleCross())
		HUD::getSingleton().Hide_crosshair();
}
//////////////////////////////////////////

void Player::set_position(Vector3 pos)
{
	bod->setPositionOrientation(pos,get_body_orientation(bod));
	//mCamNode->setPosition(pos);
	//mCamera->setPosition(Vector3(mCamera->getPosition().x,get_body_position(bod).y,mCamera->getPosition().z));
}
//////////////////////////////////////////

void Player::set_additional_force(Vector3 force)
{
	mForce = force;
}

void Player::remove_additional_force()
{
	mForce = Vector3::ZERO;
}

void Player::teleport(Vector3 pos)
{
	bod->setPositionOrientation(pos,get_body_orientation(bod));
	//mCamNode->setPosition(pos);
	//mCamera->setPosition(Vector3(mCamera->getPosition().x,get_body_position(bod).y,mCamera->getPosition().z));
}
//////////////////////////////////////////	
void Player::freeze()
{
	bod->freeze();
	freezed = true;
	//mViewNode->setOrientation(Quaternion::IDENTITY);
}
//////////////////////////////////////////
void Player::unfreeze()
{
	bod->unFreeze();
	freezed = false;
	mViewNode->setOrientation(Quaternion::IDENTITY);
}
//////////////////////////////////////////
void Player::unfreeze2()
{
	bod->unFreeze();
	freezed = false;
	//mViewNode->setOrientation(Quaternion::IDENTITY);
}
//////////////////////////////////////////
void Player::look_at(Vector3 pos)
{
	mViewNode->lookAt(pos,Node::TransformSpace::TS_PARENT);
	rot3 = get_orientation();
}
//////////////////////////////////////////
void Player::camera_force_callback( OgreNewt::Body* me ) 
   { 
	//LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLight"))+"PLAYER!!!");
	   if (parentRelation)
		{
			if (mFullP)
			{
			me->setPositionOrientation(mPar->getPosition()+Quaternion(Radian(Degree(180)),Vector3::UNIT_Y)*mPar->getOrientation()*pRelPosition,Quaternion(Radian(Degree(x_rotation_cont)),Vector3::UNIT_Y));
			}
			else
			{
				me->setPositionOrientation(mPar->getPosition()+pRelPosition,Quaternion(Radian(Degree(x_rotation_cont)),Vector3::UNIT_Y));
			}
		}
	   else
	   {
   Player *camPlayer = (Player*)me->getUserData(); 
   Ogre::Real mass;
   Ogre::Vector3 inertia;
    me->setTorque(Vector3(0,0,0));
	me->setOmega(Vector3(0,0,0));

   me->getMassMatrix(mass, inertia);
   bool onEarth=isOnEarth();
   if (!onEarth)
   {
   force=Vector3(0, -gravity,0);
   force2=Vector3(0,-gravity,0);
   }
   else
   {
	   force=force2=Vector3::ZERO;
   }
   /*force=Vector3(0, 0,0);
   force2=Vector3(0,0,0);*/
   
   Ogre::Quaternion q;
   Ogre::Vector3 qq,s;
   q=Quaternion::IDENTITY;
   Vector3 stdAddForce = me->getStdAddForce()*60000*(y_rotation_cont+200)/290;
   me->setStandartAddForce(Vector3::ZERO);

   if (front_jump && onEarth)
   {
	   front_jump=false;
		m_FrontVelocity*=6;
   }
   if (strafe_jump && onEarth)
   {
	   strafe_jump=false;
		m_StrafeVelocity*=6;
   }
  /* qq=Ogre::Vector3::ZERO;
   s=Ogre::Vector3::ZERO;*/
	/*Vector3 av = checkNotAvailable();
	if (av!=Vector3::ZERO)
	{
		if (av.x=-1 && m_StrafeVelocity.x<=0)
			m_StrafeVelocity.x=0;
		if (av.x=1 && m_StrafeVelocity.x>=0)
			m_StrafeVelocity.x=0;
		if (av.z=-1 && m_FrontVelocity.z<=0)
			m_FrontVelocity.z=0;
		if (av.z=1 && m_FrontVelocity.z>=0)
			m_FrontVelocity.z=0;
	}*/
   direction = (get_body_orientation(me) *  Vector3::NEGATIVE_UNIT_Z);
   poo2=(get_body_orientation(me) * Vector3::NEGATIVE_UNIT_X);//for strafe
   qq = direction;
   s = poo2;//for strafe
   
   Ogre::Vector3 V0 = me->getVelocity();
   Ogre::Vector3 V1(qq * m_FrontVelocity * fps_speed);
   Ogre::Vector3 V2 (s * m_StrafeVelocity * fps_speed);//for  strafe
if (((m_FrontVelocity!=0)||(m_StrafeVelocity!=0))&&(mProps.stairEffect!=-1))
	{
		Vector3 pos = get_location();
		Vector3 dir = (V1+V2).normalisedCopy();
		if (checkStairEffect(pos,dir))
		{
			V1*=2;
			V2*=2;
			//teleport(pos+Vector3(0,mProps.stairEffect,0));
		}
	}
	

   /*if (m_FrontVelocity!=0)
   {
    force.x = acel.x;
   //force.y = acel.y;
    force.z = acel.z;
   }
   else
   {
	   force.x=force.z=0;
   }
   if (m_StrafeVelocity!=0)
   {
    force2.x = acel2.x;//for strafe
    force2.z = acel2.z;//for strafe
   }
   else
   {
		force2.x=force2.z=0;
   }*/
 


	if(jumping && !jump)
    {
          jumpStartFinished = true;
    }     
    if(jump && !jumping && isOnEarth())
    {
          //preparing to jump
          jumpLanded = false;
          jumping  = true;
          jumpTime = 0;
          jumpLandedTime = 0;
          jumpStartFinished = false;
    }
	Ogre::Real timeStep = mWorld->getTimeStep();
	Ogre::Vector3 acel = (V1 - V0);
Ogre::Vector3 acel2 = (V2 - V0);//for strafe
   acel = acel * 5.0f; 
   acel2 = acel2 * 5.0f;

force.z = acel.z;
force.x = acel.x;
force2.x = acel2.x;//for strafe
force2.z = acel2.z;//for strafe

	/**/
/*Vector3 av = checkNotAvailable();
	if (av.x==-1)
			force.x=0;
	if (av.x==1)
			force2.x=0;
	if (av.z==-1)
			force.z=0;
	if (av.z==1)
			force2.z=0;*/
	if(jumping)//in the jump
		{
			
	         /* V1=V1/100;
			  V2=V2/100;*/

			/*if (av.x==1&&KEY_DOWNR(OIS::KC_D))
				{
					m_StrafeVelocity = -5*mMove;
					strafe_jump=true;
				}
				if (av.z==1&&KEY_DOWNR(OIS::KC_S))
				{
					m_FrontVelocity = 5*mMove;
					strafe_jump=true;
				}
				if (av.x==-1&&KEY_DOWNR(OIS::KC_A))
				{
					m_StrafeVelocity = 5*mMove;
					front_jump=true;
				}
				if (av.z==-1&&KEY_DOWNR(OIS::KC_W))
				{
					m_FrontVelocity = 5*mMove;
					front_jump=true;
				}*/
//LogManager::getSingleton().logMessage(StringConverter::toString(av));
		
Vector3 av = checkNotAvailable();
	if (av.x!=0)
			force.x=force2.x=0;
		if (av.z!=0)
			force.z=force2.z=0;
			jumpTime += timeStep;
	          
			if(jumpTime <= MAX_JUMPTIME)
			{
				if(!jumpStartFinished)
				{
					//we only add jumpPower in the MAX_JUMPTIME time intervall
	                
				force.y = gravity*6;
	            
				}
			}
			else
			{
					//now time is up, and if we are not on earth anymore, then the start sequence is finished
				if(!isOnEarth())
					jumpStartFinished = true;
	             
			}

	          
			if(isOnEarth() && jumpStartFinished && jumping)//means we landed, but time is not up yet
			{
				if(!jumpLanded)
				{                    
					Run3SoundRuntime::getSingleton().emitSound("run3/sounds/land.wav",0.7f,false);
					jumpLanded = true;
				}
				jumpLandedTime += timeStep;
	             
				if(jumpLandedTime >= MIN_JUMPPAUSE)//you need a little time to rest after the jump. otherwise you can use jumping to accelerate, which might not be so good
				{
					//reset everything
					jumping = false;             
				}
			}
		}


	////
    /*if(jump == true)
    {
		 if (isOnEarth())
         force.y = 20*this->jump_power;
	}*/
	
   force *= mass;

   force2 *= mass;
   //me->addForce( mForce );
   if (mForce!=Vector3::ZERO)
   {
	   if (!force.y<0)
			force.y = force.y+mForce.y*20;
	   if (!force.y>0)
		    force.y = mForce.y*20;
   }
	if (mProps.kinematic_movement)
	{
   if (alive && isOnEarth())
   {
   /*me->addForce( force );
   me->addForce( force2 );*/
	    if (!jumping)
		me->setVelocity(Vector3((force+force2).x,0,(force+force2).z)/100);
		}
		else
		{
			me->addForce(Vector3(0,-gravity,0)*mass);
		}
	}
	else
	{

				if (alive)
				{
					/*if ( (m_FrontVelocity==0) && (m_StrafeVelocity==0))
					{
						force.x=force2.x=0;
						force.z=force2.z=0;
						//me->addForce(Vector3(0,-gravity,0)*mass);
					}*/
						

					if (mProps.splitvectors)
					{
					if ( ( !(m_FrontVelocity==0) && (m_StrafeVelocity==0)) || ( (m_FrontVelocity==0) && !(m_StrafeVelocity==0)))
					{
						if (mProps.dfoz && (force==Vector3::ZERO))
						{
							force=Vector3::ZERO;
							LogManager::getSingleton().logMessage("DFOZ!!!FRONT");
						}
						if (mProps.dfoz && (force2==Vector3::ZERO))
						{
							force2=Vector3::ZERO;
							LogManager::getSingleton().logMessage("DFOZ!!!STRAFE");
						}
						me->addForce( force+force2+stdAddForce  );
						//me->addForce( force2 );
					}
					else
					{
							//LogManager::getSingleton().logMessage("BOTH WORKING!!! SPLITTED..");
							me->addForce( (force+force2)/2+stdAddForce );
							
						//	me->addForce( force2/2 );
					}
					}
					else
					{
						me->setForce( force+force2+stdAddForce  );
						//me->addForce( force2 );
					}
				}
				else
				{
					me->addForce(Vector3(0,-gravity,0)*mass);
				}
	}

   if (!alive)
   {
	   if (isOnEarth())
		{
				bod->setVelocity(Vector3(0,0,0));
		}
   }
   	/*if ((m_FrontVelocity==Ogre::Vector3::ZERO)&&(m_StrafeVelocity==Ogre::Vector3::ZERO))
	{
		me->setVelocity(Vector3::ZERO);
	}*/
   //me->setVelocity(force+force2);
	//me-ö>addForce(Vector3(0,-gravity,0)*mass);
   //me->setPositionOrientation(get_body_position(bod), Quaternion(get_body_orientation(bod).w,mViewNode->getOrientation().x,get_body_orientation(bod).y,mViewNode->getOrientation().z));
//me->setOmega(Vector3(0,camera_rotation_x*2,0));   //to rotate camera in x
//LogManager::getSingleton().logMessage(StringConverter::toString(me->getOmega())+" "+StringConverter::toString(Vector3(0,camera_rotation_x*2,0)));*/
 
  Real y = camera_rotation_x*2;
   rot3=rot3*Quaternion(Degree(y),Vector3::UNIT_Y);

   OgreNewt::Body* b2 = getBodyOnBottom();
   if (b2)
   bod->addForce(Vector3( b2->getVelocity().x*mProps.trainForce,0,b2->getVelocity().z*mProps.trainForce));
	//Real y = get_orientation().getYaw().valueAngleUnits()+camera_rotation_x*2;*/
   
	if (!global::getSingleton().computer_mode)
	{
		/*if (parentRelation)
		{
			me->setPositionOrientation(mPar->getPosition()+pRelPosition,Quaternion(Radian(Degree(x_rotation_cont)),Vector3::UNIT_Y));
		}
		else
		{*/
			me->setPositionOrientation(get_location(),Quaternion(Radian(Degree(x_rotation_cont)),Vector3::UNIT_Y));
		//}
	}
	}
mWorld->setGlobalActorPosition(get_body_position(bod));
camera_rotation_x=0;
   
}
//////////////////////////////////////////
void Player::cnoclip()
{
	noclip=!noclip;
	if (noclip)
	{
		mCamera->detatchFromParent();
		mNoclipNode->attachObject(mCamera);
		mNoclipNode->setOrientation(mViewNode->getOrientation());
		bod->freeze();
	}
	if (!noclip)
	{
		mCamera->detatchFromParent();
		mViewNode->attachObject(mCamera);
		bod->unFreeze();
	}
}

bool Player::isTop()
{
Ogre::Vector3 myPos = get_location();

   
   OgreNewt::BasicRaycast floorRay(mWorld,myPos,myPos+Ogre::Vector3::UNIT_Y * jump_power);
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
         if(found.mBody != bod && found.mDistance < hit.mDistance)
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
      distToFloor -= cur_size.y;// remove char's height; 
      
        
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
//////////////////////////////////////////
bool Player::isOnEarth()
{
        
    //this part checks distance to ceiling for unducking
   Ogre::Vector3 myPos = get_location();

   
   OgreNewt::BasicRaycast floorRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Y * jump_power);
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
         if(found.mBody != bod && found.mDistance < hit.mDistance)
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
      distToFloor -= cur_size.y;// remove char's height; 
      
        
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
//////////////////////////////////////////
Ogre::Quaternion Player::get_body_orientation(OgreNewt::Body* bod)
{
   
   Quaternion orient;
   Vector3 pos;

   bod->getPositionOrientation(pos, orient);

   return orient; 

}
//////////////////////////////////////////
Vector3 Player::checkNotAvailable(void)
{
Ogre::Vector3 myPos = get_location();
Ogre::Vector3 res = Vector3::ZERO;
OgreNewt::BasicRaycast floorRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_X * jump_power);
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
         if(found.mBody != bod && found.mDistance < hit.mDistance)
            {
                //if the body I found is not my own, and is not water and is closer than last result
                //you will probably have a different water system than me...
                hit = found;
                //break;
            }
        }
        if(hit.mBody)
		{
      distToFloor = hit.mDistance * 10; // calculale the distance to the floor
      distToFloor -= cur_size.y;// remove char's height; 
      
        
      if(distToFloor <= 0.05) //this much over the floor is considered on the floor
      {
         res.x--;
      }
		}
   } 

  floorRay=OgreNewt::BasicRaycast(mWorld,myPos,myPos+Ogre::Vector3::UNIT_X * jump_power);
  hits=floorRay.getHitCount();
   if (hits > 0)
   {           
        OgreNewt::BasicRaycast::BasicRaycastInfo hit;
        hit.mBody = NULL;
      hit.mDistance = 500;
        for(int i=0;i<floorRay.getHitCount();i++)
        {
            OgreNewt::BasicRaycast::BasicRaycastInfo found = floorRay.getInfoAt(i);
         if(found.mBody != bod && found.mDistance < hit.mDistance)
            {
                //if the body I found is not my own, and is not water and is closer than last result
                //you will probably have a different water system than me...
                hit = found;
                //break;
            }
        }
        if(hit.mBody)
		{
           
      distToFloor = hit.mDistance * 10; // calculale the distance to the floor
      distToFloor -= cur_size.y;// remove char's height; 
      
        
      if(distToFloor <= 0.05) //this much over the floor is considered on the floor
      {
         res.x++;
      }
		}
   } 


floorRay=OgreNewt::BasicRaycast(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Z * jump_power);

	hits=floorRay.getHitCount();
   if (hits > 0)
   {           
        OgreNewt::BasicRaycast::BasicRaycastInfo hit;
        hit.mBody = NULL;
      hit.mDistance = 500;
        for(int i=0;i<floorRay.getHitCount();i++)
        {
            OgreNewt::BasicRaycast::BasicRaycastInfo found = floorRay.getInfoAt(i);
         if(found.mBody != bod && found.mDistance < hit.mDistance)
            {
                //if the body I found is not my own, and is not water and is closer than last result
                //you will probably have a different water system than me...
                hit = found;
                //break;
            }
        }
        if(hit.mBody)
		{
      distToFloor = hit.mDistance * 10; // calculale the distance to the floor
      distToFloor -= cur_size.y;// remove char's height; 
      
        
      if(distToFloor <= 0.05) //this much over the floor is considered on the floor
      {
         res.z--;
      }
		}
   } 

  floorRay=OgreNewt::BasicRaycast(mWorld,myPos,myPos+Ogre::Vector3::UNIT_Z * jump_power);
  hits=floorRay.getHitCount();
   if (hits > 0)
   {           
        OgreNewt::BasicRaycast::BasicRaycastInfo hit;
        hit.mBody = NULL;
      hit.mDistance = 500;
        for(int i=0;i<floorRay.getHitCount();i++)
        {
            OgreNewt::BasicRaycast::BasicRaycastInfo found = floorRay.getInfoAt(i);
         if(found.mBody != bod && found.mDistance < hit.mDistance)
            {
                //if the body I found is not my own, and is not water and is closer than last result
                //you will probably have a different water system than me...
                hit = found;
                //break;
            }
        }
        if(hit.mBody)
		{
           
      distToFloor = hit.mDistance * 10; // calculale the distance to the floor
      distToFloor -= cur_size.y;// remove char's height; 
      
        
      if(distToFloor <= 0.05) //this much over the floor is considered on the floor
      {
         res.z++;
      }
		}
   } 
return res;
}
//////////////////////////////////////////
Ogre::Vector3 Player::get_body_position(OgreNewt::Body* bod)
{
   
   Quaternion orient;
   Vector3 pos;

   bod->getPositionOrientation(pos, orient);

   return pos; 
   
}
//////////////////////////////////////////
Ogre::Vector3 Player::get_location()
{
	return get_body_position(bod);
}
//////////////////////////////////////////
Ogre::Quaternion Player::get_orientation()
{
return get_body_orientation(bod);
}
//////////////////////////////////////////
std::string Player::_tostr(float a)
{
    std::ostringstream stream;
    stream.precision(6);
    stream.width(0);
    stream.fill(((char)" "));
    stream << a;
    return stream.str();
}
//////////////////////////////////////////
void Player::detatch_camera(bool freeze)
{
	mCamera->detatchFromParent();
	if (freeze)
	{
		freezed = true;
		bod->freeze();
	}
}
//////////////////////////////////////////
void Player::attach_camera(bool unfreeze)
{
	//if (mViewNode->
	mViewNode->attachObject(mCamera);
	if (unfreeze)
	{
		freezed = false;
		bod->unFreeze();
	}
}
//////////////////////////////////////////
void Player::FCPress(const OIS::KeyEvent &arg)
{
	if(noclip)
	{
			switch (arg.key)
			{
			    case OIS::KC_UP:
				case OIS::KC_W:
			   	    mDirection.z = -30;
					break;

				case OIS::KC_DOWN:
			    case OIS::KC_S:
				    mDirection.z = 30;
					break;

				case OIS::KC_LEFT:
				case OIS::KC_A:
					mDirection.x = -30;
					break;

				case OIS::KC_RIGHT:
				case OIS::KC_D:
					mDirection.x = 30;
					break;

				default:
					break;
			 }
	}
	else
	{
		//FPS Mode
		bool onEarth= isOnEarth();
		
			switch (arg.key)
			{
				case OIS::KC_LSHIFT:
				   fps_speed=mProps.runVel;	
				   SuperFX::getSingleton().toggleMotionBlur();
				   break;
				case OIS::KC_SPACE:
				   this->jump = true;
				   skippingCutScene=true;
				  
			/*if (KEY_DOWNR(OIS::KC_W)||KEY_DOWNR(OIS::KC_A)||KEY_DOWNR(OIS::KC_S)||KEY_DOWNR(OIS::KC_D))
			{	*/
				
				
				   reset_vel=false;
				   unpressed=false;
				   break;

			    case OIS::KC_UP:
				case OIS::KC_W:
				if (onEarth&&!front_jump)
				{
			   	    m_FrontVelocity = 30*mMove;
								
					bod->setVelocity(Vector3(0,0,0));
				}			
				else
				{
					 m_FrontVelocity = 5*mMove;
					 front_jump=true;
				}
				stop_count=2;
				reset_vel=false;
				 unpressed=false;
					break;

				case OIS::KC_DOWN:
			    case OIS::KC_S:
				if (onEarth&&!front_jump)
				{
					m_FrontVelocity = -30*mMove;
					bod->setVelocity(Vector3(0,0,0));
				}
				else
				{
					m_FrontVelocity = -5*mMove;
					front_jump=true;
				}
				reset_vel=false;
				 unpressed=false;
					break;

				case OIS::KC_LEFT:
				case OIS::KC_A:
				if (onEarth&&!strafe_jump)
				{
					m_StrafeVelocity = 30*mMove;			
					bod->setVelocity(Vector3(0,0,0));
				}		
				else
				{
					m_StrafeVelocity = 5*mMove;	
					strafe_jump=true;
				}
					if (mProps.rotate_hor)
					mViewNode->roll(Degree(-1));
					reset_vel=false;
					 unpressed=false;
					break;

				case OIS::KC_RIGHT:
				case OIS::KC_D:
					if (onEarth&&!strafe_jump)
					{
					m_StrafeVelocity = -30*mMove;
					bod->setVelocity(Vector3(0,0,0));
					}
					else
					{
					m_StrafeVelocity = -5*mMove;
					strafe_jump=true;
					}
				reset_vel=false;
				 unpressed=false;
					if (mProps.rotate_hor)
					mViewNode->roll(Degree(1));
					break;
				case OIS::KC_LCONTROL:
				case OIS::KC_RCONTROL:
					if (alive)				
					Duck();
					break;

				case OIS::KC_O:
				//	test=!test;
					if (dump)
					{
					/*OgreConsole::getSingleton().print(debugstr1);
					OgreConsole::getSingleton().print(debugstr2);
					OgreConsole::getSingleton().print(debugstr3);
					OgreConsole::getSingleton().print(debugstr4);
					OgreConsole::getSingleton().print(debugstr5);
					OgreConsole::getSingleton().print(debugstr6);*/
					}
					break;

				default:
					break;
			 }
			if (alive)
			CWeapon::getSingleton().Press(arg);
	}

}
//////////////////////////////////////////
void Player::setParentRelation(SceneNode* pPar)
{
	parentRelation=true;
	mPar=pPar;
	pRelPosition=get_location()-pPar->getPosition();
}
//////////////////////////////////////////
void Player::setParentRelation(String node)
{
	SceneNode* pPar = mSceneMgr->getSceneNode(node);
	setParentRelation(pPar);
}
//////////////////////////////////////////
void Player::MouseMove(const OIS::MouseEvent &arg,Ogre::Real time)
{
		if (Inventory::getSingleton().isVisible())
		return;
			/*camera_rotation_x = -mRotate * arg.state.X.rel;
			camera_rotation_y = -mRotate * arg.state.Y.rel;*/
			//mViewNode->yaw(Degree(camera_rotation_x), Node::TS_WORLD);]
	//if (Inventory::getSingleton().isVisible())
	//	return;

			if (global::getSingleton().computer_mode)
	{
		if (arg.state.buttonDown(OIS::MB_Right))
			{
				mCamera->setFOVy(Degree(mCamera->getFOVy())+Degree(arg.state.Y.rel));
			}
	}
			if (!freezed)
			{
			
			if (noclip)
			{
			mNoclipNode->yaw(Degree(-mRotate * arg.state.X.rel), Node::TS_WORLD);
			mNoclipNode->pitch(Degree(-mRotate * arg.state.Y.rel), Node::TS_LOCAL);
			}
			if (!noclip)
			{
				camera_rotation_x = -mRotate * arg.state.X.rel+Math::RangeRandom(-razbros*time,razbros*time);
				camera_rotation_y = -mRotate * arg.state.Y.rel+razbros*time;
				razbros=0;
            mViewNode->pitch(Degree(camera_rotation_y), Node::TS_LOCAL);
			
			y_rotation_cont += camera_rotation_y;
			x_rotation_cont += camera_rotation_x;
			if (x_rotation_cont >= 360)
			{
				x_rotation_cont = 0;
			}
			if (y_rotation_cont > y_limit_a || y_rotation_cont < y_limit_b) 
			{
				if (!noclip)
				{
				mViewNode->pitch(Degree(-camera_rotation_y ));
				}
				if (noclip)
				{
				mNoclipNode->pitch(Degree(-camera_rotation_y ));
				}
				y_rotation_cont -= camera_rotation_y;
			}
			}
			/*debugstr1="";
			debugstr2="";
			debugstr1=debugstr1+"CamRotX="+_tostr(camera_rotation_x);
			debugstr2=debugstr2+"CamRotY="+_tostr(camera_rotation_y);*/
			CWeapon::getSingleton().Move(arg,time);
			}
}
//////////////////////////////////////////
void Player::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{


	if (!alive)
	{
		global::getSingleton().GUIorGame=false;
	   LoadMap::getSingleton().UnloadM(mSceneMgr);
	   HUD::getSingleton().Show();
	   LoadMap::getSingleton().LoadPrev("Ogre Testmap","/scripts","run3/maps","General",mSceneMgr);
	   this->noclip=false;
	}
		if (Inventory::getSingleton().isVisible())
		return;
		Body* b = get_ray_shoot();
		if (b)
		{
			vector<PhysObject*> pobj = POs::getSingleton().getObjects();
			int i;
			if (!(b->getName()=="PLAYER1"))
			{
			for (i=0;i!=pobj.size();i++)
			{
				if (pobj[i]->isThisPO(b))
				{
					if (pobj[i]->dragging)
					{
						pobj[i]->dragging=false;
						Vector3 dir = get_direction();
						pobj[i]->addForce(dir*20000);
					}
					else
						CWeapon::getSingleton().MousePress(arg,id);
				}

			}
			}
		}
		else
		{
			CWeapon::getSingleton().MousePress(arg,id);
		}
}
//////////////////////////////////////////
void Player::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
		if (Inventory::getSingleton().isVisible())
		return;
			CWeapon::getSingleton().MouseRelease(arg,id);
}
//////////////////////////////////////////

void Player::FCRelease(const OIS::KeyEvent &arg)
{
		//arg.device->
	    switch (arg.key)
        {
			case OIS::KC_LSHIFT:
				   fps_speed=mProps.stdVel;
				   OgreConsole::getSingleton().print("sprint..");
				   SuperFX::getSingleton().toggleMotionBlur();
					break;
			case OIS::KC_SPACE:
				   jump = false;
				   stop_count=stop_count_std;
				   skippingCutScene=false;
				   //unpressed=true;
					break;
			case OIS::KC_UP:
			case OIS::KC_W:
				mDirection.z = 0;
				if (!mProps.inertia_active)
				{
				if (isOnEarth())
				{
				bod->setVelocity(Vector3(0,0,0));
				}
				}
				m_FrontVelocity = 0;
				stop_count=stop_count_std; 
				//unpressed=true;
								//bod->setVelocity(force2/100);
				break;

			case OIS::KC_DOWN:
			case OIS::KC_S:
				mDirection.z = 0;
				if (!mProps.inertia_active)
				{
				if (isOnEarth())
				{
				bod->setVelocity(Vector3(0,0,0));
				}
				}
				m_FrontVelocity = 0;
				stop_count=stop_count_std;
				//unpressed=true;
					//bod->setVelocity(force2/100);
				break;

			case OIS::KC_LEFT:
			case OIS::KC_A:
				mDirection.x = 0;
				//if (mProps.resetVelocityOnRelease)
				if (!mProps.inertia_active)
				{
				if (isOnEarth())
				{
				bod->setVelocity(Vector3(0,0,0));
				}
				}
				m_StrafeVelocity = 0;
				stop_count=stop_count_std;
				//unpressed=true;
				//bod->setVelocity(force/100);
				if (mProps.resetVelocityOnRelease && isOnEarth() )
				bod->setVelocity(Vector3(0,0,0));
				if (mProps.rotate_hor)
				mViewNode->roll(Degree(1));
				break;

			case OIS::KC_RIGHT:
			case OIS::KC_D:
				mDirection.x = 0;

				//if (mProps.resetVelocityOnRelease)
				if (!mProps.inertia_active)
				{
				if (isOnEarth())
				{
				bod->setVelocity(Vector3(0,0,0));
				}
				}
				m_StrafeVelocity = 0;
				stop_count=stop_count_std;
				//unpressed=true;
				//bod->setVelocity(force/100);
				if (mProps.resetVelocityOnRelease && isOnEarth())
				bod->setVelocity(Vector3(0,0,0));
				if (mProps.rotate_hor)
				mViewNode->roll(Degree(-1));
				break;
			case OIS::KC_LCONTROL:
			case OIS::KC_RCONTROL:
				if (alive)	
				{
				if (!isTop())
				{
					UnDuck();
				}
				else
				{
					awaitingunduck=true;
				}
				}
				break;
			default:
				break;
			} // switch
		if (global::getSingleton().getOISKeyboard())
		{
#define KEY_DOWNR(x) global::getSingleton().getOISKeyboard()->isKeyDown(x)
			if (!KEY_DOWNR(OIS::KC_W)&&!KEY_DOWNR(OIS::KC_A)&&!KEY_DOWNR(OIS::KC_S)&&!KEY_DOWNR(OIS::KC_D))
			{
				unpressed=true;
				if (!mProps.inertia_active)
				{
				if (isOnEarth())
				{
				bod->setVelocity(Vector3(0,0,0));
				}
				}
			}
#undef KEY_DOWNR
		}
		CWeapon::getSingleton().Release(arg);
}
//////////////////////////////////////////
void Player::Duck()
{
	bod->setCollision(duckcol);
	cur_size=size_dc;
	//Uncoment this to delete bug! This isn't a bug, it's a feature ;]//
	//16.07.10 Uncommented//
	bod->setPositionOrientation(Vector3(get_body_position(bod).x,get_body_position(bod).y-(size-size_dc).y,get_body_position(bod).z),get_body_orientation(bod));
}
//////////////////////////////////////////
void Player::UnDuck()
{
	cur_size=size;
	bod->setCollision(col);
	bod->setPositionOrientation(Vector3(get_body_position(bod).x,get_body_position(bod).y+(size-size_dc).y,get_body_position(bod).z),get_body_orientation(bod));
}
//////////////////////////////////////////
Ogre::Vector3 Player::get_cur_size()
{
	return cur_size;
}
//////////////////////////////////////////
void Player::playerDie()
{
	HUD::getSingleton().SetHealth(0);
	HUD::getSingleton().SSBlood();
	alive=false;
Run3SoundRuntime::getSingleton().emitSound("run3/sounds/bodysplat.wav",2.0f,false);
bod->setVelocity(Vector3(0,0,0));
m_FrontVelocity = 0;
m_StrafeVelocity = 0;
//mViewNode->
mViewNode->roll(Degree(-60));
bod->setPositionOrientation(get_location(),Quaternion(Degree(90),Vector3::UNIT_Z));
Duck(); //<-start here
CWeapon::getSingleton().stripall();
LogManager::getSingleton().logMessage("PlayerController: Player died!");
/*if (uv2)
{
	LogManager::getSingleton().logMessage("PlayerController: Player died!");
	delete uv2;
	uv=0;
}*/
/*if (uv1&&uv3)
{
	LogManager::getSingleton().logMessage("PlayerController: Player died!");
	delete uv1;
LogManager::getSingleton().logMessage("PlayerController: Player died!");
	delete uv3;
	LogManager::getSingleton().logMessage("PlayerController: Player died!");
	uv1=0;uv2=0;uv3=0;
}*/
LogManager::getSingleton().logMessage("PlayerController: all player resources freed!");
}
//////////////////////////////////////////
void Player::setHealthRegeneration(Real rMul,Real maxHealth)
{
	regen_Mul=rMul;
	mMaxHealth=maxHealth;
}
//////////////////////////////////////////
void Player::setOnLadder(bool ladder)
{
	if (ladder)
	{
		//LogManager::getSingleton().logMessage("Test test test" + mFootstep);
		mFootstep="ladder";
	}
	if ((ladder)&&(!mLadder))
	{
		//LogManager::getSingleton().logMessage("Set footstep");
		if (mFootstep!="ladder")
			fstPrf_before = mFootstep;
	}
	if ((!ladder)&&(mLadder))
	{
		//LogManager::getSingleton().logMessage("ReSet footstep 2");
		mFootstep=fstPrf_before;
	}
	mLadder=ladder;
}
//////////////////////////////////////////
void Player::FCUpdate(const FrameEvent &evt)
{
	sound->setListenerPosition(get_body_position(bod),Vector3::ZERO,mViewNode->_getDerivedOrientation());
	if (st!=-1.0f)
	{
		//LogManager::getSingleton().logMessage("shakepl");
		st+=evt.timeSinceLastFrame;
		shakeAmplitude();
	}

	if (!global::getSingleton().computer_mode)
	{
	if (razbros!=0.0f)
	{
		mViewNode->pitch(Degree(razbros), Node::TS_LOCAL);
		y_rotation_cont += razbros;
		if (y_rotation_cont > y_limit_a || y_rotation_cont < y_limit_b && !noclip) 
		{		
				mViewNode->pitch(Degree(-razbros ));
				y_rotation_cont -= razbros;
		}
				razbros=0;
	}
	}
	Real health2 = health;
	if (!god)
	{
	if (alive)
	health-=bod->getDamage();
	if (health2!=health)
	{
		LogManager::getSingleton().logMessage("health loss");
		LogManager::getSingleton().logMessage(StringConverter::toString(health2));
		LogManager::getSingleton().logMessage(StringConverter::toString(health));
		BloodEmitter::getSingleton().emitBlood(get_location(),Vector3(0.3,0.3,0.3));
		HUD::getSingleton().SSBlood();
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/pl_fallpain1.wav",1.0f,false);
	}
	}
	if (health<0 && alive)
	{
		playerDie();
	}
	if (health<mMaxHealth)
		health+=evt.timeSinceLastFrame/regen_Time*regen_Mul;
	if (health>mMaxHealth)
		health=mMaxHealth;
	if(alive)
		HUD::getSingleton().SetHealth(health);

	/*if (!reset_vel&&unpressed)
	{
	stop_count-=evt.timeSinceLastFrame;
	LogManager::getSingleton().logMessage(StringConverter::toString(stop_count));
	if (stop_count<0)
		reset_vel=true;
	}

	if (reset_vel&&unpressed)
	{
		if (isOnEarth())
		{
				bod->setVelocity(Vector3(0,0,0));
		}
	}*/

	if ((m_FrontVelocity!=0)||(m_StrafeVelocity!=0))
	{
		if (isOnEarth()||mLadder)
		{
			fstTimer+=evt.timeSinceLastFrame;
			if (fstTimer>((1/fps_speed)*0.5f))
			{
				fstindex++;
				String fname="";
				fname = fname + "run3/sounds/footsteps/"+mFootstep+StringConverter::toString(fstindex)+".wav";
				Run3SoundRuntime::getSingleton().emitSound(fname,0.3f,false);
				
				if (fstindex==4)
					fstindex=1;
				fstTimer=0.0f;
				//LogManager::getSingleton().logMessage("Reset footstep");
				setOnLadder(false);
			}
		}
	}
	if(noclip)
	{
		acel = Vector3(0,0,0);
		mNoclipNode->translate(mDirection * evt.timeSinceLastFrame, Node::TS_LOCAL);
		bod->freeze();
		
	}
	else
	{
		if (!freezed)
		{
		bod->unFreeze();
		}
		
	}
	if (awaitingunduck && !isTop())
	{
		UnDuck();
		awaitingunduck=false;
	}
	if (debug>3)
		debug=0;
	if (debug!=0)
	{
		debugOverlay->show();
	}
	else
	{
		debugOverlay->hide();
	}

	if (debug==2)
	{
	String debugstr1=StringConverter::toString(camera_rotation_x);
	String debugstr2=StringConverter::toString(camera_rotation_y);
	String debugstr3="";
	String debugstr4="";
	String debugstr5="";
	String debugstr6="";
	debugstr3=debugstr3+"Body Position(xyz)=" + _tostr(get_body_position(bod).x) +" \n"+ _tostr(get_body_position(bod).y) +" \n"+ _tostr(get_body_position(bod).z);
	debugstr4=debugstr4+"Body Orientation(wxyz)=" +_tostr(get_body_orientation(bod).w)+" \n"+ _tostr(get_body_orientation(bod).x) +" \n"+ _tostr(get_body_orientation(bod).y) +" \n"+ _tostr(get_body_orientation(bod).z)+" \nROT3="+StringConverter::toString(rot3);
	debugstr5=debugstr5+"Front Velocity(xyz)=" + _tostr(m_FrontVelocity) +" \n"+ _tostr(m_FrontVelocity) +" \n"+ _tostr(m_FrontVelocity);
	debugstr6=debugstr6+"Strafe Velocity(xyz)=" + _tostr(m_StrafeVelocity) +" \n"+ _tostr(m_StrafeVelocity) +" \n"+ _tostr(m_StrafeVelocity)+" \n"+"Force(xyz)=" + _tostr(force.x) +" \n"+ _tostr(force.y) +" \n"+ _tostr(force.z)+" \n"+"Force2(xyz)=" + _tostr(force2.x) +" \n"+ _tostr(force2.y) +" \n"+ _tostr(force2.z)+" \n"+"mViewNode(wxyz)=" + _tostr(mViewNode->_getDerivedOrientation().w) +" \n"+ _tostr(mViewNode->_getDerivedOrientation().x) +" \n"+ _tostr(mViewNode->_getDerivedOrientation().y)+" \n"+ _tostr(mViewNode->_getDerivedOrientation().z)+" \nSUM\n"+StringConverter::toString(force+force2)+"\n"+StringConverter::toString(mViewNode->_getDerivedPosition());
	
	textbox->setCaption("Player stats\n"+debugstr1+" \n"+debugstr2+" \n"+debugstr3+" \n"+debugstr4+" \n"+debugstr5+" \n"+debugstr6);
	}
	if (debug==1)
	{
		const RenderTarget::FrameStats& stats = global::getSingleton().getWindow()->getStatistics();
		textbox->setCaption("Perfomance Stats\nFPS:"+ StringConverter::toString(stats.lastFPS)+"\nPolyCount:\n"+ StringConverter::toString(stats.triangleCount));
	}
	if (debug==3)
	{
		String l1;
		for (unsigned int i=0;i!=Run3SoundRuntime::getSingleton().fileNames.size();i++)
		{
			l1=l1+"\n"+Run3SoundRuntime::getSingleton().fileNames[i];
		}
		String l2;
		 std::map<unsigned int,String>::iterator ik;
		for (ik=sound->sounds.begin();ik!=sound->sounds.end();ik++)
			l2=l2+"\n"+(*ik).second;
		textbox->setCaption("Sound Stats\nR3SR:\n"+l1+"SMGR:\n"+l2);
	}

}			

