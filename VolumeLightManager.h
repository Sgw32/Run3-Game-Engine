#pragma once
#include <Ogre.h>
#include "Manager_Template.h"

using namespace Ogre;
using namespace std;

enum VLightType
{
	Direct,
	Halo
};

class VolumeLightR3
{
public:
	VolumeLightR3();
	~VolumeLightR3();
	void create(Vector3 pos, Vector3 dir, Vector3 scale,unsigned int type);
	void destroy();
private:
	Entity* vlight;
	SceneNode* ln;
};

class VolumeLightManager : public Singleton<VolumeLightManager>, public managerTemplate
{
public:
	VolumeLightManager();
	~VolumeLightManager();
	virtual void init();
	void createVolumeLight(Vector3 pos, Vector3 dir, Vector3 scale,unsigned int type); 

	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();

private:
	vector<VolumeLightR3*> vlights;
	
};