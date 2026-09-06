/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

class SceneLoadOverlay : public Ogre::Singleton<SceneLoadOverlay> {
public:
  SceneLoadOverlay();
  ~SceneLoadOverlay();
  void init(Ogre::Root *mRoot);
  void Add(Ogre::String overlay);
  void Show(Ogre::String overlay);
  void Show(int ter);
  void Show();
  void Hide(Ogre::String overlay);
  void Hide_all();
  void SetRandom();

private:
  Ogre::ConfigFile cf;
  int i;
  int random_iter;
  Ogre::Overlay *overlay;
  Ogre::OverlayContainer *lCont;
  std::vector<Ogre::String> overlays;
  Ogre::Root *root;
};
