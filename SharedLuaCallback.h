#pragma once
#include <Ogre.h>

using namespace Ogre;

class SharedLuaCallback:public Ogre::Singleton<SharedLuaCallback>
{
public:
	SharedLuaCallback();
	~SharedLuaCallback();
	void shared_toggleNightvision();
	void shared_explosion(Vector3 pos, Vector3 size);
	void shared_explosion(Vector3 pos, Vector3 size,String a,String b);
	void shared_explosion_ns(Vector3 pos, Vector3 size);
	void shared_startCredits(String fileName);
	void shared_genSetPO(String fileName,Vector3 size);
	void shared_genSetX(unsigned int x);
	void shared_genSetY(unsigned int y);
	void shared_genSetZ(unsigned int z);
	void shared_genSetSp(Real spacing);
	void shared_genSetCenter(Vector3 center);
	void shared_genFire();
private:

};