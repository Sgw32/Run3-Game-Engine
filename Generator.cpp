#include "Generator.h"

template<> Generator *Singleton<Generator>::ms_Singleton=0;

Generator::Generator()
{
}

Generator::~Generator()
{
}

void Generator::init()
{
	lua_State* pLuaState = global::getSingleton().getLuaState();
	lua_register(pLuaState, "gen_setPhysObject",setPhysObject);
	lua_register(pLuaState, "gen_setNumberX",setNumberX);
	lua_register(pLuaState, "gen_setNumberY",setNumberY);
	lua_register(pLuaState, "gen_setNumberZ",setNumberZ);
	lua_register(pLuaState, "gen_setCenter",setCenter);
	lua_register(pLuaState, "gen_setSpace",setSpace);
	lua_register(pLuaState, "gen_Fire",Fire);
}

void Generator::fireGenerator()
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	OgreNewt::World* mWorld = global::getSingleton().getWorld();
	Vector3 dcorner = pos-Vector3(nx*spacing/2,ny*spacing/2,nz*spacing/2);
	LogManager::getSingleton().logMessage("Generator: Fire! "+meshFile);
	for (unsigned int i=0;i!=nx;i++)
	{
		for (unsigned int j=0;j!=ny;j++)
		{
			for (unsigned int k=0;k!=nz;k++)
			{
				Vector3 pom = dcorner+Vector3(i*spacing,j*spacing,k*spacing);
				LogManager::getSingleton().logMessage("Generator: gen"+StringConverter::toString(pom));
				SceneNode* pParent = mSceneMgr->getRootSceneNode()->createChildSceneNode(pom);
				pParent->setScale(scale);
				PhysObject* phys = new PhysObject;
				try
				{
					MeshManager::getSingleton().load(meshFile,"General");
					phys->init(mSceneMgr,mWorld);
					phys->CreateObject(pParent->getName(),meshFile,pParent,100.0,false,"");
					global::getSingleton().addPhysObject(phys);
				}
				catch(Ogre::Exception &/*e*/)
				{
					LogManager::getSingleton().logMessage("[Generator] Error loading a physics object!");
				}
			}
		}
	}
}
