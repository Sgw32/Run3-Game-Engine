#pragma once
#include "Ogre.h"
using namespace Ogre;

struct lightsLess
{
    bool operator()(const Light* l1, const Light* l2) const
    {
        if (l1 == l2)
            return false;
        return l1->tempSquareDist < l2->tempSquareDist;
    }
};

class PSSMShadowListener:public Ogre::SceneManager::Listener
{
   
public:
    PSSMShadowListener(Ogre::SceneManager *sm,Ogre::Light *l,Ogre::ShadowCameraSetupPtr s,Ogre::Camera *cam);
    virtual ~PSSMShadowListener() {}
	virtual void shadowTexturesUpdated(size_t numberOfShadowTextures){};
	virtual void shadowTextureCasterPreViewProj(Ogre::Light* light,Ogre::Camera* camera, size_t iteration);
	virtual void shadowTextureReceiverPreViewProj(Ogre::Light* light,Ogre::Frustum* frustum){};
    virtual bool sortLightsAffectingFrustum(Ogre::LightList& lightList);


	virtual void preFindVisibleObjects(SceneManager* source, 
		Ogre::SceneManager::IlluminationRenderStage irs, Viewport* v){};
	virtual void postFindVisibleObjects(SceneManager* source, 
		Ogre::SceneManager::IlluminationRenderStage irs, Viewport* v){};
private:
	 Ogre::Light *light;
    Ogre::ShadowCameraSetupPtr setup;
    Ogre::Camera *view_camera;        // NOT shadow camera!
    Ogre::SceneManager *sceneMgr;
    mutable int split_index;
};