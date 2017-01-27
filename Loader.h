/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>
#include <list>
#include "CWeapon.h"
#include "SuperFX.h"
#include "global.h"
#include "StereoManager.h"
#include "GlowMaterialListener.h"
#include "SaveGame.h"
#include "PSSMShadowListener.h"
//#include "OgreShadowCameraSetupLiSPSM.h"

#define SCREEN_DIST 10
#define EYES_SPACING 0.06

#define EYESSPACING_SPEED 0.2
#define FOCALLENGTH_SPEED 2
#define SCALING_SPEED 2

//#include "Display.h"


using namespace Ogre;
using namespace std;

class Loader: public Singleton<Loader>
{
public:
   Loader();
   ~Loader();


struct shadowListener: public Ogre::SceneManager::Listener
{
    // this is a callback we'll be using to set up our shadow camera
    void shadowTextureCasterPreViewProj(Ogre::Light *light, Ogre::Camera *cam, size_t)
    {
        // basically, here we do some forceful camera near/far clip attenuation
        // yeah.  simplistic, but it works nicely.  this is the function I was talking
        // about you ignoring above in the Mgr declaration.
        float range = light->getAttenuationRange();
        cam->setNearClipDistance(0.01);
        cam->setFarClipDistance(range);
        // we just use a small near clip so that the light doesn't "miss" anything
        // that can shadow stuff.  and the far clip is equal to the lights' range.
        // (thus, if the light only covers 15 units of objects, it can only
        // shadow 15 units - the rest of it should be attenuated away, and not rendered)
    }

    // these are pure virtual but we don't need them...  so just make them empty
    // otherwise we get "cannot declare of type Mgr due to missing abstract
    // functions" and so on
    void shadowTexturesUpdated(size_t) {}
    void shadowTextureReceiverPreViewProj(Ogre::Light*, Ogre::Frustum*) {}
    void preFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*) {}
    void postFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*) {}
} shadowCameraUpdater;

struct ssaoListener: public Ogre::CompositorInstance::Listener
{
    // this callback we will use to modify SSAO parameters
    void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
    {
        if (pass_id != 42) // not SSAO, return
            return;

        // this is the camera you're using
		Ogre::Camera *cam = global::getSingleton().getCamera();

        // calculate the far-top-right corner in view-space
        Ogre::Vector3 farCorner = cam->getViewMatrix(true) * cam->getWorldSpaceCorners()[4];

        // get the pass
        Ogre::Pass *pass = mat->getBestTechnique()->getPass(0);

        // get the vertex shader parameters
        Ogre::GpuProgramParametersSharedPtr params = pass->getVertexProgramParameters();
        // set the camera's far-top-right corner
        if (params->_findNamedConstantDefinition("farCorner"))
            params->setNamedConstant("farCorner", farCorner);

        // get the fragment shader parameters
        params = pass->getFragmentProgramParameters();
        // set the projection matrix we need
        static const Ogre::Matrix4 CLIP_SPACE_TO_IMAGE_SPACE(
            0.5,    0,    0,  0.5,
            0,   -0.5,    0,  0.5,
            0,      0,    1,    0,
            0,      0,    0,    1);
        if (params->_findNamedConstantDefinition("ptMat"))
            params->setNamedConstant("ptMat", CLIP_SPACE_TO_IMAGE_SPACE * cam->getProjectionMatrixWithRSDepth());
        if (params->_findNamedConstantDefinition("far"))
            params->setNamedConstant("far", cam->getFarClipDistance());
    }
} ssaoParamUpdater;

 void pssmAddLight(Light* light)
   {
	   LogManager::getSingleton().logMessage("Attempt to add to pssm.");
	   if (pssm){
		   LogManager::getSingleton().logMessage("Adding light to pssm...");
	   PSSMShadowListener* l1 = new PSSMShadowListener(mSceneMgr,light,mCurrentShadowCameraSetup,global::getSingleton().getCamera());
	   mSceneMgr->addListener(l1);
	   }
   }
   void init(SceneManager* scene,Viewport *LeftViewport,Viewport *RightViewport);
   void enableSSAO();
    void enaSSAO();
   void disableSSAO();
   String getPrefix();
   //void parse texture levels
   void parseGFX();
   bool depthshadowmap;
   bool deferred;
   bool deferred_test;
   bool material_disable;
   bool rtt_display;
private:
	bool pssm;
	String mapPrefix;
	ShadowCameraSetupPtr mCurrentShadowCameraSetup;
	LiSPSMShadowCameraSetup* mLiSPSMSetup;
	SceneManager* mSceneMgr;
	ConfigFile cf;
	StereoManager mStereoManager;
	Viewport *gLeftViewport;
	Viewport *gRightViewport;
	CWeapon* weapons;
	Ogre::CompositorInstance *ssao;
};