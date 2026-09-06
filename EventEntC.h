/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <vector>

class EventEntC : public Ogre::Singleton<EventEntC> {
public:
  EventEntC();
  ~EventEntC();
  void init(Ogre::SceneManager *scene);
  void ent(Ogre::String name, Ogre::String file, Ogre::String event,
           Ogre::Vector3 pos);
  void cleanup();

private:
  std::vector<Ogre::String> entc_name;
  Ogre::SceneManager *mSceneMgr;
};
