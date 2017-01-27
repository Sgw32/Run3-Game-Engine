/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "OgreConsole.h"
#include <OIS/OIS.h>
#include <stdlib.h>
#include <time.h>

class SceneLoadOverlay: public Singleton<SceneLoadOverlay>
{
public:
   SceneLoadOverlay();
   ~SceneLoadOverlay();
   void init(Root* mRoot);
   void Add(String overlay);
   void Show(String overlay);
   void Show(int ter);
   void Show();
   void Hide(String overlay);
   void Hide_all();
   void SetRandom();
private:
	ConfigFile cf;
	int i;
	int random_iter;
	Overlay* overlay;
	OverlayContainer* lCont;
	vector<String> overlays;
	Root* root;
};