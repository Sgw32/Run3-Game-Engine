#include "generic_lua_weapon.h"
#include "global.h"
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////Original class by:Sgw32				       ////////
///////////////Copyright(c) 2010 Sgw32 Corporation		   ////////
///////////////////////////////////////////////////////////////////
// Generic lua weapon										///////
// It is used for outside lua-weapon-programming			///////
///////////////////////////////////////////////////////////////////

generic_lua_weapon::generic_lua_weapon()
{
bod=0;
shooting=false;
}

generic_lua_weapon::~generic_lua_weapon()
{

}

void generic_lua_weapon::init(Ogre::Root *_root,SceneManager* sceneMgr,SceneNode* weaponNode,Entity* _ent,SoundManager* sound,OgreNewt::World* world)
{
	pLuaState=global::getSingleton().getLuaState();
   root=_root;
   mWorld=world;
   mSceneMgr=sceneMgr;
   root->addFrameListener(this);
   mWeaponNode=weaponNode;
   mPlayerNode=weaponNode->getParentSceneNode()->getParentSceneNode();
   weapon = _ent;
   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setWeight(0.1);
   mWeaponState->setEnabled(true);
   mWeaponState->setLoop(true);
   shoot="Run3/game/weapons/"+_ent->getName();
   cf.load(shoot+"/weapon.cfg");
   shoot=cf.getSetting("Attack1");
   soundmgr=sound;
   soundmgr->loadAudio("Run3/sounds//"+shoot, &shoots, false); 
   energyid=1;
   d_p= new DecalProjector;
   d_p->createProjector(mWeaponNode);
	
   script="Run3/game/weapons/"+_ent->getName();
   script+="/";
	script+=cf.getSetting("luaScript");
   // Lua Functions
   keyPress=cf.getSetting("kPFunc");
   keyPress+="_"+_ent->getName();
   keyRelease=cf.getSetting("kRFunc");
	keyRelease+="_"+_ent->getName();
   mousePressL=cf.getSetting("mPLFunc");
   mousePressL+="_"+_ent->getName();
   mousePressR=cf.getSetting("mPRFunc");
   mousePressR+="_"+_ent->getName();
   mousePressM=cf.getSetting("mPMFunc");
   mousePressM+="_"+_ent->getName();
   mouseReleaseL=cf.getSetting("mRLFunc");
   mouseReleaseL+="_"+_ent->getName();
   mouseReleaseR=cf.getSetting("mRRFunc");
   mouseReleaseR+="_"+_ent->getName();
   mouseReleaseM=cf.getSetting("mRMFunc");
   mouseReleaseM+="_"+_ent->getName();
   mouseMove=cf.getSetting("mMFunc");
   mouseMove+="_"+_ent->getName();
	updateFunc=cf.getSetting("UpdFunc");
   updateFunc+="_"+_ent->getName(); 
   initLua=cf.getSetting("initLua");
   initLua+="_"+_ent->getName();
   
	primaryAttack=cf.getSetting("primaryAttack");
   primaryAttack+="_"+_ent->getName();
	RunLuaScript(pLuaState,script.c_str());
	luaFuncInit();
}

void generic_lua_weapon::luaFuncInit()
{
	lua_getglobal(pLuaState, initLua.c_str());

   //check that it is there
   if (lua_isfunction(pLuaState, -1))
   {
   lua_call(pLuaState, 0, 0);
   }
}

