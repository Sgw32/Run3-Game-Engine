// AI Section
#include "AIManager.h"
#include "NPCManager.h"
// finished AI Section
#include "BlastWave.h"
#include "BloodEmitter.h"
#include "BulletHitManager.h"
#include "CWeapon.h"
#include "ChapterController.h"
#include "ConCommands.h"
#include "Credits.h"
#include "CrosshairOp.h"
#include "Display.h"
#include "DisplayLuaCallback.h"
#include "Energy.h"
#include "ExplosionManager.h"
#include "FacialAnimationManager.h"
#include "FadeListener.h"
#include "Generator.h"
#include "GibManager.h"
#include "HUD.h"
#include "LightPerfomanceManager.h"
#include "LoadMap.h"
#include "Loader.h"
#include "LuaHelperFunctions.h"
#include "MagicManager.h"
#include "MirrorManager.h"
#include "Modulator.h"
#include "MusicPlayer.h"
#include "OgreMagic.h"
#include "POs.h"
#include "Player.h"
#include "Run3Application.h"
#include "Run3Batcher.h"
#include "Run3Benchmark.h"
#include "Run3Input.h"
#include "Run3LoadingBar.h"
#include "Run3Shadowing.h"
#include "Run3SoundRuntime.h"
#include "SaveGame.h"
#include "SceneLoadOverlay.h"
#include "Sequence.h"
#include "SequenceLuaCallback.h"
#include "SharedLuaCallback.h"
#include "SkyManager.h"
#include "SoftwareOcclusionCulling.h"
#include "SoundManager.h"
#include "SuperFX.h"
#include "Timeshift.h"
#include "WaterManager.h"
#include "ZonePortalManager.h"
#include "global.h"
#include "oalufmod.h"
#include "ogreconsole.h"
#include <CEGUI/CEGUI.h>
#include <CEGUI/CEGUISchemeManager.h>
#include <CEGUI/CEGUIWindow.h>
#include <CEGUI/CEGUIWindowManager.h>
#include <OIS/OIS.h>
#include <OgreCEGUIRenderer.h>
#include <OgreNewt.h>
// #include "ZonePortalManager.h"

SoundManager *soundMgr;
Overlay *g_MenuOverlayUnsafe = NULL;
bool g_ShutdownRequested = NULL;
// bool global::getSingleton().GUIorGame = NULL;
bool g_MouseVisible = NULL;
bool first;
bool ingame = NULL;
bool noclip;
bool display = false;
bool menuShown;
String overlay;
// LoadMap* ml = new LoadMap;
lua_State *pL;
CEGUI::Window *sheet;
SceneManager *MapSm = NULL;
OgreNewt::World *mWorld;
OgreNewt::BasicFrameListener *mOgreNewtListener;
Player *player = new Player;
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

std::vector<managerTemplate *> managers;