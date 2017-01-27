#include "Shockrifle.h"
#include "Run3SoundRuntime.h"
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
////////////////////////special for LUNATIX TEAM - Approach////////
///////////////Original class by:Sgw32				       ////////
///////////////Copyright(c) 2010 Sgw32 Corporation		   ////////
///////////////////////////////////////////////////////////////////
/*				Very powerful rifle = VPF-1						/*/
/*				Okey,VPF-1										/*/
/*  I think that I'm only one building this game.Not very cool! /*/
///started:22.03.2010											/*/
///finished:?													/*/
///////////////////////////////////////////////////////////////////
Shockrifle::Shockrifle()
{
bod=0;
shooting=false;
right=false;
iright2=false;
right2=false;
nochange=false;
}

Shockrifle::~Shockrifle()
{

}

void Shockrifle::init(Ogre::Root *_root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* _ent,SoundManager* sound,OgreNewt::World* world)
{
   root=_root;
   mWorld=world;
   mSceneMgr=sceneMgr;
   root->addFrameListener(this);
   mWeaponNode=weaponNode;
   mPlayerNode=weaponNode->getParentSceneNode()->getParentSceneNode();
   weapon = _ent;
   shoot="Run3/game/weapons/"+_ent->getName();
   cf.load(shoot+"/weapon.cfg");
   //mWeaponState = weapon->getAnimationState("idle");
   mWeaponState = weapon->getAnimationState(cf.getSetting("idleAnim",StringUtil::BLANK,"idle"));
  // mWeaponState->setWeight(0.1);
   mWeaponState->setWeight(StringConverter::parseReal(cf.getSetting("animWeight",StringUtil::BLANK,"0.1")));
  // HUD::getSingleton().ChangeHUDOverlay(,true);
   myHUD = cf.getSetting("HUDOverlay",StringUtil::BLANK,"Run3/HUD");
   mWeaponState->setEnabled(true);
   mWeaponState->setLoop(true);
   
   shoot=cf.getSetting("Attack1");
   soundmgr=sound;
   //soundmgr->loadAudio("Run3/sounds//"+shoot, &shoots, false); 
   energyid=1;
   red_energy=1.0f;
   green_energy=1.0f;
   blue_energy=1.0f;
   minusr=0;
   minusg=0;
   minusb=0;
  /* lShock = mSceneMgr->createParticleSystem("RedLightning", "Weapons/ShockLaser_univ");
   lShock_t = mSceneMgr->createParticleSystem("RedLightning_tree", "Weapons/Shock_tree");
   /*lShock = mSceneMgr->createBillboardSet("RedLightning");
   lShock->setMaterialName("lightning_gun");
   lShock_t = mSceneMgr->createBillboardSet("RedLightning_tree");
   lShock_t->setMaterialName("lightning_gun");
   rgbEmit
   rgbEmit = lShock->createBillboard(mLightPositions[i]);
   //lGreen = mSceneMgr->createParticleSystem("GreenLightning", "Weapons/ShockLaser_g");
   //lBlue = mSceneMgr->createParticleSystem("BlueLightning", "Weapons/ShockLaser_b");
   mLightenNode = weaponNode->createChildSceneNode(Vector3(0,0,1000));
   mTreeNode = weaponNode->createChildSceneNode(Vector3(0,0,1000));
   mLightenNode->attachObject(lShock);
   mTreeNode->attachObject(lShock_t);
   mLightenNode->setScale(5,5,10);
   mTreeNode->setScale(1,1,1);
   //mLightenNode->attachObject(lGreen);
   //mLightenNode->attachObject(lBlue);
   /*lShock->setVisible(false);
   lGreen->setVisible(false);
   lBlue->setVisible(false);*/
   d_p= new DecalProjector;
   d_p->createProjector(mWeaponNode);
   mLightenNode = weaponNode->createChildSceneNode(Vector3(0,0,1000));
   mTreeNode = weaponNode->createChildSceneNode(Vector3(0,0,1000));
   
   mLightenNode->setScale(5,5,10);
   mTreeNode->setScale(1,1,1);
   Plane plane( Vector3::UNIT_Y, 0 );
   Plane plane2( Vector3::UNIT_X, 0 );

   MeshManager::getSingleton().createPlane("Shockrifle_Light_n"+_ent->getName(),
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
			10,200,1,1,true,1,1,1,Vector3::UNIT_Z);
	MeshManager::getSingleton().createPlane("Shockrifle_Light2_n"+_ent->getName(),
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane2,
			10,200,1,1,true,1,1,1,Vector3::UNIT_Z);

	ent = mSceneMgr->createEntity( "Lightning"+_ent->getName(), "Shockrifle_Light_n"+_ent->getName() );
	//mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
	ent->setMaterialName("lightning_gun");
	ent2 = mSceneMgr->createEntity( "Lightning2"+_ent->getName(), "Shockrifle_Light2_n"+_ent->getName() );
	//mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
	ent2->setMaterialName("lightning_gun");
	//mater = (MaterialPtr)MaterialManager::getSingleton().getByName("lightning_gun");
	mater = (MaterialPtr)MaterialManager::getSingleton().getByName(cf.getSetting("shockMaterial",StringUtil::BLANK,"lightning_gun"));
	
	ent->setCastShadows(false);
	ent2->setCastShadows(false);
	mLightenNode->attachObject(ent);
	mLightenNode->attachObject(ent2);
	lShock_t = mSceneMgr->createParticleSystem("RedLightning_tree"+_ent->getName(), "Weapons/Shock_tree");
	rgbEmit_t = lShock_t->getEmitter(0);
	mTreeNode->attachObject(lShock_t);
	mLightenNode->setVisible(false);
   mTreeNode->setVisible(false);
}


