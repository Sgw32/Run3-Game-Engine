#pragma once
#include <SkyX/SkyX.h>

#include "global.h"

class SkyManager: public Singleton<SkyManager>
{
public:
	SkyManager();
	~SkyManager();
	void init();
	void create();
	void setTimeMultiplier(Real timestep);
	void setHeightPosition(Real height);
	void setOuterRadius(Real outr);
	void setInnerRadius(Real inrr);
	void setExposure(Real exps);
	void setMie(Real mie);
	void setRayleighMultiplier(Real rayl);
	void setSampleNumber(int n);
	void updateParameters();
	void upd(const FrameEvent& evt);
	void cleanup();
private:
	SkyX::SkyX* mSkyX;
	SkyX::AtmosphereManager::Options mSkyXOptions;
};