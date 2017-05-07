/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include "Ogre.h"
#include "AIR3.h"
#include "Player.h"
#include "PhysObject.h"
#include "SoundManager.h"
#include "DeferredShading.h"
#include "BlastWave.h"
#include "Serial.h"
//LUA//
#include "LuaHelperFunctions.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
//LUA//
//#include "LoadMap.h"
#include <vector>
#include <OgreNewt.h>
#include "POs.h"

#define PHYSOBJECT_PLAYER 1
#define PHYSOBJECT_NPC 2
#define PHYSOBJECT_STANDART 3
#define PHYSOBJECT_STATIC 4
#define PHYSOBJECT_BREAKABLE 5
#define PHYSOBJECT_TRAIN 6
#define PHYSOBJECT_LADDER 7


#define GRAVITY -500
// Equal macro. a is number 1, b is number 2, c is a discrapency
#define PRIMERNO_RAVNO(a,b,c) (fabs(a-b)<=fabs(c))
#define DEGREE_DISCRAPENCY 2.0f

#define MAKE_SOME_GORE(s) anim->destroyNodeTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyNumericTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyVertexTrack(skel->getBone(s)->getHandle()); \
		skel->getBone(s)->setManuallyControlled(true); \
		skel->getBone(s)->setScale(0.01,0.01,0.01); 

#define RE_GORE(s) anim->destroyNodeTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyNumericTrack(skel->getBone(s)->getHandle()); \
	 anim->destroyVertexTrack(skel->getBone(s)->getHandle()); \
		skel->getBone(s)->setManuallyControlled(true); \
		skel->getBone(s)->setScale(0.01,0.01,0.01); 

#define TIME_SHIFT Timeshift::getSingleton().getTimeK()

class global:public Singleton<global>
{
public:
	global();
	~global();
	void setSceneManager(SceneManager* scenemgr);
	//void setMapLoader(LoadMap* mLoad);
	//LoadMap* getMapLoader();
	SceneManager* getSceneManager();
	void setPlayer(Player* ply);
	void setRoot(Ogre::Root* mRoot);
	void setNULLChange();
	Ogre::Root* getRoot();
	void destroyAllPhysObjects();
	void addPhysObject(PhysObject* obj);
	void setWorld(OgreNewt::World* world);
	Real getTLod(void){return tLod;}
	Real getMLod(void){return mLod;}
	void setNodeList(NodeList* nList){STORE_NODELIST}
	NodeList* getNodeList(void){GET_NODELIST}
	void setTexLod(int lod)
	{
		tLod=lod;
	}
	void setMeshLod(int lod)
	{
		mLod=lod;
	}
	void setLuaState(lua_State* state)
	{
		mLua=state;
	}

	void setSoundManager(SoundManager* sMgr){soundMgr=sMgr;}
	SoundManager* getSoundManager(void){return soundMgr;}
	
	void setCamera(Camera* camera){mCamera=camera;}
	Camera* getCamera(void){return mCamera;}

	void setWindow(RenderWindow* win){mWindow=win;}
	RenderWindow* getWindow(void){return mWindow;}

	void setOISKeyboard(OIS::Keyboard* keyboard){mKeyboard=keyboard;}
	OIS::Keyboard* getOISKeyboard(void){return mKeyboard;}

	lua_State* getLuaState(void)
	{return mLua;}
	//void set_GUIorGame(bool set);
	//bool get_GUIorGAme();
	OgreNewt::World* getWorld();
	Player* getPlayer();
	//entc event
		String entc_name;
		String entc_meshFile;
		String entc_event;
		Vector3 entc_spawn;
		bool changemap_now,GUIorGame;
		String mMap;
		Camera* mCamera;
		RenderWindow* mWindow;
		DeferredShadingSystem *iSystem;
		bool deferred,deferred_test;
		BlastWave* mBw;
		bool computer_mode;
		const OgreNewt::MaterialID* mMatDefault;
		vector<PhysObject*> pobjects;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* physicalMat;
	const OgreNewt::MaterialID* ragdollMat;
	PhysObjectMatCallback* mPhysCallback;
	RagdollMatCallback* mRagCallback;	
	Ragdoll2RagdollMatCallback* mRag2Callback;	
	CSerial* port;
	Real rx1;
	Real rx2;
	Real ry1;
	Real ry2;
	Real rz1;
	Real rz2;
	Real a;
	Real ra;
	Real rb;
	Real rc;

	ColourValue ambiLight;

	/*bool restModulator; //1
	bool restPlayer;//2
	bool restChCtrl;//3
	bool restMusicPlayer;//4
	bool restWaterManager;//5
	bool restSkyManager;//6
	bool restBloodEmitter;//7
	bool restCredits;//8
	bool restSoundRuntime;//9
	bool restExplosionManager;//10
	bool restCrosshairOperator;//11
	bool restGibManager;//12
	bool restLPrfMgr;//13
	bool restBHitMgr;//14
	bool restMMgr;//15
	bool restOthers;//16*/
	vector<bool> restData;
private:
	PRIVATE_NODELIST
	int mLod;
	int tLod;
	SceneManager* mSceneMgr;
	SoundManager* soundMgr;
	OgreNewt::World* mWorld;
	lua_State* mLua;
//	LoadMap* mLoad;
	Player* player;
	Root* root;
	OIS::Keyboard* mKeyboard;
	
};