void Shockrifle::Move(const OIS::MouseEvent &arg,Ogre::Real time)
{

}
void Shockrifle::Press(const OIS::KeyEvent &arg)
{
	
}
void Shockrifle::Release(const OIS::KeyEvent &arg)
{

}
void Shockrifle::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{

	if (id==OIS::MB_Left)
	{
		shooting=true;
		mWeaponState->setEnabled(false);
		mWeaponState->setLength(0.5f);
		mWeaponState = weapon->getAnimationState("attack01");
		mWeaponState->setLoop(true);
		mWeaponState->setEnabled(true);
		/*soundmgr->stopAudio(shoots);
		soundmgr->setSoundPosition(shoots,mPlayerNode->getPosition());
		soundmgr->playAudio(shoots,false);*/
		mLightenNode->setVisible(true);
		mTreeNode->setVisible(true);
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/beamstart10.wav",2,false);
		soundmgr->loadAudio("Run3/sounds//"+shoot, &shoots, true); 
		soundmgr->setSoundPosition(shoots,mPlayerNode->getPosition());
		soundmgr->playAudio(shoots,true);
		if (energyid==1)
		{
			minusr=0.001f;
			if (red_energy>0)
			{
				rgbEmit_t->setColourRangeStart(ColourValue(1,0,0,1));
				rgbEmit_t->setColourRangeEnd(ColourValue(1,0.21,0.21,1));
				mater->setAmbient(ColourValue(1,0,0,1));
				
			}
		}
		if (energyid==2)
		{
			minusg=0.001f;
			if (green_energy>0)
			{
				rgbEmit_t->setColourRangeStart(ColourValue(0,1,0,1));
				rgbEmit_t->setColourRangeEnd(ColourValue(0.21,1,0.21,1));
				mater->setAmbient(ColourValue(0,1,0,1));
			}
		}
		if (energyid==3)
		{
			minusb=0.001f;
			if (blue_energy>0)
			{
				rgbEmit_t->setColourRangeStart(ColourValue(0,1,1,1));
				rgbEmit_t->setColourRangeEnd(ColourValue(0.21,0.21,1,1));
				mater->setAmbient(ColourValue(0,0,1,1));
			}
		}
		//OgreNewt::World* wrld =  global::getSingleton().getWorld;
		
		
	}
	if (id==OIS::MB_Middle)
	{
		energyid++;
		if (energyid==4)
			energyid=1;
	}
	if (id==OIS::MB_Right)
	{

shooting=true;
		mWeaponState->setEnabled(false);
		mWeaponState->setLength(0.5f);
		mWeaponState = weapon->getAnimationState("attack01");
		mWeaponState->setLoop(true);
		mWeaponState->setEnabled(true);
		/*soundmgr->stopAudio(shoots);
		soundmgr->setSoundPosition(shoots,mPlayerNode->getPosition());
		soundmgr->playAudio(shoots,false);*/

		if (!nochange)
		{
		right=true;
		
		}
		if (iright2)
			right2=true;
	}
	mWeaponState->setEnabled(true);
}

OgreNewt::Body* Shockrifle::get_ray_shoot()
{
Ogre::Vector3 myPos = mPlayerNode->getPosition();
//Ogre::Quaternion myOrient = mWeaponNode->getParentSceneNode()->getOrientation();
Ogre::Vector3 direction,end;
//direction = myOrient*Vector3::NEGATIVE_UNIT_Z;
direction = get_direction();
end=myPos + direction * 10000.0f;
//OgreNewt::BasicRaycast shootRay(mWorld,myPos,myPos+Ogre::Vector3::NEGATIVE_UNIT_Z * 100);
OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
return shootRay.getFirstHit().mBody;
}

