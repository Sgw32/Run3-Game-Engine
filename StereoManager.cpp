/*
-----------------------------------------------------------------------------
This source is part of the Stereoscopy manager for OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/
-----------------------------------------------------------------------------
* Copyright (c) 2008, Mathieu Le Ber, AXYZ-IMAGES
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the AXYZ-IMAGES nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Mathieu Le Ber ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Mathieu Le Ber BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

// Last update 11/25/2008

#include "Ogre.h"
#include "StereoManager.h"
#include <vector>
#include <limits>
using namespace Ogre;


//-------------- Stereo Camera listener -------------------------------
void StereoManager::StereoCameraListener::init(StereoManager *stereoMgr, Viewport *viewport, bool isLeftEye)
{
	mStereoMgr = stereoMgr;
	//mCamera = mStereoMgr->getCamera();
	mIsLeftEye = isLeftEye;
	mViewport = viewport;
}

//void StereoManager::StereoCameraListener::preRenderTargetUpdate(const RenderTargetEvent& evt)
void StereoManager::StereoCameraListener::preViewportUpdate (const RenderTargetViewportEvent& evt)
{
	if(evt.source != mViewport)
		return;
	mCamera = mViewport->getCamera();
	assert(mCamera);
	mStereoMgr->setCamera(mCamera);

	Real offset = mStereoMgr->getEyesSpacing()/2;
	if(mIsLeftEye)
	{
		offset = -offset;
	}

	mOldOffset = mCamera->getFrustumOffset();
	if(!mStereoMgr->isFocalLengthInfinite())
		mCamera->setFrustumOffset(mOldOffset - Vector2(offset,0));
	mOldPos = mCamera->getPosition();
	Vector3 pos = mOldPos;
	pos += offset * mCamera->getRight();
	mCamera->setPosition(pos);
	mStereoMgr->updateAllDependentRenderTargets(mIsLeftEye);
	mStereoMgr->chooseDebugPlaneMaterial(mIsLeftEye);
}
//void StereoManager::StereoCameraListener::postRenderTargetUpdate(const RenderTargetEvent& evt)
void StereoManager::StereoCameraListener::postViewportUpdate (const RenderTargetViewportEvent& evt)
{
	if(evt.source != mViewport)
		return;
	mCamera->setFrustumOffset(mOldOffset);
	mCamera->setPosition(mOldPos);
}

//-------------- Device Lost listener -------------------------------

void StereoManager::DeviceLostListener::init(StereoManager *stereoMgr)
{
	mStereoMgr = stereoMgr;
}

void StereoManager::DeviceLostListener::eventOccurred (const String &eventName, const NameValuePairList *parameters)
{
	if(eventName == "DeviceRestored")
	{
		if(mStereoMgr->mCompositorInstance)
		{
			Viewport *leftViewport, *rightViewport;
			mStereoMgr->shutdownListeners();
			leftViewport = mStereoMgr->mCompositorInstance->getRenderTarget("Stereo/Left")->getViewport(0);
			rightViewport = mStereoMgr->mCompositorInstance->getRenderTarget("Stereo/Right")->getViewport(0);
			mStereoMgr->initListeners(leftViewport, rightViewport);
		}
	}
}

//------------------------ init Stereo Manager --------------------------
StereoManager::StereoManager(void)
{
	mStereoMode = SM_NONE;
	mDebugPlane = NULL;
	mDebugPlaneNode = NULL;
	mLeftViewport = NULL;
	mRightViewport = NULL;
	mCamera = NULL;
	mCompositorInstance = NULL;
	mIsFocalPlaneFixed = false;
	mScreenWidth = 1;
	mEyesSpacing = 0.06;
	mFocalLength = 10;
	mFocalLengthInfinite = false;
	mIsInversed = false;
	mRightMask = ~((uint32)0);
	mLeftMask = ~((uint32)0);
//	mAvailableModes[SM_ANAGLYPH] = StereoModeDescription("ANAGLYPH", "Stereo/RedCyanAnaglyph");
	mAvailableModes[SM_ANAGLYPH_RC] = StereoModeDescription("ANAGLYPH_RED_CYAN", "Stereo/RedCyanAnaglyph");
	mAvailableModes[SM_ANAGLYPH_YB] = StereoModeDescription("ANAGLYPH_YELLOW_BLUE", "Stereo/YellowBlueAnaglyph");
	mAvailableModes[SM_INTERLACED_H] = StereoModeDescription("INTERLACED_HORIZONTAL", "Stereo/HorizontalInterlace");
	mAvailableModes[SM_INTERLACED_V] = StereoModeDescription("INTERLACED_VERTICAL", "Stereo/VerticalInterlace");
	mAvailableModes[SM_INTERLACED_CB] = StereoModeDescription("INTERLACED_CHECKBOARD", "Stereo/CheckboardInterlace");

	mAvailableModes[SM_DUALOUTPUT] = StereoModeDescription("DUALOUTPUT");
	mAvailableModes[SM_NONE] = StereoModeDescription("NONE");
}

StereoManager::~StereoManager(void)
{
	shutdown();
	destroyDebugPlane();
}

void StereoManager::init(Viewport* leftViewport, Viewport* rightViewport, StereoMode mode)
{
	if(mStereoMode != SM_NONE)
		return;
	mStereoMode = mode;
	init(leftViewport, rightViewport);
}

void StereoManager::init(Viewport* leftViewport, Viewport* rightViewport, const String &fileName)
{
	if(mStereoMode != SM_NONE)
		return;
	mStereoMode = loadConfig(fileName);
	init(leftViewport, rightViewport);
}

void StereoManager::init(Viewport* leftViewport, Viewport* rightViewport)
{
	if(mStereoMode == SM_NONE)
		return;
	if(!leftViewport)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "At least left viewport must be provided",
        "StereoManager::init");

	mCamera = leftViewport->getCamera();
	if(!mCamera && rightViewport && !rightViewport->getCamera())
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Viewports must have cameras associated",
        "StereoManager::init");

	if(rightViewport && mCamera != rightViewport->getCamera())
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Left and right viewports must have the same camera associated",
        "StereoManager::init");

	
	if(mAvailableModes[mStereoMode].mUsesCompositor)
	{
		Viewport *newLeft, *newRight;
		initCompositor(leftViewport, mAvailableModes[mStereoMode].mMaterialName, newLeft, newRight);
		leftViewport = newLeft;
		rightViewport = newRight;
	}

	initListeners(leftViewport, rightViewport);

	RenderTargetList::iterator it;
	RenderTargetList::iterator end = mRenderTargetList.end();
	for(it = mRenderTargetList.begin(); it != end; ++it)
	{
		it->first->setAutoUpdated(false);
	}

	bool infinite = mFocalLengthInfinite;
	setFocalLength(mFocalLength); // setFocalLength will erase the infinite focal length option, so backup it and restore it
	setFocalLengthInfinite(infinite);

	if(mIsFocalPlaneFixed)
		updateCamera(0);
}

void StereoManager::initListeners(Viewport* leftViewport, Viewport* rightViewport)
{
	if(leftViewport)
	{
		mLeftCameraListener.init(this, leftViewport, !mIsInversed);
		leftViewport->getTarget()->addListener(&mLeftCameraListener);
		mLeftViewport = leftViewport;
	}

	if(rightViewport)
	{
		mRightCameraListener.init(this, rightViewport, mIsInversed);
		rightViewport->getTarget()->addListener(&mRightCameraListener);
		mRightViewport = rightViewport;
	}
}

void StereoManager::shutdownListeners(void)
{
	if(mLeftViewport)
	{
		mLeftViewport->getTarget()->removeListener(&mLeftCameraListener);
		mLeftViewport = NULL;
	}
	if(mRightViewport)
	{
		mRightViewport->getTarget()->removeListener(&mRightCameraListener);
		mRightViewport = NULL;
	}
}

void StereoManager::initCompositor(Viewport *viewport, const String &materialName, Viewport *&out_left, Viewport *&out_right)
{
	mCompositorViewport = viewport;
	mCompositorInstance = CompositorManager::getSingleton().addCompositor(viewport, "Stereo/BaseCompositor");
	if(!mCompositorInstance)
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot create compositor, missing StereoManager resources",
        "StereoManager::initCompositor");
	CompositorManager::getSingleton().setCompositorEnabled(viewport, "Stereo/BaseCompositor", true);

	MaterialPtr mat = static_cast<MaterialPtr>(MaterialManager::getSingleton().getByName(materialName));
	if(mat.isNull())
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, materialName + " not found, missing StereoManager resources",
        "StereoManager::initCompositor");

	mCompositorInstance->getTechnique()->getOutputTargetPass()->getPass(0)->setMaterial(mat);
	out_left = mCompositorInstance->getRenderTarget("Stereo/Left")->getViewport(0);
	out_right = mCompositorInstance->getRenderTarget("Stereo/Right")->getViewport(0);

/*
	// extract all the compositors added to the main viewport and attach them to the left/right vewports
	CompositorManager &compositorManager = CompositorManager::getSingleton();
	if(compositorManager.hasCompositorChain(mCompositorViewport))
	{
		CompositorChain *oldChain = compositorManager.getCompositorChain(mCompositorViewport);

		for(unsigned int i = 0; i < oldChain->getNumCompositors(); i++)
		{
			CompositorInstance *compositorInstance = oldChain->getCompositor(i);
			Compositor *compositor = compositorInstance->getCompositor();
			compositorManager.addCompositor(leftViewport, compositor->getName());
			compositorManager.setCompositorEnabled(leftViewport, compositor->getName(),compositorInstance->getEnabled());
			compositorManager.addCompositor(rightViewport, compositor->getName());
			compositorManager.setCompositorEnabled(rightViewport, compositor->getName(),compositorInstance->getEnabled());

		}
		// remove the compositors from the main viewport since they are now on the left/right vewports
		oldChain->removeAllCompositors();
	}
*/
/*
	// enable the overlays on the new viewports and disable them from the main viewport
	mAreOverlaysEnabled = viewport->getOverlaysEnabled();
	out_left->setOverlaysEnabled(mAreOverlaysEnabled);
	out_right->setOverlaysEnabled(mAreOverlaysEnabled);
	viewport->setOverlaysEnabled(false);
*/
	mDeviceLostListener.init(this);
	Root::getSingleton().getRenderSystem()->addListener(&mDeviceLostListener);
}