void generic_lua_weapon::Move(const OIS::MouseEvent &arg,Ogre::Real time)
{
	lua_getglobal(pLuaState, mouseMove.c_str());

   if (lua_isfunction(pLuaState, -1))
   {
	lua_pushnumber(pLuaState,arg.state.X.rel);
	lua_pushnumber(pLuaState,arg.state.Y.rel);
   lua_call(pLuaState, 2, 0);
   }
}
void generic_lua_weapon::Press(const OIS::KeyEvent &arg)
{
	lua_getglobal(pLuaState, keyPress.c_str());

   if (lua_isfunction(pLuaState, -1))
   {
	lua_pushnumber(pLuaState,arg.key);
   lua_call(pLuaState, 1, 0);
   }
}
void generic_lua_weapon::Release(const OIS::KeyEvent &arg)
{
	lua_getglobal(pLuaState, keyRelease.c_str());

   if (lua_isfunction(pLuaState, -1))
   {
	lua_pushnumber(pLuaState,arg.key);
   lua_call(pLuaState, 1, 0);
   }
}
void generic_lua_weapon::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{

	if (id==OIS::MB_Left)
	{
	//	shooting=true;
		mWeaponState->setEnabled(false);
		mWeaponState->setLength(0.5f);
		mWeaponState = weapon->getAnimationState("attack01");
		mWeaponState->setLoop(true);
		mWeaponState->setEnabled(true);
		soundmgr->stopAudio(shoots);
		soundmgr->setSoundPosition(shoots,mPlayerNode->getPosition());
		soundmgr->playAudio(shoots,false);

		lua_getglobal(pLuaState, mousePressL.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	}

	if (id==OIS::MB_Right)
	{
		lua_getglobal(pLuaState, mousePressR.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	}

	if (id==OIS::MB_Middle)
	{
		lua_getglobal(pLuaState, mousePressM.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	}
	mWeaponState->setEnabled(true);
}

OgreNewt::Body* generic_lua_weapon::get_ray_shoot()
{
Ogre::Vector3 myPos = mPlayerNode->getPosition();
Ogre::Vector3 direction,end;
direction = get_direction();
end=myPos + direction * 10000.0f;
OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
return shootRay.getFirstHit().mBody;
}

Ogre::Vector3 generic_lua_weapon::get_direction()
{
Ogre::Quaternion myOrient = mPlayerNode->getOrientation();
Ogre::Quaternion myOrient2 = mWeaponNode->getParentSceneNode()->_getDerivedOrientation();
Ogre::Vector3 direction2,down,down2,direction;
direction2 = myOrient2*Vector3::NEGATIVE_UNIT_Z;
down = Vector3(0,direction2.y,0);
direction = myOrient*Vector3::NEGATIVE_UNIT_Z+down;
return direction;
}


void generic_lua_weapon::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{

	if (id==OIS::MB_Right)
	{
		lua_getglobal(pLuaState, mouseReleaseR.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	}

	if (id==OIS::MB_Middle)
	{
		lua_getglobal(pLuaState, mouseReleaseM.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	}

	if (id==OIS::MB_Left)
	{
		lua_getglobal(pLuaState, mouseReleaseL.c_str());

		if (lua_isfunction(pLuaState, -1))
		{
			lua_pushnumber(pLuaState,arg.state.X.rel);
			lua_pushnumber(pLuaState,arg.state.Y.rel);
			lua_call(pLuaState, 2, 0);
		}
	//shooting=false;
	bod=0;
	}
   mWeaponState->setEnabled(false);
   mWeaponState->setLength(5.0f);
   mWeaponState = weapon->getAnimationState("idle");
   mWeaponState->setEnabled(true);
}

void generic_lua_weapon::In()
{
	mWeaponState->setEnabled(false);
	mWeaponState = weapon->getAnimationState("in");
	mWeaponState->setLoop(false);
	mWeaponState->setEnabled(true);
	in=true;
}

bool generic_lua_weapon::frameStarted(const Ogre::FrameEvent &evt)
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
	/*if (shooting)
		bod = get_ray_shoot();
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
				
			}
		}
		}
	}*/

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
				lua_getglobal(pLuaState, primaryAttack.c_str());

				if (lua_isfunction(pLuaState, -1))
				{
						//lua_pushnumber(pLuaState,evt.timeSinceLastFrame);
						lua_call(pLuaState, 0, 0);
				}	
			}
		}
		}
	}


	lua_getglobal(pLuaState, updateFunc.c_str());

	if (lua_isfunction(pLuaState, -1))
	{
			lua_pushnumber(pLuaState,evt.timeSinceLastFrame);
			lua_call(pLuaState, 1, 0);
	}

	lua_getglobal(pLuaState, updateFunc.c_str());

	if (lua_isfunction(pLuaState, -1))
	{
			lua_pushnumber(pLuaState,evt.timeSinceLastFrame);
			lua_call(pLuaState, 1, 0);
	}

	mWeaponState->addTime(evt.timeSinceLastFrame);
	return true;
}

bool generic_lua_weapon::frameEnded(const Ogre::FrameEvent &evt)
{
	return true;
}