Ogre::Vector3 Shockrifle::get_direction()
{
//Ogre::Vector3 myPos = mPlayerNode->getPosition();
Ogre::Quaternion myOrient = mPlayerNode->getOrientation();
Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation();
Ogre::Vector3 direction2,down,down2,direction;
direction2 = myOrient2*Vector3::NEGATIVE_UNIT_Z;
down = Vector3(0,direction2.y,0);
/*down = myOrient2*Vector3::NEGATIVE_UNIT_Y;
direction2 = myOrient*Vector3::NEGATIVE_UNIT_Z;
direction = Vector3(direction2.x,down.y,direction2.z);*/
//direction = myOrient*Vector3::NEGATIVE_UNIT_Z;
direction = myOrient*Vector3::NEGATIVE_UNIT_Z+down;
return direction;
}


void Shockrifle::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	if (id==OIS::MB_Left)
	{
	shooting=false;
	bod=0;
	soundmgr->releaseAudio(shoots);
	}
	if (id==OIS::MB_Right)
	{
		shooting=false;
		if (right)
			iright2=true;
		if (right2)
		{
			right=false;
			right2=false;
			nochange=false;
			iright2=false;
		}
		//right=false;
		if (!nochange)
		bod=0;
	}
	minusr=0;
	minusg=0;
	minusb=0;
   mWeaponState->setEnabled(false);
   mWeaponState->setLength(5.0f);
   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setEnabled(true);
   mLightenNode->setVisible(false);
   mTreeNode->setVisible(false);
}

void Shockrifle::In()
{
	mWeaponState->setEnabled(false);
	mWeaponState = weapon->getAnimationState("in");
	mWeaponState->setLoop(false);
	mWeaponState->setEnabled(true);
	in=true;
}

bool Shockrifle::frameStarted(const Ogre::FrameEvent &evt)
{
	if (in)
	{
		if (mWeaponState->getTimePosition() >= mWeaponState->getLength())
		{
			mWeaponState->setEnabled(false);
			mWeaponState = weapon->getAnimationState("idle");
			mWeaponState->setLength(5.0f);
			mWeaponState->setLoop(true);
			mWeaponState->setEnabled(true);
			in=false;
		}
	}
	if (shooting)
	{
		//if (!right2)
		if (!nochange)
			bod = get_ray_shoot();
		if (right)
		{
			nochange=true;
		}
		if (right2)
		{

			magnet=get_direction()*100000;
		}
	}
	if (bod&&shooting)
	{
		vector<PhysObject*> pobj = POs::getSingleton().getObjects();
		int i;
		if (!(bod->getName()=="PLAYER1"))
		{
		for (i=0;i!=pobj.size();i++)
		{
			if (pobj[i]->isThisPO(bod))
			{
				if (pobj[i]->isMetal)
					Energy::getSingleton().processBodyEnergy(bod);
				if (!right)
				{
				
				//Bullet* bullet = new Bullet();
				Vector3 dir = get_direction();
				//bullet->init(mPlayerNode->getPosition()+mPlayerNode->getOrientation()*Vector3(0,0,-100),dir,1000000, mWorld,mSceneMgr);
				//bullet->start();
				//USE BULLETS FROM HERE!!!
				pobj[i]->remove_health(10);
				pobj[i]->addForce(dir*10000);
				Entity* ent = pobj[i]->ent;
				/*for (unsigned int i = 0; i < ent->getNumSubEntities(); i++)
					d_p->makeMaterialReceiveDecal(ent->getSubEntity(i)->getMaterialName());*/
				}
				else
				{
					if (right2)
					{
						bod->setEnergyForce(magnet);
					}
				}
			}
		}
		bod->setDamage(5);
		bod->setStandartAddForce(get_direction()*50000);
		}
	}
	red_energy=red_energy-minusr;
	green_energy=green_energy-minusg;
	blue_energy=blue_energy-minusb;
	HUD::getSingleton().SetRedEnergy(red_energy);
	HUD::getSingleton().SetGreenEnergy(green_energy);
	HUD::getSingleton().SetBlueEnergy(blue_energy);
	mWeaponState->addTime(evt.timeSinceLastFrame);
	return true;
}

bool Shockrifle::frameEnded(const Ogre::FrameEvent &evt)
{
	return true;
}