#include "LaserMinigun.h"
#include "global.h"
#include "SuperFX.h"
#include "CrosshairOp.h"
#include "MeshDecalMgr.h"
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////Original class by:Sgw32				       ////////
///////////////Copyright(c) 2010 Sgw32 Corporation		   ////////
///////////////////////////////////////////////////////////////////
///started:22.03.2010											/*/
///finished:?													/*/
///////////////////////////////////////////////////////////////////
LaserMinigun::LaserMinigun()
{
bod=0;
shooting=false;
haveanim=false;
allowShoot=false;
hasRing=false;
hasIt=false;
soundEstUnload=0;
}

void LaserMinigun::buyWeapon()
{
	LogManager::getSingleton().logMessage("Weapon "+this->get_name()+" is now buying");
	//mWeaponNode->setVisible(true);
	mLightenNode->setVisible(false);
	//LogManager::getSingleton().logMessage("Weapon "+this->get_name()+" is now buying");
	In();
	//LogManager::getSingleton().logMessage("Weapon "+this->get_name()+" is now buying");
	changeHUD();
	//LogManager::getSingleton().logMessage("Weapon "+this->get_name()+" is now buying");
		hasIt=true;
		//LogManager::getSingleton().logMessage("Weapon "+this->get_name()+" is now buying");
}


LaserMinigun::~LaserMinigun()
{

}

