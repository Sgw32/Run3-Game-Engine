/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
//Fast fading			///
//(c) 2026 PHOL-LABS Kft ///
///////////////////////////
#pragma once
#include "fader.h"
#include <OIS/OIS.h>
#include <Ogre.h>
#include <OgreFrameListener.h>
#include <list>
#include <vector>

using namespace Ogre;
using namespace std;

class FadeListener : public Singleton<FadeListener>, FrameListener {
public:
  FadeListener();
  ~FadeListener();
  void init(Ogre::Root *root);
  void rebuild();
  void rebuild(String mat, bool return1);
  void setDuration(Real dur);
  void setDurationF(Real durf);
  void setWait(Real ait) { wait = ait; }
  void setFadeColor(ColourValue col);
  void startIN();
  void startOUT();
  virtual bool frameStarted(const Ogre::FrameEvent &evt);

private:
  Fader *fader;
  bool fadeb;
  bool inited;
  bool returnafter;
  Real duration;
  Real durationf;
  Real wait;
  Ogre::Root *root;
};