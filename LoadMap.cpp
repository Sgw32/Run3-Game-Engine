/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "LoadMap.h"
#include "AIManager.h"
#include "Loader.h"
#include "Run3Benchmark.h"

template<> LoadMap *Singleton<LoadMap>::ms_Singleton=0;

LoadMap::LoadMap(){
scriptOnExit="";
}
LoadMap::~LoadMap(){

}
void LoadMap::init(SceneManager *SceneMgr,OgreNewt::World* world,Camera* camera,SoundManager* soundMgr,Player* ply)
{
  if( !SceneMgr )
  {
    throw Ogre::Exception( -1, "SceneMgr :  assertion failed at LoadMap::init","" );
  }
   scene=SceneMgr;
	
  if( !world )
  {
    throw Ogre::Exception( -1, "World :  assertion failed at PhysObject::init","" );
  }
   mWorld=world;

  if( !camera )
  {
    throw Ogre::Exception( -1, "Camera :  assertion failed at PhysObject::init","" );
  }
   mCamera=camera;
   if( !ply )
  {
    throw Ogre::Exception( -1, "Player :  assertion failed at PhysObject::init","" );
  }
   player=ply;
   sound=soundMgr;			
  dsl = new Ogre::DotSceneLoader();
  mnode = scene->getRootSceneNode()->createChildSceneNode("rootn",Vector3(0,0,0));
  mWorld->setWorldSize( Vector3(-10000, -10000, -10000), Vector3(10000, 10000, 10000) );
  dsl->texture_copy_counter=0;
}

void LoadMap::LoadM(const String &map, const String &title, const String &sdir,const String &mapdir,const String& groupName, SceneManager *mSceneMgr)
{
	if (map!="none")
		SaveGame::getSingleton().saveLast(map);

	String x1,x2,y1,y2,z1,z2;
	sound->stopAllAudio();
	String fpath;
	String spath;
	String scenefile,sequence;
	String skybox;
	ConfigFile cf;
		//String &map,String &title,String &sdir,String &mapdir,const String& groupName, SceneManager *mSceneMgr
	fpath = mapdir+"/"+Loader::getSingleton().getPrefix()+map;
	spath = mapdir+"/"+sdir;
	LogManager::getSingleton().logMessage("loading map..");
	try
	{
		cf.load(mapdir+"/"+Loader::getSingleton().getPrefix()+map+"/scene.cfg");
		LogManager::getSingleton().logMessage("map "+fpath+" loaded!");
		scenefile = cf.getSetting("Scene");
		LogManager::getSingleton().logMessage("a");
		sequence = "/" +cf.getSetting("Sequence","","none");
		LogManager::getSingleton().logMessage("b");
		scenefile = "/" +scenefile;
		LogManager::getSingleton().logMessage("c");
		x1=cf.getSetting("NewtonWX1");
		y1=cf.getSetting("NewtonWY1");
		z1=cf.getSetting("NewtonWZ1");
		x2=cf.getSetting("NewtonWX2");
		y2=cf.getSetting("NewtonWY2");
		z2=cf.getSetting("NewtonWz2");
		LogManager::getSingleton().logMessage("d");
		lOverlay=cf.getSetting("LoadingMat");
		LogManager::getSingleton().logMessage("e");
		global::getSingleton().setNULLChange();
		LogManager::getSingleton().logMessage("f");
		if (lOverlay=="random")
		{
		SceneLoadOverlay::getSingleton().SetRandom();
		}
		//DEBUG LOAD
		LogManager::getSingleton().logMessage("0");
		SceneLoadOverlay::getSingleton().Show(lOverlay);
		LogManager::getSingleton().logMessage("1");
		mWorld->setWorldSize( Vector3(atof(x1.c_str()),atof(y1.c_str()),atof(z1.c_str())),Vector3(atof(x2.c_str()),atof(y2.c_str()),atof(z2.c_str())));
		LogManager::getSingleton().logMessage("2");
		Run3Batcher::getSingleton().prepareBatch();
		LogManager::getSingleton().logMessage("3");
		dsl->parseDotScene(mapdir+"/"+Loader::getSingleton().getPrefix()+map+scenefile,groupName,mSceneMgr,mnode,"",mWorld,mCamera,sound,player);
		LogManager::getSingleton().logMessage("4");
		Run3Batcher::getSingleton().buildBatch();
		LogManager::getSingleton().logMessage("5");
		SceneLoadOverlay::getSingleton().Hide_all();
		LogManager::getSingleton().logMessage("6");
		scriptOnExit="";
		if (!Run3Benchmark::getSingleton().benchMarkEnabled())
			Sequence::getSingleton().SetSceneSeq(sequence);
		LogManager::getSingleton().logMessage("7");

	}
	catch(...)
	{
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[LoadMap] Error loading map. (Maybe doesn't exist?)");
		Run3Benchmark::getSingleton().gameStarted();
		return;
	}
	prevMap=map;
	
/*skybox = cf.getSetting("SkyBox");
OgreConsole::getSingleton().print("setting skybox...");
mSceneMgr->setSkyBox(true,skybox);*/
//player->show_overlay();
	Run3Benchmark::getSingleton().gameStarted();
}

void LoadMap::MergeM(String map)
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	dsl->mergeDotScene(map,"General",mSceneMgr,mnode,"",mWorld,mCamera,sound,player);
}

