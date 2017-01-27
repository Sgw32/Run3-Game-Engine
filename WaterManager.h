#pragma once
#include "Hydrax/Hydrax.h"
#include "Hydrax/Noise/Perlin/Perlin.h"
#include "Hydrax/Modules/ProjectedGrid/ProjectedGrid.h"
//#include "Manager_Template.h"
#include "global.h"

class WaterManager: public Singleton<WaterManager>
{
public:
	WaterManager();
	~WaterManager();
	void init();
	void create(Vector3 sunPosition,Vector3 sunColor);
	void create(Vector3 sunPosition,Vector3 sunColor,String fileName,Vector3 position);
	void create();
	void upd(const FrameEvent& evt);
	void cleanup();
private:
	Hydrax::Hydrax* mHydrax;
};