void StereoManager::shutdownCompositor()
{
	CompositorManager::getSingleton().setCompositorEnabled(mCompositorViewport, "Stereo/BaseCompositor", false);
	CompositorManager::getSingleton().removeCompositor(mCompositorViewport, "Stereo/BaseCompositor");

/*
		// put back the compositors on the main viewport
		CompositorManager &compositorManager = CompositorManager::getSingleton();
		if(compositorManager.hasCompositorChain(mLeftViewport))
		{
			CompositorChain *oldChain = compositorManager.getCompositorChain(mLeftViewport);
			for(unsigned int i = 0; i < oldChain->getNumCompositors(); i++)
			{
				CompositorInstance *compositorInstance = oldChain->getCompositor(i);
				Compositor *compositor = compositorInstance->getCompositor();
				compositorManager.addCompositor(mCompositorViewport, compositor->getName());
				compositorManager.setCompositorEnabled(mCompositorViewport, compositor->getName(),compositorInstance->getEnabled());
			}
			oldChain->removeAllCompositors();
		}

		//mRightTarget->removeAllViewports();
		//mLeftTarget->removeAllViewports();
		mCompositorViewport->setOverlaysEnabled(mAreOverlaysEnabled);
*/
/*
	mCompositorViewport->setOverlaysEnabled(mAreOverlaysEnabled);
*/
	Root::getSingleton().getRenderSystem()->removeListener(&mDeviceLostListener);
	mCompositorInstance = NULL;
	mCompositorViewport = NULL;
}

