#pragma once
#include "OgreNewt.h"
#include "Manager_Template.h"

using namespace OgreNewt;

class BlastWave:public managerTemplate
{
public:
	BlastWave(String blastLog);
	BlastWave();
	virtual ~BlastWave();
	virtual void init();
	void blast(Vector3 pos,Vector3 size,Real power);
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
	/* from Energy.h */
	Vector3 get_pos(OgreNewt::Body* bod)
	{
		Vector3 pos;
		Quaternion quat;
		bod->getPositionOrientation(pos,quat);
		return pos;
	}
	void getBodyOnDegree(Vector3 pos,int deg);
private:
	Real mPower;
	bool startExp;
	OgreNewt::World* mWorld;
	Real blastDist;
};