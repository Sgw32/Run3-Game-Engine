#pragma once

#include "Ogre.h"
//#include "global.h"

using namespace Ogre;

class DecalProjector
{
public:
	DecalProjector();
	~DecalProjector();

	void createProjector(SceneNode* aNode)
    {
        // set up the main decal projection frustum
		mWeaponNode=aNode;
        mDecalFrustum = new Frustum();
        mProjectorNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("DecalProjectorNode");
        mProjectorNode->attachObject(mDecalFrustum);
        mProjectorNode->setPosition(0,5,5000);

        // include these two lines if you don't want perspective projection
        mDecalFrustum->setProjectionType(PT_ORTHOGRAPHIC);
        mDecalFrustum->setNearClipDistance(25);

        // set up the perpendicular filter texture frustum
        mFilterFrustum = new Frustum();
        mFilterFrustum->setProjectionType(PT_ORTHOGRAPHIC);
        filterNode = mProjectorNode->createChildSceneNode("DecalFilterNode");
        filterNode->attachObject(mFilterFrustum);
        filterNode->setOrientation(Quaternion(Degree(90),Vector3::UNIT_Y));
		
        // load the images for the decal and the filter
        //TextureManager::getSingleton().load("decal.png", "General", TEX_TYPE_2D, 1);
        //TextureManager::getSingleton().load("decal_filter.png", "General", TEX_TYPE_1D, 1);
    }
	void makeMaterialReceiveDecal(const String &matName)
    {
        // get the material
		mProjectorNode->setPosition(Ogre::Vector3(mWeaponNode->getParentSceneNode()->getParentSceneNode()->getPosition().x,mWeaponNode->getParentSceneNode()->getParentSceneNode()->getPosition().y,mWeaponNode->getParentSceneNode()->getParentSceneNode()->getPosition().z-20));
		mProjectorNode->setOrientation(mWeaponNode->getOrientation());
		filterNode->setPosition(mProjectorNode->getPosition()+Ogre::Vector3::NEGATIVE_UNIT_Z*100);
		filterNode->setOrientation(mWeaponNode->getOrientation());
        MaterialPtr mat = (MaterialPtr)MaterialManager::getSingleton().getByName(matName);

        // create a new pass in the material to render the decal
		Pass *pass;
		if (!mat->getTechnique(0)->getPass("run3_decal_"))
		{
			 pass = mat->getTechnique(0)->createPass();
			 pass->setName("run3_decal_");
		}
		else
		{
			pass = mat->getTechnique(0)->getPass("run3_decal_");
		}
        // set our pass to blend the decal over the model's regular texture
        pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
        pass->setDepthBias(1);

        // set the decal to be self illuminated instead of lit by scene lighting
        pass->setLightingEnabled(false);

        // set up the decal's texture unit
        TextureUnitState *texState = pass->createTextureUnitState("bullet_concrete_hit.png");
        texState->setProjectiveTexturing(true, mDecalFrustum);
        texState->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
        texState->setTextureFiltering(FO_POINT, FO_LINEAR, FO_NONE);

        // set up the filter texture's texture unit
        texState = pass->createTextureUnitState("bullet_concrete_hit.png");
        texState->setProjectiveTexturing(true, mFilterFrustum);
        texState->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
        texState->setTextureFiltering(TFO_NONE);
    }
private:
	Ogre::SceneManager* mSceneMgr;
	Ogre::SceneNode *mProjectorNode;
	Ogre::SceneNode *mWeaponNode;
	Ogre::SceneNode *filterNode;
    Ogre::Frustum *mDecalFrustum;
    Ogre::Frustum *mFilterFrustum;
};