void StereoManager::shutdown(void)
{
	if(mStereoMode == SM_NONE)
		return;

	shutdownListeners();
	if(mAvailableModes[mStereoMode].mUsesCompositor)
		shutdownCompositor();

	RenderTargetList::iterator it;
	RenderTargetList::iterator end = mRenderTargetList.end();
	for(it = mRenderTargetList.begin(); it != end; ++it)
	{
		it->first->setAutoUpdated(it->second);
	}

	mStereoMode = SM_NONE;
}
//-------------------------- misc --------------

void StereoManager::setVisibilityMask(uint32 leftMask, uint32 rightMask)
{
	if(mLeftViewport)
	{
		mLeftViewport->setVisibilityMask(leftMask);
	}
	if(mRightViewport)
	{
		mRightViewport->setVisibilityMask(rightMask);
	}
	mRightMask = rightMask;
	mLeftMask = leftMask;
}

void StereoManager::addRenderTargetDependency(RenderTarget *renderTarget)
{
	if(mRenderTargetList.find(renderTarget) != mRenderTargetList.end())
		return;
	mRenderTargetList[renderTarget] = renderTarget->isAutoUpdated();
	renderTarget->setAutoUpdated(false);
}

void StereoManager::removeRenderTargetDependency(RenderTarget *renderTarget)
{
	if(mRenderTargetList.find(renderTarget) == mRenderTargetList.end())
		return;
	renderTarget->setAutoUpdated(mRenderTargetList[renderTarget]);
	mRenderTargetList.erase(renderTarget);
}

