#pragma once
#include "global.h"
#include "Manager_Template.h"
#include "ExplosionEmitter.h"
#include "SharedLuaCallback.h"
//#include "Timeshift.h"

using namespace std;

class ExplosionManager: public Singleton<ExplosionManager>
{
public:
	ExplosionManager();
	~ExplosionManager();
	void init();
	void spawnExplosion(Vector3 pos,Vector3 scale);
	void spawnExplosionEx(Vector3 pos,Vector3 scale,String sound,String psys);
	void spawnExplosionNosound(Vector3 pos,Vector3 scale);
	void upd(const FrameEvent& evt);
	void cleanup();
	static int explosion(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=6)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL,5)&&lua_isstring(pL, 6))
		{
			Vector3 pos = Vector3(StringConverter::parseReal(lua_tostring(pL, 1)),
				StringConverter::parseReal(lua_tostring(pL, 2)),
				StringConverter::parseReal(lua_tostring(pL, 3)));
			Vector3 size = Vector3(StringConverter::parseReal(lua_tostring(pL, 4)),
				StringConverter::parseReal(lua_tostring(pL, 5)),
				StringConverter::parseReal(lua_tostring(pL, 6)));
			SharedLuaCallback::getSingleton().shared_explosion(pos,size);
		}
		return 1;
	}
	static int explosion2(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=8)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)
			&&lua_isstring(pL,5)&&lua_isstring(pL, 6)&&lua_isstring(pL, 7)&&lua_isstring(pL, 8))
		{
			Vector3 pos = Vector3(StringConverter::parseReal(lua_tostring(pL, 1)),
				StringConverter::parseReal(lua_tostring(pL, 2)),
				StringConverter::parseReal(lua_tostring(pL, 3)));
			Vector3 size = Vector3(StringConverter::parseReal(lua_tostring(pL, 4)),
				StringConverter::parseReal(lua_tostring(pL, 5)),
				StringConverter::parseReal(lua_tostring(pL, 6)));
			String snd = lua_tostring(pL, 7);
			String pSys = lua_tostring(pL, 8);
			SharedLuaCallback::getSingleton().shared_explosion(pos,size,snd,pSys);
		}
		return 1;
	}
	static int explosionNosound(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=6)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL,5)&&lua_isstring(pL, 6))
		{
			Vector3 pos = Vector3(StringConverter::parseReal(lua_tostring(pL, 1)),
				StringConverter::parseReal(lua_tostring(pL, 2)),
				StringConverter::parseReal(lua_tostring(pL, 3)));
			Vector3 size = Vector3(StringConverter::parseReal(lua_tostring(pL, 4)),
				StringConverter::parseReal(lua_tostring(pL, 5)),
				StringConverter::parseReal(lua_tostring(pL, 6)));
			SharedLuaCallback::getSingleton().shared_explosion_ns(pos,size);
		}
		return 1;
	}
private:
	std::vector<ExplosionEmitter*> explosions;
};