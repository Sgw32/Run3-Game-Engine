#include "VolumeLightManager.h"
#include "global.h"

template<> VolumeLightManager *Ogre::Singleton<VolumeLightManager>::ms_Singleton=0;

VolumeLightR3::VolumeLightR3()
{
}

VolumeLightR3::~VolumeLightR3()
{
}

void VolumeLightR3::create(Vector3 pos, Vector3 dir, Vector3 scale,unsigned int type)
{
	if (type==Direct)
	{
	}
	if (type==Halo)
	{
	}
}

void VolumeLightR3::destroy()
{
}

VolumeLightManager::VolumeLightManager()
{
	
}

VolumeLightManager::~VolumeLightManager()
{
}

void VolumeLightManager::init()
{

}


	
void VolumeLightManager::createVolumeLight(Vector3 pos, Vector3 dir, Vector3 scale,unsigned int type)
{
	VolumeLightR3* vl = new VolumeLightR3();
	vl->create(pos,dir,scale,type);
	vlights.push_back(vl);
}

void VolumeLightManager::upd(const FrameEvent& evt)
{

}

void VolumeLightManager::cleanup()
{
	vector<VolumeLightR3*>::iterator i;
	for (i=vlights.begin();i!=vlights.end();i++)
	{
		(*i)->destroy();
	}
	vlights.clear();
}

