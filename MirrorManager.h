///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "global.h"

using namespace OgreNewt;
using namespace Ogre;

class MirrorManager: public Singleton<MirrorManager>, FrameListener, RenderTargetListener
{
public:
	MirrorManager();
	~MirrorManager();
	void init();
	void createNewMirror(String metalTex,Vector3 pos, Vector3 size,String name, Quaternion rot);
	void createNewMirror(String metalTex,SceneNode* node, Vector3 size);
	void destroyAllMirrors();
	bool frameRenderingQueued(const FrameEvent& evt);
	bool frameStarted(const FrameEvent& evt){return true;}

	void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        // Hide plane 
		vector<Entity*>::iterator k;
		for (k=mirrors.begin();k!=mirrors.end();k++)
        (*k)->setVisible(false);

    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        // Show plane 
        vector<Entity*>::iterator k;
		for (k=mirrors.begin();k!=mirrors.end();k++)
        (*k)->setVisible(true);
    }

private:
	vector<Camera*> reflectCams;
	vector<SceneNode*> mirrorNodes;
	vector<Body*> mirrorBodies;
	vector<Entity*> mirrors;
	vector<TexturePtr> textures;
	vector<MaterialPtr> materials;
	vector<Viewport*> renderViews;
	vector<MovablePlane*> planes;
	MovablePlane* mPlane;
};