#pragma once
#include <OIS/OIS.h>
#include <Ogre.h>
#include <math.h>
#include <vector>
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "POs.h"
#include <OgreNewt.h>

class Energy : public Singleton<Energy> {
public:
  Energy(OgreNewt::World *world);
  ~Energy();
  void init(Real maxDist, Real energyLoss, bool energyon);
  void setEnergy(bool on);
  void reset() { mElapsedEnergy = 1.0f; }
  void processBodyEnergy(OgreNewt::Body *bod);
  Vector3 get_pos(OgreNewt::Body *bod) {
    Vector3 pos;
    Quaternion quat;
    bod->getPositionOrientation(pos, quat);
    return pos;
  }
  OgreNewt::Body *getBodyOnDegree(Vector3 pos, int deg);

private:
  bool energy_on;
  Real mMaxDist;
  Real mEnergyLoss, mElapsedEnergy;
  OgreNewt::World *mWorld;
  vector<String> added;
};