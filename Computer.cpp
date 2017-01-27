#include "Computer.h"
#include "CWeapon.h"

Computer::Computer()
{
mActMagic=false;
mFov=90;
allowVirtualDisplay=true;
notice=true;
mNearLua="";
}


Computer::~Computer()
{

}

void Computer::init(SceneNode* cComp,int type,String fileName)
{
	//global::getSingleton().getRoot()->addFrameListener(this);
	mComp=cComp;
	Vector3	size=cComp->getScale();
	mWorld = global::getSingleton().getWorld();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld,cComp,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( cComp );
	bod->setName(cComp->getName());
	bod->setPositionOrientation( cComp->getPosition(), cComp->getOrientation() );
	pLuaState = global::getSingleton().getLuaState();
	luaFlName=fileName;
	delete col;
	
}

void Computer::init(SceneNode* cComp,int type,String fileName,String dispMatName)
{
	//global::getSingleton().getRoot()->addFrameListener(this);
	mDispMatName=dispMatName;
	mComp=cComp;
	Vector3	size=cComp->getScale();
	mWorld = global::getSingleton().getWorld();
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld,cComp,true);
	bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( cComp );
	bod->setName(cComp->getName());
	bod->setPositionOrientation( cComp->getPosition(), cComp->getOrientation() );
	pLuaState = global::getSingleton().getLuaState();
	luaFlName=fileName;
	delete col;
	
}

void Computer::power()
{
	if (!Display::getSingleton().isVisible())
	{
		mFov=global::getSingleton().getPlayer()->getFOV();
		LogManager::getSingleton().logMessage("Computer: player FOV is:");
		LogManager::getSingleton().logMessage(StringConverter::toString(mFov));
		MagicManager::getSingleton().activateMagic(true);
		LogManager::getSingleton().logMessage("Computer turned on!");
		Display::getSingleton().curcomputer=this;
		Display::getSingleton().setAllowVirtualDisplay(allowVirtualDisplay);
		Display::getSingleton().show();
		global::getSingleton().computer_mode=true;
		CWeapon::getSingleton().hide_all();
		global::getSingleton().getPlayer()->freeze();
		RunLuaScript(pLuaState, luaFlName.c_str());
		HUD::getSingleton().Hide();
	}
}

void Computer::logoff()
{
	if (Display::getSingleton().isVisible())
	{
		LogManager::getSingleton().logMessage("Computer turned off!");
		MagicManager::getSingleton().activateMagic(false);
	LogManager::getSingleton().logMessage("Computer turned off2!");
	Display::getSingleton().hide();
	if (!(msLua==""))
	{
		RunLuaScript(pLuaState, msLua.c_str());
	}
	RunLuaScript(pLuaState, "run3/lua/resetcomp.lua");
	global::getSingleton().computer_mode=false;
	LogManager::getSingleton().logMessage("Computer: last FOV is. Reverting changes.");
	LogManager::getSingleton().logMessage(StringConverter::toString(mFov));
	global::getSingleton().getPlayer()->unfreeze2();
	global::getSingleton().getPlayer()->changeFOV(mFov);
	HUD::getSingleton().Show();
	}
	
}

void Computer::explode()
{
	//explosions are not implemented
}

void Computer::reset()
{
	RunLuaScript(pLuaState, "run3/lua/resetcomp.lua");
	RunLuaScript(pLuaState, luaFlName.c_str());
}

bool Computer::frameStarted(const Ogre::FrameEvent &evt)
{
if (bod->getUsing()) //user pressed "E" near the computer
{
  power();
}
if (notice)
{
	Vector3 dist = global::getSingleton().getPlayer()->get_location()-mComp->getPosition();
	//LogManager::getSingleton().logMessage(StringConverter::toString(dist.length()));
	if (dist.length()<200.0f)
	{
		LogManager::getSingleton().logMessage("Computer: near.");
		if (!mNearLua.empty())
		{
			RunLuaScript(pLuaState, mNearLua.c_str());
		}
		notice=false;
	}
}
/*if (!bod->getUsing()) //user pressed "E" near the computer
{
  hide();
}*/
return true;
}

void Computer::del()
{
		//global::getSingleton().getRoot()->removeFrameListener(this);
		delete bod;
		Ogre::MovableObject* aEnt = mComp->getAttachedObject(0);
		if (global::getSingleton().getSceneManager()->hasEntity(aEnt->getName()))
		{
		mComp->detachAllObjects();
		global::getSingleton().getSceneManager()->destroySceneNode(mComp);
		global::getSingleton().getSceneManager()->destroyEntity(aEnt->getName());
		}
}

bool Computer::frameEnded(const Ogre::FrameEvent &evt)
{
return true;
}