void LaserMinigun::init(Ogre::Root *_root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* _ent,SoundManager* sound,OgreNewt::World* world)
{
   root=_root;
   mWorld=world;
   mSceneMgr=sceneMgr;
   pLua=global::getSingleton().getLuaState();
   mWeaponNode=weaponNode;
   mPlayerNode=weaponNode->getParentSceneNode()->getParentSceneNode();
   weapon = _ent;
   _ent->setCastShadows(false);
   weapon->setRenderQueueGroup(RENDER_QUEUE_6);
   //mSceneMgr->
   //weapon->setRenderQueueGroup(RENDER_QUEUE_1);
   /*mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setWeight(0.1);
   mWeaponState->setEnabled(true);
   mWeaponState->setLoop(true);*/
   shoot="Run3/game/weapons/"+_ent->getName();
   cf.load(shoot+"/weapon.cfg");
   hasMuzzleFlash=StringConverter::parseBool(cf.getSetting("hasMuzzleFlash",StringUtil::BLANK,"true"));
   scale_ind=StringConverter::parseVector3(cf.getSetting("relScale"));
   ironSight=StringConverter::parseVector3(cf.getSetting("ironSight"));
   if (scale_ind!=Vector3::ZERO)
   {
	   weaponNode->setScale(scale_ind);
   }
   shoot=cf.getSetting("Attack1");
  // HUD::getSingleton().ChangeHUDOverlay(cf.getSetting("HUDOverlay",StringUtil::BLANK,"Run3/HUDAmmo"),false);
   myHUD = cf.getSetting("HUDOverlay",StringUtil::BLANK,"Run3/HUDAmmo");
   singleShot=StringConverter::parseBool(cf.getSetting("singleShot",StringUtil::BLANK,"false"));
   haveanim=StringConverter::parseBool(cf.getSetting("haveAnims",StringUtil::BLANK,"false"));
ringMat = cf.getSetting("ringMat",StringUtil::BLANK,"Run3/CrosshairRing");
crosshairMat = cf.getSetting("crosshairMat",StringUtil::BLANK,"Run3/Crosshair");
	stammo=StringConverter::parseUnsignedInt(cf.getSetting("Ammo",StringUtil::BLANK,"360"));
	Vector3 tr = StringConverter::parseVector3(cf.getSetting("Translate",StringUtil::BLANK,"0 0 0"));
	Vector3 mtr = StringConverter::parseVector3(cf.getSetting("muzzleTran",StringUtil::BLANK,"0 0 0"));
	Vector3 msc = StringConverter::parseVector3(cf.getSetting("muzzleScale",StringUtil::BLANK,"1 1 1"));
	dmgPSHT=StringConverter::parseReal(cf.getSetting("damagePerShot",StringUtil::BLANK,"10"));
	defOtt =StringConverter::parseReal(cf.getSetting("ottdacha",StringUtil::BLANK,"1"));
	ottSpd  = 1.0f +StringConverter::parseReal(cf.getSetting("ottSpd",StringUtil::BLANK,"0"));
	if (tr!=Vector3::ZERO)
		mWeaponNode->translate(tr);
	ammo=stammo;
	hasRing=StringConverter::parseBool(cf.getSetting("hasRing",StringUtil::BLANK,"false"));
   secondSlot_allow=StringConverter::parseBool(cf.getSetting("AllowSecondSlot",StringUtil::BLANK,"false"));
   hasRingShift=StringConverter::parseBool(cf.getSetting("hasRingShift",StringUtil::BLANK,"false"));
   second_Slot=StringConverter::parseUnsignedInt(cf.getSetting("SecondSlot",StringUtil::BLANK,"0"));
   m_pause=StringConverter::parseReal(cf.getSetting("master_Pause",StringUtil::BLANK,"10"));
   fuzzy=StringConverter::parseReal(cf.getSetting("FuzzyShot",StringUtil::BLANK,"0"));
   col=StringConverter::parseColourValue(cf.getSetting("lightColor",StringUtil::BLANK,"1 0.7 0.1"));
   resetTimePosition=StringConverter::parseBool(cf.getSetting("resetTimePosition",StringUtil::BLANK,"false"));
   luaOnShoot=cf.getSetting("LUAFileShoot",StringUtil::BLANK,"none");
   primaryAmmo=second_Slot;
	if (secondSlot_allow)
	{
		ammo-=primaryAmmo;
	}
   soundmgr=sound;
   //soundmgr->loadAudio("Run3/sounds//"+shoot, &shoots, false); 
   
  lShock_t = mSceneMgr->createParticleSystem("muzzleFlashS"+_ent->getName(), "muzzleFlashAK");
   mLightenNode= weaponNode->createChildSceneNode(mtr);
   String ha = StringConverter::toString(haveanim);
   pauseBetweenShots=StringConverter::parseReal(cf.getSetting("pauseBetweenShots",StringUtil::BLANK,"0"));
   LogManager::getSingleton().logMessage("pbs");
	LogManager::getSingleton().logMessage(cf.getSetting("pauseBetweenShots",StringUtil::BLANK,"0"));
   mWeaponState=0;
   if (haveanim)
   {
	   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setEnabled(true);
   mWeaponState->setLoop(true);
   }
   root->addFrameListener(this);
   
Plane plane( Vector3::UNIT_Y, 0 );
   Plane plane2( Vector3::NEGATIVE_UNIT_X, 0 );

   MeshManager::getSingleton().createPlane("Shockrifle_Light_n"+_ent->getName(),
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
			27,60,1,1,true,1,1,1,Vector3::UNIT_Z);
	MeshManager::getSingleton().createPlane("Shockrifle_Light2_n"+_ent->getName(),
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane2,
			27,60,1,1,true,1,1,1,Vector3::UNIT_Z);

	/*ent = mSceneMgr->createEntity( "Lightning"+_ent->getName(), "Shockrifle_Light_n"+_ent->getName() );
	//mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
	ent->setMaterialName("Run3/MuzzleFlash");
	ent2 = mSceneMgr->createEntity( "Lightning2"+_ent->getName(), "Shockrifle_Light2_n"+_ent->getName() );
	//mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
	ent2->setMaterialName("Run3/MuzzleFlash");*/

	mLightenNode->attachObject(lShock_t);
	//mLightenNode->attachObject(ent2);
	mLightenNode->setScale(msc);
	//Plane plane( Vector3::UNIT_Y, 0 );
   //mLightenNode->attachObject(//lShock_t);
   ////lShock_t->setVisible(false);
   mLightenNode->setVisible(false);
   startCount=false;
   /*//lShock_t->setVisible(true);
   mLightenNode->setVisible(true);*/
   energyid=1;
  
   fcnt=0;
   soundLoaded=false;
   time_pause=0;
   modPos=tr;
   LogManager::getSingleton().logMessage(StringConverter::toString(mWeaponNode->getPosition()));
}


