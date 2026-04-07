#pragma once
#include "Timeshift.h"
#include "global.h"

#define BLOOD_DURATION 1.0f

class BloodEmitter : public Singleton<BloodEmitter> {
public:
  BloodEmitter();
  ~BloodEmitter();
  void init();
  void emitBlood(Vector3 pos, Vector3 size);
  void upd(const Ogre::FrameEvent &evt);

private:
  ParticleSystem *bSys;
  SceneNode *bNode;
  bool startemit;
  Real timePos;
};