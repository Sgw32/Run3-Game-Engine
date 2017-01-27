#include "ExplosionManager.h"

template<> ExplosionManager *Singleton<ExplosionManager>::ms_Singleton=0;

ExplosionManager::ExplosionManager()
{
}

ExplosionManager::~ExplosionManager()
{
}

void ExplosionManager::init()
{
	lua_State* lua = global::getSingleton().getLuaState();
	lua_register(lua, "explosion",explosion);
	lua_register(lua, "explosion2",explosion2);
}

void ExplosionManager::spawnExplosion(Vector3 pos,Vector3 scale)
{
	ExplosionEmitter* emit = new ExplosionEmitter();
	emit->init();
	emit->emitExplosion(pos,scale);
	explosions.push_back(emit);
}

void ExplosionManager::spawnExplosionEx(Vector3 pos,Vector3 scale,String a,String b)
{
	ExplosionEmitter* emit = new ExplosionEmitter();
	emit->init(b);
	emit->emitExplosion(pos,scale,a);
	explosions.push_back(emit);
}

void ExplosionManager::spawnExplosionNosound(Vector3 pos,Vector3 scale)
{
	ExplosionEmitter* emit = new ExplosionEmitter();
	emit->init();
	emit->emitExplosion(pos,scale,"");
	explosions.push_back(emit);
}

void ExplosionManager::upd(const FrameEvent& evt)
{
	vector<ExplosionEmitter*>::iterator i;
	for (i=explosions.begin();i!=explosions.end();i++)
	{
		ExplosionEmitter* emit = *i;
		emit->upd(evt);
	}
	for (i=explosions.begin();i!=explosions.end();i++)
	{
		ExplosionEmitter* emit = *i;
		if (emit->isDispose())
		{
			explosions.erase(i);
			delete emit;
		}
	}
}

void ExplosionManager::cleanup()
{
}