void LaserMinigun::Move(const OIS::MouseEvent &arg,Ogre::Real time)
{
	
	CrosshairOperator::getSingleton().getCrosshairShift(global::getSingleton().getPlayer()->camera_rotation_x,global::getSingleton().getPlayer()->camera_rotation_y);
	global::getSingleton().getPlayer()->camera_rotation_y=0;
}
void LaserMinigun::Press(const OIS::KeyEvent &arg)
{
	if (arg.key==OIS::KC_R && secondSlot_allow)
	{
		if ((ammo+primaryAmmo)>=second_Slot)
		{
		ammo=ammo-second_Slot+primaryAmmo;
		primaryAmmo=second_Slot;
		//HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
		}
		else
		{
			primaryAmmo=ammo;
			ammo=0;
		}
		HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
	}
}
void LaserMinigun::Release(const OIS::KeyEvent &arg)
{
	
}
void LaserMinigun::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	/*if ((ammo+primaryAmmo)>0)
	{*/
	/*if (ammo>0)
	{*/

	if ((ammo>0 && !secondSlot_allow) || (primaryAmmo>0 && secondSlot_allow)) 
	{
	if (id==OIS::MB_Left)
	{
	if (TIME_SHIFT==0.0f)
		return;
		if (!soundLoaded)
		{
		soundmgr->loadAudio("Run3/sounds//"+shoot, &shoots, false); 
		soundLoaded=true;
		}
		
		soundmgr->setSound(shoots,mPlayerNode->getPosition(),Vector3(0,0,0),Vector3(0,0,0),10000,false,false,1000,1000,1000);
	//	curOtt=defOtt;
		if (singleShot)
		{
			curOtt=defOtt;
			soundmgr->playAudio(shoots,true);
		}
		if (singleShot&&!secondSlot_allow)
		{
		if(ammo!=0)
			ammo--;
		}
		if (singleShot&&secondSlot_allow)
		{
		if(primaryAmmo!=0)
			primaryAmmo--;
		}
		shooting=true;
		fcnt=0;
		startCount=false;
		allowShoot=true;

if (mSceneMgr->hasLight("flashLighter2"))
			{
			flashLight=mSceneMgr->getLight("flashLighter2");
			}
else
{
flashLight = mSceneMgr->createLight("flashLighter2");
}
    flashLight->setDiffuseColour(col);
    flashLight->setSpecularColour(Ogre::ColourValue(1,1,1));
  flashLight->setType(Ogre::Light::LT_SPOTLIGHT);
    flashLight->setSpotlightInnerAngle(Ogre::Degree(60));
    flashLight->setSpotlightOuterAngle(Ogre::Degree(90));
    flashLight->setAttenuation(2000, 1, 1, 1); // meter range.  the others our shader ignores
    flashLight->setDirection(Ogre::Vector3(0, 0, -1));
	mWeaponNode->attachObject(flashLight);
	//flashLight->setCastShadows(false);
	flashLight->setVisible(true);

		/*if (singleShot && time_pause>0)
		{
			allowShoot=false;
			time_pause=0;
		}*/
		
		/*mWeaponState->setEnabled(false);
		mWeaponState->setLength(0.5f);
		mWeaponState = weapon->getAnimationState("attack01");
		mWeaponState->setLoop(true);
		mWeaponState->setEnabled(true);*/
		/**/
		////lShock_t->setVisible(true);

		if (haveanim)
		{
				if (mWeaponState)
	{
	mWeaponState->setEnabled(false);
	}
			mWeaponState = weapon->getAnimationState("attack01");
			if (!singleShot)
			{
			mWeaponState->setLoop(true);
			if (resetTimePosition)
				mWeaponState->setTimePosition(0);
			mWeaponState->setEnabled(true);

			}
			else
			{
			
			mWeaponState->setLoop(false);
			mWeaponState->setTimePosition(0);
			mWeaponState->setEnabled(true);
			}
		}
		////lShock_t->setVisible(true);
		if (hasMuzzleFlash)
			mLightenNode->setVisible(true);

		
	}
	}
	if (id==OIS::MB_Middle)
	{
		energyid++;
		if (energyid==4)
			energyid=1;
	}
	if (id==OIS::MB_Right)
	{
		//mWeaponNode->setPosition(mWeaponNode->getPosition()+ironSight);
		LogManager::getSingleton().logMessage(StringConverter::toString(mWeaponNode->getPosition())+"AA!");
		LogManager::getSingleton().logMessage(StringConverter::toString(modPos)+"AA2!");
		Modulator::getSingleton().modulate_scenenode_pos(mWeaponNode,modPos+ironSight,0.5f);
		Modulator::getSingleton().modulator_complete_fov();
		Modulator::getSingleton().modulate_FOV(global::getSingleton().getPlayer()->getFOV(),global::getSingleton().getPlayer()->getFOV()-20,0.7);
		//global::getSingleton().getPlayer()->changeFOV(global::getSingleton().getPlayer()->getFOV()-20);
		HUD::getSingleton().Hide_crosshair();
		SuperFX::getSingleton().toggleRadialBlur();
	}
	//mWeaponState->setEnabled(true);
}

