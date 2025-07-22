// AI Section
#include "AIManager.h"
#include "NPCManager.h"
// finished AI Section
#include "MagicManager.h"
#include "OgreMagic.h"
#include <CEGUI/CEGUISchemeManager.h>
#include <CEGUI/CEGUIWindowManager.h>
#include <CEGUI/CEGUIWindow.h>
#include <CEGUI/CEGUI.h>
#include <OIS/OIS.h>
#include <OgreCEGUIRenderer.h>
#include <OgreNewt.h>
#include "Run3Application.h"
#include "Run3LoadingBar.h"
#include "Display.h"
#include "DisplayLuaCallback.h"
#include "SequenceLuaCallback.h"
#include "SharedLuaCallback.h"
#include "SoundManager.h"
#include "ogreconsole.h"
#include "LoadMap.h"
#include "Player.h"
#include "oalufmod.h"
#include "FadeListener.h"
#include "ConCommands.h"
#include "CWeapon.h"
#include "HUD.h"
#include "Loader.h"
#include "SceneLoadOverlay.h"
#include "Sequence.h"
#include "global.h"
#include "SuperFX.h"
#include "Run3Input.h"
#include "POs.h"
#include "Energy.h"
#include "LuaHelperFunctions.h"
#include "Run3Shadowing.h"
#include "Run3SoundRuntime.h"
#include "MirrorManager.h"
#include "Modulator.h"
#include "MusicPlayer.h"
#include "WaterManager.h"
#include "Credits.h"
#include "BloodEmitter.h"
#include "ExplosionManager.h"
#include "BlastWave.h"
#include "CrosshairOp.h"
#include "GibManager.h"
#include "LightPerfomanceManager.h"
#include "BulletHitManager.h"
#include "Generator.h"
#include "Run3Batcher.h"
#include "ZonePortalManager.h"
#include "Timeshift.h"
#include "SkyManager.h"
#include "SaveGame.h"
#include "ChapterController.h"
#include "SoftwareOcclusionCulling.h"
#include "PolygonOcclusionCulling.h"
#include "FacialAnimationManager.h"
#include "Run3Benchmark.h"
//#include "ZonePortalManager.h"

SoundManager* soundMgr;
Overlay* g_MenuOverlayUnsafe = NULL;
bool g_ShutdownRequested = NULL;
//bool global::getSingleton().GUIorGame = NULL;
bool g_MouseVisible = NULL;
bool first;
bool ingame = NULL;
bool noclip;
bool display = false;
bool menuShown;
String overlay;
//LoadMap* ml = new LoadMap;
lua_State* pL;
CEGUI::Window* sheet;
SceneManager* MapSm = NULL;
OgreNewt::World* mWorld;
OgreNewt::BasicFrameListener* mOgreNewtListener;
Player* player = new Player;
String backMap;
StringVector backMaps;
/*test maps*/
String chapter01;
unsigned int audioId;
unsigned int backMaps_number;
Viewport *gLeftViewport = NULL;
Viewport *gRightViewport = NULL;
ChapterController mChCtrl;
NODELIST_SPAWN

std::vector<managerTemplate*> managers;