/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "global.h"
#include "PhysObjectMatCallback.h"

template<> global *Singleton<global>::ms_Singleton=0;

global::global()
{
	changemap_now = false;
	GUIorGame=true;
	mLod=tLod=0;
	deferred=false;
	computer_mode=false;
	port=0;
	for(unsigned int i=0;i!=20;i++)
		restData.push_back(false);
}

global::~global()
{
}

void global::setPlayer(Player* ply)
{
	player=ply;
}

void global::setSceneManager(SceneManager* scene)
{
	mSceneMgr = scene;
}

/*void global::setMapLoader(LoadMap* load);
{
	mLoad = load;
}

LoadMap* global::getMapLoader()
{
	return mLoad;
}*/

Player* global::getPlayer()
{
	return player;
}

void global::setWorld(OgreNewt::World* world)
{
	mWorld = world;
	mPhysCallback = new PhysObjectMatCallback();
	mRagCallback = new RagdollMatCallback();
	mRag2Callback = new Ragdoll2RagdollMatCallback();
  mMatDefault = mWorld->getDefaultMaterialID();
  physicalMat= new OgreNewt::MaterialID( mWorld );
  ragdollMat= new OgreNewt::MaterialID( mWorld );
  mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, physicalMat );
  mMatPair->setContactCallback( mPhysCallback );

  mMatPair = new OgreNewt::MaterialPair( mWorld, mMatDefault, ragdollMat );
  mMatPair->setContactCallback( mRagCallback );
  mMatPair = new OgreNewt::MaterialPair( mWorld, physicalMat, ragdollMat );
  mMatPair->setContactCallback( mRagCallback );
  mMatPair = new OgreNewt::MaterialPair( mWorld, ragdollMat, ragdollMat );
  mMatPair->setContactCallback( mRag2Callback );
}

OgreNewt::World* global::getWorld()
{
	return mWorld;
}

void global::setRoot(Ogre::Root* mRoot)
{
root=mRoot;
}
Ogre::Root* global::getRoot()
{
	return root;
}

SceneManager* global::getSceneManager()
{
	return mSceneMgr;
}

void global::addPhysObject(PhysObject* obj)
{
	pobjects.push_back(obj);
	POs::getSingleton().addPO(obj);
}
void global::setNULLChange()
{
	changemap_now = false;
					mMap = "";
}
void global::destroyAllPhysObjects()
{
	// MAYBE UNSTABLE!!!
	vector<PhysObject*>::iterator i;
	for (i=pobjects.begin(); i!=pobjects.end();i++)
	{
		PhysObject* obj = (*i);
		if (obj)
		{
		obj->DeleteObject();
		delete obj;
		LogManager::getSingleton().logMessage("Physobject deleted!");
		}
	}
	pobjects.clear();
}