#include "SharedLuaCallback.h"
#include "SuperFX.h"
#include "ExplosionManager.h"
#include "Credits.h"
#include "Generator.h"

template<> SharedLuaCallback *Ogre::Singleton<SharedLuaCallback>::ms_Singleton=0;

SharedLuaCallback::SharedLuaCallback()
{

}

SharedLuaCallback::~SharedLuaCallback()
{

}

void SharedLuaCallback::shared_toggleNightvision()
{
	SuperFX::getSingleton().toggleNightvision();
}

void SharedLuaCallback::shared_explosion(Vector3 pos, Vector3 size)
{
	ExplosionManager::getSingleton().spawnExplosion(pos,size);
}

void SharedLuaCallback::shared_explosion(Vector3 pos, Vector3 size,String a,String b)
{
	ExplosionManager::getSingleton().spawnExplosionEx(pos,size,a,b);
}

void SharedLuaCallback::shared_explosion_ns(Vector3 pos, Vector3 size)
{
	ExplosionManager::getSingleton().spawnExplosionNosound(pos,size);
}


void SharedLuaCallback::shared_startCredits(String fileName)
{
	Credits::getSingleton().start(fileName);
}

void SharedLuaCallback::shared_genSetPO(String fileName,Vector3 size)
{
	Generator::getSingleton().meshFile=fileName;
	Generator::getSingleton().scale=size;
}

void SharedLuaCallback::shared_genSetX(unsigned int x)
{
Generator::getSingleton().nx=x;
}

void SharedLuaCallback::shared_genSetY(unsigned int y)
{
Generator::getSingleton().ny=y;
}

void SharedLuaCallback::shared_genSetZ(unsigned int z)
{
Generator::getSingleton().nz=z;
}

void SharedLuaCallback::shared_genSetSp(Real spacing)
{
Generator::getSingleton().spacing=spacing;
}

void SharedLuaCallback::shared_genSetCenter(Vector3 center)
{
Generator::getSingleton().pos=center;
}

void SharedLuaCallback::shared_genFire()
{
Generator::getSingleton().fireGenerator();
}