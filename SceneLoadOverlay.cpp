#include "SceneLoadOverlay.h"

template <>
SceneLoadOverlay *Ogre::Singleton<SceneLoadOverlay>::msSingleton = nullptr;

SceneLoadOverlay::SceneLoadOverlay() {}

SceneLoadOverlay::~SceneLoadOverlay() {}

void SceneLoadOverlay::init(Ogre::Root *mRoot) {
  srand(time(NULL));
  cf.load("run3/game/loadingscrs/loadingscrs.cfg");
  Ogre::ConfigFile::SettingsMultiMap *settings =
      cf.getSectionIterator().getNext();
  Ogre::ConfigFile::SettingsMultiMap::iterator b;
  settings = cf.getSectionIterator().getNext();
  Ogre::String curLab;
  overlay = Ogre::OverlayManager::getSingleton().getByName("Run3/TD01");
  lCont = overlay->getChild("Run3/TD01Panel");
  for (b = settings->begin(); b != settings->end(); ++b) {
    curLab = b->first;
    Add(cf.getSetting(curLab));
  }
  root = mRoot;
}

void SceneLoadOverlay::Add(Ogre::String overlay) {
  //	overlays.push_back(OverlayManager::getSingleton().getByName(overlay));
  overlays.push_back(overlay);
}

void SceneLoadOverlay::Show(Ogre::String over) {
  Ogre::LogManager::getSingleton().logMessage("11");
  lCont->setMaterialName(over);
  Ogre::LogManager::getSingleton().logMessage("12");
  overlay->show();
  Ogre::LogManager::getSingleton().logMessage("13");
  root->renderOneFrame();
  Ogre::LogManager::getSingleton().logMessage("14");
}

void SceneLoadOverlay::Show() {
  overlay->show();
  root->renderOneFrame();
}

void SceneLoadOverlay::Show(int iter) {
  lCont->setMaterialName(overlays.at(iter));
  overlay->show();
  root->renderOneFrame();
}

void SceneLoadOverlay::Hide(Ogre::String over) { overlay->hide(); }

void SceneLoadOverlay::Hide_all() { overlay->hide(); }

void SceneLoadOverlay::SetRandom() {
  random_iter = rand() % overlays.size() + 1;
  Hide_all();
  Show(random_iter);
}