void StereoManager::updateAllDependentRenderTargets(bool isLeftEye)
{
	uint32 mask;
	if(isLeftEye)
	{
		mask = mLeftMask;
	}
	else
	{
		mask = mRightMask;
	}

	RenderTargetList::iterator itarg, itargend;
	itargend = mRenderTargetList.end();
	for( itarg = mRenderTargetList.begin(); itarg != itargend; ++itarg )
	{
		RenderTarget *rt = itarg->first;

		int n = rt->getNumViewports();
		std::vector<int> maskVector(n); // VS2005 gives a warning if I declare the vector as uint32 but not with int

		for(int i = 0; i<n ; ++i)
		{
			maskVector[i] = rt->getViewport(i)->getVisibilityMask();
			rt->getViewport(i)->setVisibilityMask(maskVector[i] & mask);
		}

		rt->update();

		for(int i = 0; i<n ; ++i)
		{
			rt->getViewport(i)->setVisibilityMask(maskVector[i]);
		}
	}
}

//---------------------------- Stereo tuning  ------------------------
void StereoManager::setFocalLength(Real l)
{
	if(l == std::numeric_limits<Real>::infinity())
	{
		setFocalLengthInfinite(true);
	}
	else
	{
		setFocalLengthInfinite(false);

		Real old = mFocalLength;
		mFocalLength = l;
		if( mCamera )
		{
			mCamera->setFocalLength(mFocalLength);
			if(mIsFocalPlaneFixed)
				updateCamera(mFocalLength - old);
			else if(mDebugPlane)
				updateDebugPlane();
		}
	}
}

Real StereoManager::getFocalLength(void) const
{
	if(mFocalLengthInfinite)
		return std::numeric_limits<Real>::infinity();
	else
		return mFocalLength;
}

void StereoManager::setFocalLengthInfinite(bool isInfinite)
{
	mFocalLengthInfinite = isInfinite;
	if(isInfinite)
		mIsFocalPlaneFixed = false;
}

void StereoManager::useScreenWidth(Real w)
{
	mScreenWidth = w;
	mIsFocalPlaneFixed = true;
	if( mCamera )
		updateCamera(0);
}

void StereoManager::updateCamera(Real delta)
{
	mCamera->moveRelative(-delta * Vector3::UNIT_Z);
	Radian a = 2 * Math::ATan(mScreenWidth/(2 * mFocalLength * mCamera->getAspectRatio()));
	mCamera->setFOVy(a);
}

void StereoManager::inverseStereo(bool inverse)
{
	mIsInversed = inverse;
	mLeftCameraListener.mIsLeftEye = !mIsInversed;
	mRightCameraListener.mIsLeftEye = mIsInversed;
}

//------------------------------------ Debug focal plane ---------------------------------
void StereoManager::enableDebugPlane(bool enable)
{
	if(mDebugPlane)
		mDebugPlane->setVisible(enable);
}

void StereoManager::toggleDebugPlane(void)
{
	if(mDebugPlane)
		mDebugPlane->setVisible(!mDebugPlane->isVisible());
}

void StereoManager::createDebugPlane(SceneManager *sceneMgr, const String &leftMaterialName, const String &rightMaterialName)
{
	if(mDebugPlane)
		return;

	mSceneMgr = sceneMgr;
    Plane screenPlane;
	screenPlane.normal = Vector3::UNIT_Z;
	MeshManager::getSingleton().createPlane("Stereo/Plane",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		screenPlane,1,1,10,10);
	mDebugPlane = sceneMgr->createEntity( "Stereo/DebugPlane", "Stereo/Plane" );

	if(leftMaterialName == "")
	{
		mLeftMaterialName = "Stereo/Wireframe";
	}
	else
	{
		mLeftMaterialName = leftMaterialName;
	}

	if(rightMaterialName == "")
	{
		mRightMaterialName = "Stereo/Wireframe";
	}
	else
	{
		mRightMaterialName = rightMaterialName;
	}


	mDebugPlaneNode = static_cast<SceneNode*>(sceneMgr->getRootSceneNode()->createChild("Stereo/DebugPlaneNode"));
	mDebugPlaneNode->attachObject(mDebugPlane);

	enableDebugPlane(true);
	updateDebugPlane();
}

