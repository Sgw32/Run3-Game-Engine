#include "MirrorManager.h"

template<> MirrorManager *Singleton<MirrorManager>::ms_Singleton=0;

MirrorManager::MirrorManager()
{
	
}

MirrorManager::~MirrorManager()
{
}

void MirrorManager::init()
{
	global::getSingleton().getRoot()->addFrameListener(this);
}

void MirrorManager::createNewMirror(String metalTex,Vector3 pos, Vector3 size, String name, Quaternion rot)
{
	World* mWorld = global::getSingleton().getWorld();
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	Entity* ent = mSceneMgr->createEntity(name,"box.mesh");
	SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,rot);
	node->attachObject(ent);
	node->setScale(size);

	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	delete col;
	
	mirrorBodies.push_back(bod);
	mirrors.push_back(ent);
	mirrorNodes.push_back(node);


}

void MirrorManager::createNewMirror(String metalTex,SceneNode* node, Vector3 size)
{
	World* mWorld = global::getSingleton().getWorld();
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	//Entity* ent = mSceneMgr->createEntity(node->getName(),"box.mesh");
	Camera* mCamera = global::getSingleton().getCamera();
mPlane = new MovablePlane(node->getName()+"ReflectPlane");
        mPlane->d = 0;
        mPlane->normal = Vector3::UNIT_Y;
		//MeshManager::getSingleton().getByName(node->getName()+"ReflectionPlane");
        MeshManager::getSingleton().createPlane(node->getName()+"ReflectionPlane", 
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
            *mPlane, 1, 1, 
            1, 1, true, 1, 1, 1, Vector3::UNIT_Z);

    Entity* ent = mSceneMgr->createEntity( node->getName(), node->getName()+"ReflectionPlane" );

	planes.push_back(mPlane);
	//mPlane->
	node->attachObject(ent);
	node->attachObject(mPlane);
	node->setScale(size);


	

	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld,size);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	bod->setName(node->getName());
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	RenderWindow* mWindow = global::getSingleton().getWindow();
	delete col;
	
	Camera* mReflectCam;
	TexturePtr texture = TextureManager::getSingleton().createManual( node->getName()+"RttTex", 
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 
			512, 512, 0, PF_R8G8B8, TU_RENDERTARGET );
		RenderTarget *rttTex = texture->getBuffer()->getRenderTarget();
		/*texture->unloSig
			TextureManager::getSingleton().remove()*/
            mReflectCam = mSceneMgr->createCamera("ReflectCam");
            mReflectCam->setNearClipDistance(mCamera->getNearClipDistance());
            mReflectCam->setFarClipDistance(mCamera->getFarClipDistance());
            mReflectCam->setAspectRatio(
                (Real)mWindow->getViewport(0)->getActualWidth() / 
                (Real)mWindow->getViewport(0)->getActualHeight());
			mReflectCam->setFOVy (mCamera->getFOVy());
			/*mReflectCam->setAspectRatio(
                (Real)mCamera->getViewport()->getActualWidth() / 
                (Real)mCamera->getViewport()->getActualHeight());
			mReflectCam->setFOVy (mCamera->getFOVy());*/
			

            Viewport *v = rttTex->addViewport( mReflectCam );
            v->setClearEveryFrame( true );
            v->setBackgroundColour( ColourValue::Black );

            MaterialPtr mat = MaterialManager::getSingleton().create(node->getName()+"RttMat",
                ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			 TextureUnitState* t;
			if (!metalTex.empty())
            t = mat->getTechnique(0)->getPass(0)->createTextureUnitState(metalTex);
            t = mat->getTechnique(0)->getPass(0)->createTextureUnitState(node->getName()+"RttTex");
            // Blend with base texture
            t->setColourOperationEx(LBX_BLEND_MANUAL, LBS_TEXTURE, LBS_CURRENT, ColourValue::White, 
                ColourValue::White, 0.9);
			t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			t->setProjectiveTexturing(true, mReflectCam);
            rttTex->addListener(this);

            // set up linked reflection
            mReflectCam->enableReflection(mPlane);
            // Also clip
            mReflectCam->enableCustomNearClipPlane(mPlane);
        

		reflectCams.push_back(mReflectCam);
        // Give the plane a texture
        ent->setMaterialName(node->getName()+"RttMat");

	mirrorBodies.push_back(bod);
	mirrors.push_back(ent);
	mirrorNodes.push_back(node);
	textures.push_back(texture);
	materials.push_back(mat);
	renderViews.push_back(v);
}

void MirrorManager::destroyAllMirrors()
{
	SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
	vector<Body*>::iterator i;
	vector<SceneNode*>::iterator j;
	vector<Entity*>::iterator k;
	vector<Camera*>::iterator m;
	vector<Viewport*>::iterator n;
	vector<MovablePlane*>::iterator pl;
	/*vector<MaterialPtr*>::iterator mat;
	vector<TexturePtr*>::iterator tex;*/

	for (i=mirrorBodies.begin();i!=mirrorBodies.end();i++)
	{
		delete (*i);
	}
	for (j=mirrorNodes.begin();j!=mirrorNodes.end();j++){
		if (mSceneMgr->hasSceneNode((*j)->getName()))
		{
		(*j)->detachAllObjects();
		mSceneMgr->destroySceneNode((*j));
		}
	}

	for (pl=planes.begin();pl!=planes.end();pl++)
		delete (*pl);

	for (k=mirrors.begin();k!=mirrors.end();k++)
	{
		if (mSceneMgr->hasEntity((*k)->getName()))
		mSceneMgr->destroyEntity((*k));
	}
	/*for (n=reflectCams.begin();n!=reflectCams.end();n++)
	{
		if (mSceneMgr->hasViewport((*m)->getName()))
		mSceneMgr->destroyCamera((*m));
	}*/
	for (unsigned int id=0;id!=materials.size();id++)
	{
		MaterialManager::getSingleton().remove((ResourcePtr)materials[id]);
		TextureManager::getSingleton().remove((ResourcePtr)textures[id]);
	}
	for (m=reflectCams.begin();m!=reflectCams.end();m++)
	{
		if (mSceneMgr->hasCamera((*m)->getName()))
		{
		mSceneMgr->destroyCamera((*m));
		}
	}
	planes.clear();
	textures.clear();
	materials.clear();
	renderViews.clear();
	mirrorBodies.clear();
	mirrors.clear();
	mirrorNodes.clear();
	reflectCams.clear();
}

bool MirrorManager::frameRenderingQueued(const FrameEvent& evt)
{
	vector<Camera*>::iterator i;
	for (i=reflectCams.begin();i!=reflectCams.end();i++)
	{
		Vector3 pos= global::getSingleton().getCamera()->getDerivedPosition();
		Quaternion rot = global::getSingleton().getCamera()->getDerivedOrientation();
		(*i)->setOrientation(rot);
        (*i)->setPosition(pos);
	}
	return true;
}