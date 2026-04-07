#pragma once

#include "Manager_Template.h"
#include <Ogre.h>

using namespace Ogre;

class CrosshairOperator : public Singleton<CrosshairOperator>,
                          public managerTemplate {
public:
  CrosshairOperator();
  ~CrosshairOperator();
  virtual void init();
  Vector3 getCrosshairShift(void);
  Vector3 getCrosshairShift(Real x, Real y);
  virtual void upd(const FrameEvent &evt);
  virtual void cleanup();

private:
  Real speedOfReturn;
  bool startDecreasing;
  Real pauseBeforeDecrease;
  Real x, y;
};