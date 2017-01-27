/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
/*Console commands for Run3 Game Engine
  2010 (c) Sgw32 Corporation
*/
///////////////////////////
#pragma once
#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>
#include <vector>
#include "ogreconsole.h"
#include "Player.h"
#include "FadeListener.h"
#include "CWeapon.h"
#include "HUD.h"
#include "SuperFX.h"
#include "Timeshift.h"

using namespace Ogre;
using namespace std;




class ConCommands: public Singleton<ConCommands>
{
public:
   ConCommands();
   ~ConCommands();
   void init(Player* ply,Root* mRoot);
   void Compile();
   static void Noclip(vector<String>& param);
   static void Fade(vector<String>& param);
   static void a1gfdo4abfs(vector<String>& param);
   static void Give(vector<String>& param);
   static void SetRed(vector<String>& param);
   static void SetGreen(vector<String>& param);
   static void SetBlue(vector<String>& param);
   static void Gluk();
   static void toggleOldTv(vector<String>& param);
   static void toggleNightvision(vector<String>& param);
   static void changeFOV(vector<String>& param);
   static void printStats(vector<String>& param);
   static void Commodore(vector<String>& param);
   static void testCompos(vector<String>& param);
   static void testCompos2(vector<String>& param);
   static void drug1(vector<String>& param);
   static void drug2(vector<String>& param);
   static void drug3(vector<String>& param);
   static void lua(vector<String>& param);
   static void buyWeapon(vector<String>& param);
   static void god(vector<String>& param);
   static void ts(vector<String>& param);
   static void disssao(vector<String>& param);
   static void enablessao(vector<String>& param);
   static void timesh(vector<String>& param);
   static void spawnzom(vector<String>& param);
private:
	SceneManager* mSceneMgr;
	Root* mRoot;
	bool luaactivated;
	static Player* play;
};