OgreNewt::Body* LaserMinigun::get_ray_shoot()
{
//Ogre::Vector3 myPos = mPlayerNode->getPosition();ghjgh

	//Ogre::Vector3 myPos =  global::getSingleton().getPlayer()->get_location()+Vector3(0,50,0);
//Ogre::Quaternion myOrient = mWeaponNode->getParentSceneNode()->getOrientation();
	Ogre::Vector3 myPos = mWeaponNode->getParentSceneNode()->_getDerivedPosition();
Ogre::Vector3 direction,end;
//direction = myOrient*Vector3::NEGATIVE_UNIT_Z;
direction = get_direction();
Ogre::Vector3 rnd(
               Ogre::Math::RangeRandom(-fuzzy, fuzzy),
               Ogre::Math::RangeRandom(-fuzzy, fuzzy),
               Ogre::Math::RangeRandom(-fuzzy, fuzzy));
	//Ogre::Vector3 myPos = mPlayerNode->_getDerivedPosition()+Vector3(0,50,0)+direction*20.5;
end=myPos + direction * 100000.0f+rnd;
//OgreNewt::BasicRaycast shootRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Z * 100);
//LogManager::getSingleton().logMessage(StringConverter::toString(myPos)+""+StringConverter::toString(end));
OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
//LogManager::getSingleton().logMessage("LaserMinigun template: first collide @"+StringConverter::toString(shootRay.getFirstHit().mDistance*(end-myPos).length()));
/*Real hitLen = (end-myPos).length();
LogManager::getSingleton().logMessage("Laser minigun hit length="+StringConverter::toString(hitLen));*/
Vector3 hitpos = myPos + shootRay.getFirstHit().mDistance*(direction*100000+rnd);
BulletHitManager::getSingleton().addBulletEffect(hitpos,0.5f);

OgreNewt::BasicRaycast shootRay2(mWorld,myPos,end-Vector3(500,0,500));
OgreNewt::BasicRaycast shootRay3(mWorld,myPos,end+Vector3(500,0,500));
if (shootRay2.getFirstHit().mBody&&(shootRay2.getFirstHit().mBody==shootRay3.getFirstHit().mBody)&&(shootRay2.getFirstHit().mBody->getType()==4))
{
Vector3 hitpos2 = myPos + shootRay2.getFirstHit().mDistance*0.99*(direction*100000+rnd-Vector3(500,0,500));
Vector3 hitpos3 = myPos + shootRay3.getFirstHit().mDistance*0.99*(direction*100000+rnd+Vector3(500,0,500));

MeshDecalMgr::getSingleton().addHoleEffect(hitpos-direction*0.01,shootRay2.getFirstHit().mNormal);
}
if (shootRay.getFirstHit().mBody)
{
if (shootRay.getFirstHit().mBody->getType()==PHYSOBJECT_NPC)
{
	shootRay.getFirstHit().mBody->damagePosition=hitpos;	
}
}
return shootRay.getFirstHit().mBody;
}

Ogre::Vector3 LaserMinigun::get_direction()
{
/*Ogre::Quaternion myOrient = mPlayerNode->getOrientation(); //Where the body is
Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation(); //An orientation of mViewNode
Ogre::Vector3 direction2,down,down2,direction;
direction2 = myOrient2*Vector3::NEGATIVE_UNIT_Z;
down = Vector3(0,direction2.y,0);
direction = myOrient*Vector3::NEGATIVE_UNIT_Z+down;
LogManager::getSingleton().logMessage(StringConverter::toString(direction));
return direction;*/
	
	Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation();
	//LogManager::getSingleton().logMessage(StringConverter::toString( myOrient2*Vector3::NEGATIVE_UNIT_Z));
	return myOrient2*Vector3::NEGATIVE_UNIT_Z;
}


