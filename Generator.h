#pragma once

#include "Ogre.h"
#include "PhysObject.h"
#include "global.h"
#include "SharedLuaCallback.h"
//lua

class Generator: public Singleton<Generator>
{
public:
	Generator();
	~Generator();
	void init();

	static int setPhysObject(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=2)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2))
		{
			SharedLuaCallback::getSingleton().shared_genSetPO(lua_tostring(pL, 1),StringConverter::parseVector3(lua_tostring(pL, 2)));
		}
		return 1;
	}
	static int setNumberX(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isnumber(pL, 1))
		{
			SharedLuaCallback::getSingleton().shared_genSetX(lua_tonumber(pL, 1));
		}
		return 1;
	}
	static int setNumberY(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isnumber(pL, 1))
		{
			SharedLuaCallback::getSingleton().shared_genSetY(lua_tonumber(pL, 1));
		}
		return 1;
	}
	static int setNumberZ(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isnumber(pL, 1))
		{
			SharedLuaCallback::getSingleton().shared_genSetZ(lua_tonumber(pL, 1));
		}
		return 1;
	}
	static int setCenter(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			SharedLuaCallback::getSingleton().shared_genSetCenter(StringConverter::parseVector3(lua_tostring(pL, 1)));
		}
		return 1;
	}
	static int setSpace(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			SharedLuaCallback::getSingleton().shared_genSetSp(StringConverter::parseReal(lua_tostring(pL, 1)));
		}
		return 1;
	}
	static int Fire(lua_State* pL)
	{
		SharedLuaCallback::getSingleton().shared_genFire();
		return 1;
	}
	unsigned int nx,ny,nz;
Real spacing;
Vector3 pos;
String meshFile;
Vector3 scale;
	void fireGenerator();



};