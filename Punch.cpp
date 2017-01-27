
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Punch.h"

//template<> Punch *Singleton<Punch>::ms_Singleton=0;
///////////////////////////////////////////////////////////////////
///////////////Original class by:Sgw32				       ////////
///////////////Copyright(c) 2010 Sgw32 Corporation		   ////////
///////////////////////////////////////////////////////////////////

Punch::Punch()
{
}

Punch::~Punch()
{
}

void Punch::init(Ogre::Root *_root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* _ent,SoundManager* sound,OgreNewt::World* world)
{
   root=_root;
   mWorld=world;
   root->addFrameListener(this);
   mWeaponNode=weaponNode;
   mPlayerNode=weaponNode->getParentSceneNode()->getParentSceneNode();
   weapon = _ent;
   mWeaponState = weapon->getAnimationState("idle");

   mWeaponState->setEnabled(true);
   mWeaponState->setLoop(true);
   kick_f="Run3/game/weapons/"+_ent->getName();
   cf.load(kick_f+"/weapon.cfg");
   //HUD::getSingleton().ChangeHUDOverlay(cf.getSetting("HUDOverlay",StringUtil::BLANK,"Run3/HUDAmmo"),false);
   myHUD = cf.getSetting("HUDOverlay",StringUtil::BLANK,"Run3/HUDAmmo");
   kick_f=cf.getSetting("Attack1");
   kick_w=cf.getSetting("Attack2");
   soundmgr=sound;
  // soundmgr->loadAudio("Run3/sound/"+kick_f, &kick, false);
 /* soundmgr->loadAudio("Run3/sounds//"+kick_f, &kick, false); 
  soundmgr->loadAudio("Run3/sounds//"+kick_w, &hitw, false); 
  soundmgr->setSound(kick,mPlayerNode->getPosition(),Vector3(0,0,0),Vector3(0,0,0),10000,false,false,1000,1000,1000);
  soundmgr->setSound(hitw,mPlayerNode->getPosition(),Vector3(0,0,0),Vector3(0,0,0),10000,false,false,1000,1000,1000);*/
  shooting=false;
  shootcount=0.0f;
}


void Punch::Move(const OIS::MouseEvent &arg,Ogre::Real time)
{

}
void Punch::Press(const OIS::KeyEvent &arg)
{
	
}
void Punch::Release(const OIS::KeyEvent &arg)
{

}
void Punch::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	shooting=true;
	shootcount=0.0f;
	if (id==OIS::MB_Left)
	{
		mWeaponState->setEnabled(false);
		mWeaponState = weapon->getAnimationState("attack1");
		mWeaponState->setLoop(true);
		bod = get_ray_shoot();
		soundmgr->stopAudio(kick);
		soundmgr->setSoundPosition(kick,mPlayerNode->getPosition());
		soundmgr->playAudio(kick,false);
		if (bod)
		{
			/*soundmgr->stopAudio(hitw);
			soundmgr->setSoundPosition(hitw,mPlayerNode->getPosition());
			soundmgr->playAudio(hitw,false);*/
		}
	}
	if (id==OIS::MB_Right)
	{
		mWeaponState->setEnabled(false);
		mWeaponState = weapon->getAnimationState("attack2");
		mWeaponState->setLoop(true);
		OgreNewt::Body* bod;
		bod = get_ray_shoot();
		soundmgr->stopAudio(kick);
		soundmgr->setSoundPosition(kick,mPlayerNode->getPosition());
		soundmgr->playAudio(kick,false);
		if (bod)
		{
			/*soundmgr->stopAudio(hitw);
			soundmgr->setSoundPosition(hitw,mPlayerNode->getPosition());
			soundmgr->playAudio(hitw,false);*/
		}
	}
	mWeaponState->setEnabled(true);
}

OgreNewt::Body* Punch::get_ray_shoot()
{
Ogre::Vector3 myPos = mPlayerNode->getPosition();
Ogre::Vector3 direction,end;
direction = get_direction();
end=myPos + direction * 300.0f;
OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
return shootRay.getFirstHit().mBody;
}

Ogre::Vector3 Punch::get_direction()
{
Ogre::Quaternion myOrient = mPlayerNode->getOrientation();
Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation();
Ogre::Vector3 direction2,down,down2,direction;
direction2 = myOrient2*Vector3::NEGATIVE_UNIT_Z;
down = Vector3(0,direction2.y,0);
direction = myOrient*Vector3::NEGATIVE_UNIT_Z+down;
return direction;
}


void Punch::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
   shooting=false;
   mWeaponState->setEnabled(false);
   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setEnabled(true);
}


bool Punch::frameStarted(const Ogre::FrameEvent &evt)
{
	mWeaponState->addTime(evt.timeSinceLastFrame);
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
					Vector3 dir = get_direction();
					pobj[i]->addForce(dir*10000);
				}
			}
		}
		shootcount+=evt.timeSinceLastFrame;
		if (shootcount>1.5f)
			shooting=false;
	}
	return true;
}

bool Punch::frameEnded(const Ogre::FrameEvent &evt)
{
	return true;
}