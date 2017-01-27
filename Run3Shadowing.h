#pragma once
#include "Ogre.h"
using namespace Ogre;


//shadow Listener
#ifndef RUN3SHADOWING_H
#define RUN3SHADOWING_H

#endif


class Run3Shadowing:public Singleton<Run3Shadowing>
{
public:
	Run3Shadowing();
	~Run3Shadowing();



//SSAO - listener

struct ssaoListener: public Ogre::CompositorInstance::Listener
{
    // this callback we will use to modify SSAO parameters
    void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
    {
        if (pass_id != 42) // not SSAO, return
            return;

        // this is the camera you're using
        Ogre::Camera *cam = cam;

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



	void init(SceneManager* mSceneMgr){sceneMgr=mSceneMgr;}
	
	void initSSAO()
	{
		ssao = Ogre::CompositorManager::getSingleton().addCompositor(vp, "ssao");
		ssao->setEnabled(true);
		ssao->addListener(&ssaoParamUpdater);
	}

	void stopSSAO()
	{
	}
private:
	 // a config file we'll use to retrieve some settings
    Ogre::ConfigFile cfgFile;

    // our root
    Ogre::Root *root;

    // simples stuff here
    Ogre::SceneManager *sceneMgr;
    Ogre::Camera *cam;
    Ogre::Viewport *vp;
    Ogre::RenderWindow *window;
    // note we only used one camera & viewport, simplistic
    Ogre::Timer *timer; // we'll just use the root's timer
    // otherwise you can make more complex timer systems

    // just incase
    Ogre::RenderSystem *rsys;

    // ssao instance
    Ogre::CompositorInstance *ssao;

    // local "continue?" variable
    bool running;

};