void LoadMap::LoadPrev(const String &title, const String &sdir,const String &mapdir,const String& groupName, SceneManager *mSceneMgr)
{
	
String x1,x2,y1,y2,z1,z2;
sound->stopAllAudio();
String fpath;
String spath;
String scenefile,sequence;
String skybox;
ConfigFile cf;
	//String &map,String &title,String &sdir,String &mapdir,const String& groupName, SceneManager *mSceneMgr
fpath = mapdir+"/"+Loader::getSingleton().getPrefix()+prevMap;
spath = mapdir+"/"+sdir;
OgreConsole::getSingleton().print("loading map..");
	try
	{
cf.load(mapdir+"/"+Loader::getSingleton().getPrefix()+prevMap+"/scene.cfg");
OgreConsole::getSingleton().print("map "+fpath+" loaded!");
scenefile = cf.getSetting("Scene");
sequence = "/" +cf.getSetting("Sequence","","none");
scenefile = "/" +scenefile;
x1=cf.getSetting("NewtonWX1");
y1=cf.getSetting("NewtonWY1");
z1=cf.getSetting("NewtonWZ1");
x2=cf.getSetting("NewtonWX2");
y2=cf.getSetting("NewtonWY2");
z2=cf.getSetting("NewtonWz2");
lOverlay=cf.getSetting("LoadingMat");
global::getSingleton().setNULLChange();
if (lOverlay=="random")
{
SceneLoadOverlay::getSingleton().SetRandom();
}
SceneLoadOverlay::getSingleton().Show(lOverlay);
mWorld->setWorldSize( Vector3(atof(x1.c_str()),atof(y1.c_str()),atof(z1.c_str())),Vector3(atof(x2.c_str()),atof(y2.c_str()),atof(z2.c_str())));
Run3Batcher::getSingleton().prepareBatch();
dsl->parseDotScene(mapdir+"/"+Loader::getSingleton().getPrefix()+prevMap+scenefile,groupName,mSceneMgr,mnode,"",mWorld,mCamera,sound,player);
Run3Batcher::getSingleton().buildBatch();
SceneLoadOverlay::getSingleton().Hide_all();
scriptOnExit="";
Sequence::getSingleton().SetSceneSeq(sequence);
ZonePortalManager::getSingleton().triggerAll();
Run3SoundRuntime::getSingleton().setCheckAmbientSounds();
	}
	catch(...)
	{
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[LoadMap] Error loading map. (Maybe doesn't exist?)");
		
		return;
	}
	//prevMap=map;
/*skybox = cf.getSetting("SkyBox");
OgreConsole::getSingleton().print("setting skybox...");
mSceneMgr->setSkyBox(true,skybox);*/
//player->show_overlay();
}

void LoadMap::UnloadM(SceneManager *mSceneMgr)
{
	if (!scriptOnExit.empty())
	{
		RunLuaScript(global::getSingleton().getLuaState(),scriptOnExit.c_str());
	}
	player->stopParentRelation();
	if (!mnode) 
	{
		return;
	}
    OgreConsole::getSingleton().print("clearing space...");
	mnode->detachAllObjects();
	dsl->refreshObjs();
    for (i=0; i!=dsl->dynamicObjects.size();i++)
	{
		mSceneMgr->destroyEntity(dsl->dynamicObjects[i]);
	}
	for (i=0; i!=dsl->staticObjects.size();i++)
	{
		mSceneMgr->destroyEntity(dsl->staticObjects[i]);
		if (mSceneMgr->hasParticleSystem(dsl->staticObjects[i]))
			mSceneMgr->destroyParticleSystem(dsl->staticObjects[i]);
	}
	for (i=0; i!=dsl->sounds.size();i++)
	{
		sound->releaseAudio(dsl->sounds[i]);
	}
	/*for (i=0; i!=dsl->bodies.size();i++)
	{
		dsl->bodies[i].removeForceAndTorqueCallback();
	}*/
	mnode->removeAndDestroyAllChildren();
	mSceneMgr->destroyAllLights();
	
	//mSceneMgr->destroyAllEntities();
	Run3Batcher::getSingleton().destroyBatch();
	Sequence::getSingleton().init_reload();
	global::getSingleton().destroyAllPhysObjects();
	SkyManager::getSingleton().cleanup();
	EventEntC::getSingleton().cleanup();
	POs::getSingleton().clear();
	CWeapon::getSingleton().stripall();
	Display::getSingleton().reset();
	player->reloadPlayer();
	HUD::getSingleton().resetTextColor();
	HUD::getSingleton().resetTextSize();
	AIManager::getSingleton().cleanupAllNodes();
	MirrorManager::getSingleton().destroyAllMirrors();
	MusicPlayer::getSingleton().cleanup();
	Credits::getSingleton().cleanup();
	TextureManager::getSingleton().unloadUnreferencedResources();
	//global::getSingleton().iSystem->destroyAllMLights();
	WaterManager::getSingleton().cleanup();
	LightPerfomanceManager::getSingleton().cleanup();
	MeshDecalMgr::getSingleton().cleanup();
	ZonePortalManager::getSingleton().cleanup();
	FacialAnimationManager::getSingleton().cleanup();
	Run3SoundRuntime::getSingleton().clearAmbientSounds();
	sound->releaseAllAudio();
	sound->releaseBuffers();
	NPCManager::getSingleton().setStep(0.05f);
}
