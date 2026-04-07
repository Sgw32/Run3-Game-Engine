/*A PART OF RUN3 GAME ENGINE
Sgw32
*/
#pragma once

#include "FacialAnimation.h"
#include "Manager_Template.h"
#include "Ogre.h"
#include "global.h"

using namespace Ogre;
using namespace std;

class FacialAnimationManager : public Singleton<FacialAnimationManager>,
                               public managerTemplate {
public:
  FacialAnimationManager(String manName) {
    LogManager::getSingleton().logMessage(manName + " manager initialized!");
  }
  FacialAnimationManager();
  virtual ~FacialAnimationManager();
  virtual void init();
  void passFacial(FacialAnimation *fanim);

  virtual void upd(const FrameEvent &evt);
  virtual void cleanup();

private:
  vector<FacialAnimation *> fanims;
};