void LaserMinigun::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	if (id==OIS::MB_Left)
	{
	allowShoot=true;
	shooting=false;
			if ((flashLight)&&(mSceneMgr->hasLight("flashLighter2")))
		{

			mWeaponNode->detachObject(flashLight);
			//LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLighter3"))+"PLAYERB!!!");
			if (flashLight)
				mSceneMgr->destroyLight(flashLight);
			flashLight=0;
		}
	/*if (!singleShot)
	{
		if (soundLoaded)
		{
	soundmgr->releaseAudio(shoots);
	soundLoaded=false;
		}
	}
	else
	{*/
		startCount=true;
		

	bod=0;
		if (haveanim)
		{
				if (mWeaponState)
				{
				mWeaponState->setEnabled(false);
				}
				mWeaponState = weapon->getAnimationState("idle");
				mWeaponState->setLoop(true);
				mWeaponState->setEnabled(true);
		}
	}
	//lShock_t->setVisible(false);
	if (hasMuzzleFlash)
	mLightenNode->setVisible(false);
	////lShock_t->setVisible(false);
	

	if (id==OIS::MB_Right)
	{
		//mWeaponNode->setPosition(mWeaponNode->getPosition()-ironSight);
		LogManager::getSingleton().logMessage(StringConverter::toString(mWeaponNode->getPosition())+"AAA!");
		LogManager::getSingleton().logMessage(StringConverter::toString(modPos)+"AAA2!");
		Modulator::getSingleton().modulate_scenenode_pos(mWeaponNode,modPos,0.5f);
		Modulator::getSingleton().modulator_complete_fov();
		Modulator::getSingleton().modulate_FOV(global::getSingleton().getPlayer()->getFOV(),global::getSingleton().getPlayer()->getFOV()+20,0.7);
		//global::getSingleton().getPlayer()->changeFOV(global::getSingleton().getPlayer()->getFOV()+20);
		HUD::getSingleton().Show_crosshair();
		SuperFX::getSingleton().toggleRadialBlur();
	}
  /* mWeaponState->setEnabled(false);
   mWeaponState->setLength(5.0f);
   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setEnabled(true);*/
   //
   //mTreeNode->setVisible(false);
	
}

void LaserMinigun::In()
{
	/*mWeaponState->setEnabled(false);
	mWeaponState = weapon->getAnimationState("in");
	mWeaponState->setLoop(false);
	mWeaponState->setEnabled(true);*/
	in=true;
}