void StereoManager::destroyDebugPlane(void)
{
	if(mDebugPlane)
	{
		SceneNode *parent = static_cast<SceneNode*>(mDebugPlaneNode->getParent());
		parent->removeAndDestroyChild("Stereo/DebugPlaneNode");
		mDebugPlaneNode = NULL;
		mSceneMgr->destroyEntity("Stereo/DebugPlane");
		mDebugPlane = NULL;
		MeshManager::getSingleton().remove("Stereo/Plane");
	}
}

void StereoManager::updateDebugPlane(void)
{
	if(mDebugPlaneNode && mCamera)
	{
		Real actualFocalLength = mFocalLengthInfinite ? mCamera->getFarClipDistance() * 0.99f : mFocalLength;

		Vector3 pos = mCamera->getDerivedPosition();
		pos += mCamera->getDerivedDirection() * actualFocalLength;
		mDebugPlaneNode->setPosition(pos);
		mDebugPlaneNode->setOrientation(mCamera->getDerivedOrientation());
		Vector3 scale;
		Real height = actualFocalLength * Math::Tan(mCamera->getFOVy()/2)*2;
		scale.z = 1;
		scale.y = height;
		scale.x = height * mCamera->getAspectRatio();
		mDebugPlaneNode->setScale(scale);
	}
}

void StereoManager::chooseDebugPlaneMaterial(bool isLeftEye)
{
	if(mDebugPlane)
	{
		if(isLeftEye)
			mDebugPlane->setMaterialName(mLeftMaterialName);
		else
			mDebugPlane->setMaterialName(mRightMaterialName);
	}
}


//-------------------------------------- config ------------------------------------
void StereoManager::saveConfig(const String &filename)
{
	std::ofstream of(filename.c_str());
    if (!of)
        OGRE_EXCEPT(Exception::ERR_CANNOT_WRITE_TO_FILE, "Cannot create settings file.",
        "StereoManager::saveConfig");

	of << "[Stereoscopy]" << std::endl;
	of << "# Available Modes: ";
	
	const StereoModeList::const_iterator end = mAvailableModes.end();
	for(StereoModeList::iterator it = mAvailableModes.begin(); it != end; ++it)
	{
		of << it->second.mName << " ";
	}
	of << std::endl;

	of << "Stereo mode = "  << mAvailableModes[mStereoMode].mName << std::endl;
	of << "Eyes spacing = " << mEyesSpacing << std::endl;
	
	of << "# Set to inf for parallel frustrum stereo." << std::endl;
	if(mFocalLengthInfinite)
		of << "Focal length = " << "inf"  << std::endl;
	else
		of << "Focal length = " << mFocalLength << std::endl;

	of << "Inverse stereo = " << (mIsInversed ? "true" : "false") << std::endl;

	of << std::endl << "# For advanced use. See StereoManager.h for details." << std::endl;
	of << "Fixed screen = " << (mIsFocalPlaneFixed ? "true" : "false") << std::endl;
	of << "Screen width = " << mScreenWidth << std::endl;

    of.close();
}

StereoManager::StereoMode StereoManager::loadConfig(const String &filename)
{
	ConfigFile cf;
	cf.load(filename.c_str());

	StereoMode mode;

	const String &modeName = cf.getSetting("Stereo mode","Stereoscopy");
	const StereoModeList::const_iterator end = mAvailableModes.end();
	StereoModeList::iterator it;
	for(it = mAvailableModes.begin(); it != end; ++it)
	{
		if(it->second.mName == modeName)
		{
			mode = it->first;
			break;
		}
	}
	if(it == mAvailableModes.end())
		mode = SM_NONE;

	fixFocalPlanePos(StringConverter::parseBool(cf.getSetting("Fixed screen","Stereoscopy")));

	if(cf.getSetting("Focal length","Stereoscopy") == "inf")
		mFocalLengthInfinite = true;
	else
		setFocalLength(StringConverter::parseReal(cf.getSetting("Focal length","Stereoscopy")));

	setEyesSpacing(StringConverter::parseReal(cf.getSetting("Eyes spacing","Stereoscopy")));
	setScreenWidth(StringConverter::parseReal(cf.getSetting("Screen width","Stereoscopy")));
	inverseStereo(StringConverter::parseBool(cf.getSetting("Inverse stereo","Stereoscopy")));
	
	return mode;
}
