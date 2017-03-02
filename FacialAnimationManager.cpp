#include "FacialAnimationManager.h"

template<> FacialAnimationManager *Ogre::Singleton<FacialAnimationManager>::ms_Singleton=0;

FacialAnimationManager::FacialAnimationManager()
{
}

FacialAnimationManager::~FacialAnimationManager()
{
}

void FacialAnimationManager::init()
{

}

void FacialAnimationManager::passFacial(FacialAnimation* fanim)
{
	fanims.push_back(fanim);
}

void FacialAnimationManager::cleanup()
{
	fanims.clear();
}

void FacialAnimationManager::upd(const FrameEvent& evt)
{
	vector<FacialAnimation*>::iterator i;
	for (i=fanims.begin();i!=fanims.end();i++)
	{
		(*i)->upd(evt.timeSinceLastFrame);	
	}
}