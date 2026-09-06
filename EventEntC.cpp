/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "EventEntC.h"

template <> EventEntC *Ogre::Singleton<EventEntC>::msSingleton = nullptr;

EventEntC::EventEntC() {}

EventEntC::~EventEntC() {}

void EventEntC::init(Ogre::SceneManager *scene) { mSceneMgr = scene; }
void EventEntC::ent(Ogre::String name, Ogre::String file, Ogre::String event,
                    Ogre::Vector3 pos) {
  if (event == "spawn") {
    Ogre::Entity *ent = mSceneMgr->createEntity(name, file);
    Ogre::SceneNode *node =
        mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
    node->attachObject(ent);
  }
  entc_name.push_back(name);
}
void EventEntC::cleanup() {
  int i;
  for (i = 0; i != entc_name.size(); i++) {
    if (mSceneMgr->hasEntity(entc_name[i])) {
      // mSceneMgr->getEntity(entc_name[i])->getParentSceneNode()->detachAllObjects();
      mSceneMgr->destroySceneNode(
          mSceneMgr->getEntity(entc_name[i])->getParentSceneNode());
      mSceneMgr->destroyEntity(entc_name[i]);
    }
  }
  entc_name.clear();
}