bool LaserMinigun::frameStarted(const Ogre::FrameEvent &evt)
{
	if (TIME_SHIFT==0.0f)
		return true;
	if (in)
	{
		/*if (mWeaponState->getTimePosition() >= mWeaponState->getLength())
		{
			mWeaponState->setEnabled(false);
			mWeaponState = weapon->getAnimationState("idle");
			mWeaponState->setLength(5.0f);
			mWeaponState->setLoop(true);
			mWeaponState->setEnabled(true);
			in=false;
		}*/
	}
	if ((!allowShoot)&&(shooting))
	{
	time_pause+=evt.timeSinceLastFrame*TIME_SHIFT;
	//LogManager::getSingleton().logMessage(StringConverter::toString(time_pause));
	if (time_pause>pauseBetweenShots/2)
	{
			//if (hasMuzzleFlash)
			//	mLightenNode->setVisible(false);
		if(mSceneMgr->hasLight("flashLighter2"))
		{
		flashLight->setVisible(false);
		}
	}
	if (time_pause>pauseBetweenShots)
	{
		if(mSceneMgr->hasLight("flashLighter2"))
		{
		flashLight->setVisible(true);
		}
			//if (hasMuzzleFlash)
			//	mLightenNode->setVisible(true);
			allowShoot=true;
			time_pause=0;
	}
	}
//LogManager::getSingleton().logMessage(StringConverter::toString(fcnt)+" "+StringConverter::toString(soundLoaded));
	if (startCount)
	{
		fcnt+=evt.timeSinceLastFrame*TIME_SHIFT;
		
		if (fcnt>m_pause)
		{
			if (soundLoaded)
			{
			soundmgr->releaseAudio(shoots);
			soundLoaded=false;
			}
			bod=0;
			shooting=false;
		if (haveanim)
		{
			if (mWeaponState)
			{
			mWeaponState->setEnabled(false);
			}
				mWeaponState = weapon->getAnimationState("idle");
				mWeaponState->setLoop(true);
				mWeaponState->setEnabled(true);
		}
		//lShock_t->setVisible(false);
		mLightenNode->setVisible(false);
		fcnt=0;
		startCount=false;
		}
	}
	
	/*if (shooting)
	{
		if (time_pause>pauseBetweenShots/2)
		{
			if(mSceneMgr->hasLight("flashLighter2"))
			{
			flashLight->setVisible(false);
			}
		}
		if (time_pause>pauseBetweenShots)
		{
			//if (hasMuzzleFlash)
			//	mLightenNode->setVisible(true);
			allowShoot=true;
			time_pause=0;
		}
	}*/

	if (shooting&&allowShoot)
	{
		

		if (!startCount)
			bod = get_ray_shoot();
		if (!singleShot)
		{
		soundmgr->stopAudio(shoots);
		/*if(mSceneMgr->hasLight("flashLighter2"))
		{
		flashLight->setVisible(true);
		}*/
		soundmgr->playAudio(shoots,false);
		}
		soundmgr->setSoundPosition(shoots,mPlayerNode->getPosition());
		HUD::getSingleton().shoot();
		

		if (!singleShot&&!secondSlot_allow)
		{
		if(ammo!=0)
			ammo--;
		if (ammo==0)
		{
			shooting=false;
			release();
		}
		}

		if (!singleShot&&secondSlot_allow)
		{
		
		if(primaryAmmo!=0)
			primaryAmmo--;
		if (primaryAmmo==0)
		{
			shooting=false;
			release();
		}
		}

		/*if (!singleShot)
		{
		if(ammo!=0)
			ammo--;
		}*/
/*
		if (ammo==0)
		{
			shooting=false;
			release();
		}*/

			if(curOtt>0)
			{
				if (!singleShot)
				curOtt=defOtt;
			global::getSingleton().getPlayer()->razbros=curOtt;
			curOtt-=evt.timeSinceLastFrame*ottSpd*TIME_SHIFT;
			}
		if (singleShot)
		{
			
			release();
		}		
			//LogManager::getSingleton().logMessage(StringConverter::toString(ammo));
		//HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
		if (!secondSlot_allow)
		{
			HUD::getSingleton().setAmmo(StringConverter::toString(ammo));
		}
		else
		{
			HUD::getSingleton().setAmmo(StringConverter::toString(primaryAmmo)+" "+StringConverter::toString(ammo));
		}
		
		//allowShoot=false;
	}
	
	

	if (bod&&shooting&&allowShoot)
	{
		
		vector<PhysObject*> pobj = POs::getSingleton().getObjects();
		int i;
		if (!(bod->getName()=="PLAYER1"))
		{
		for (i=0;i!=pobj.size();i++)
		{
			if (pobj[i]->isThisPO(bod))
			{
				//Energy::getSingleton().processBodyEnergy(bod);
				//Bullet* bullet = new Bullet();
				Vector3 dir = get_direction();
				
				/*bullet->init(mPlayerNode->getPosition()+mPlayerNode->getOrientation()*Vector3(0,0,-100),dir,100000, mWorld,mSceneMgr);
				bullet->start();*/
				//USE BULLETS FROM HERE!!!
				pobj[i]->addForce(dir*10000);
				pobj[i]->remove_health(dmgPSHT);
			}

		}
		if (luaOnShoot!="none")
					RunLuaScript(pLua,luaOnShoot.c_str());
		bod->setDamage(dmgPSHT/2);
		bod->setStandartAddForce(get_direction()*50000);
		}
		
	}
		if (haveanim)
	{
	mWeaponState->addTime(evt.timeSinceLastFrame*TIME_SHIFT);
	}
	allowShoot=false;
	return true;
}

void LaserMinigun::release()
{
	Modulator::getSingleton().logic_erase(mWeaponNode);
	LogManager::getSingleton().logMessage("RELEASE");
	//Modulator::getSingleton().modulator_complete_fov();
	if (singleShot)
	{
	startCount=true;
	}
	else
	{
		shooting=false;
	//soundmgr->releaseAudio(shoots);
	bod=0;
	if (haveanim)
	{
		if (mWeaponState)
		{
		mWeaponState->setEnabled(false);
		}
			mWeaponState = weapon->getAnimationState("idle");
			mWeaponState->setLoop(true);
			mWeaponState->setEnabled(true);
	}
	//lShock_t->setVisible(false);
	mLightenNode->setVisible(false);
	////lShock_t->setVisible(false);
	}
}

bool LaserMinigun::frameEnded(const Ogre::FrameEvent &evt)
{
	return true;
}