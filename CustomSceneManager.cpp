#include "CustomSceneManager.h"
 
//#include <OgreNoMemoryMacros.h>
#include "OgreCamera.h"
#include "OgreRenderSystem.h"
#include "OgreMeshManager.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreEntity.h"
#include "OgreSubEntity.h"
#include "OgreLight.h"
#include "OgreMath.h"
#include "OgreControllerManager.h"
#include "OgreMaterialManager.h"
#include "OgreAnimation.h"
#include "OgreAnimationTrack.h"
#include "OgreRenderQueueSortingGrouping.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "OgreStringConverter.h"
#include "OgreRenderQueueListener.h"
#include "OgreBillboardSet.h"
#include "OgrePass.h"
#include "OgreTechnique.h"
#include "OgreTextureUnitState.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreHardwareBufferManager.h"
#include "OgreRoot.h"
#include "OgreSpotShadowFadePng.h"
#include "OgreGpuProgramManager.h"
#include "OgreGpuProgram.h"
#include "OgreShadowVolumeExtrudeProgram.h"
#include "OgreDataStream.h"
#include "OgreStaticGeometry.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreManualObject.h"
#include "OgreRenderQueueInvocation.h"
#include "OgreBillboardChain.h"
#include "OgreRibbonTrail.h"
#include "OgreParticleSystemManager.h"
#include "OgreSceneQuery.h"
#include <OgreHardwareOcclusionQuery.h>
//#include <OgreMemoryMacros.h>
 
using namespace Ogre;

///
/// STATIC OCCLUSION QUERY LIST
///
std::list<RenderableOcclusionPair*> g_RenderableOcclusionPairList;


void CustomSceneManagerFactory::initMetaData(void) const
{
    mMetaData.typeName = FACTORY_TYPE_NAME;
    mMetaData.description = "The custom scene manager";
    mMetaData.sceneTypeMask = Ogre::ST_GENERIC;
    mMetaData.worldGeometrySupported = false;
}
 
const Ogre::String CustomSceneManagerFactory::FACTORY_TYPE_NAME = "CustomSceneManager";
 
Ogre::SceneManager* CustomSceneManagerFactory::createInstance(const Ogre::String& instanceName)
{
    return new CustomSceneManager(instanceName);
}
 
void CustomSceneManagerFactory::destroyInstance(Ogre::SceneManager* instance)
{
    delete instance;
}
 
LogDefineModule(CustomSceneManager);
 
CustomSceneManager::CustomSceneManager(const Ogre::String& name)
    : Ogre::SceneManager(name)
{
	g_InspectRenderable = 0;
}
 
CustomSceneManager::~CustomSceneManager()
{
}
 
const Ogre::String& CustomSceneManager::getTypeName(void) const
{
    return CustomSceneManagerFactory::FACTORY_TYPE_NAME;
}
 
void CustomSceneManager::renderSingleObject(Renderable* rend, const Pass* pass, 
                                      bool lightScissoringClipping, bool doLightIteration, 
									  const LightList* manualLightList)
{
     /// <!-- START OF CUSTOM CODE -->
   ///
   /// CREATE HardwareOcclusionQuery
   ///
   static Ogre::HardwareOcclusionQuery* hardwareOcclusionQuery;

#ifndef OCCLUSION_CULLING
   if (g_InspectRenderable == rend)
   {
      hardwareOcclusionQuery = Ogre::Root::getSingleton().getRenderSystem()->createHardwareOcclusionQuery();
      assert (hardwareOcclusionQuery);

      ///
      /// Start the HardwareOcclusionQuery
      ///
      hardwareOcclusionQuery->beginOcclusionQuery();
   }
#else
	  hardwareOcclusionQuery = Ogre::Root::getSingleton().getRenderSystem()->createHardwareOcclusionQuery();
      assert (hardwareOcclusionQuery);

      ///
      /// Start the HardwareOcclusionQuery
      ///
      hardwareOcclusionQuery->beginOcclusionQuery();
#endif

#ifdef OCCLUSION_CULLING2
	  bool render;
	   hardwareOcclusionQuery = Ogre::Root::getSingleton().getRenderSystem()->createHardwareOcclusionQuery();
      assert (hardwareOcclusionQuery);

      ///
      /// Start the HardwareOcclusionQuery
      ///
      hardwareOcclusionQuery->beginOcclusionQuery();
#endif
   /// <!-- END OF CUSTOM CODE -->

    unsigned short numMatrices;
    static RenderOperation ro;

    // Set up rendering operation
    // I know, I know, const_cast is nasty but otherwise it requires all internal
    // state of the Renderable assigned to the rop to be mutable
    const_cast<Renderable*>(rend)->getRenderOperation(ro);
    ro.srcRenderable = rend;

    // Set world transformation
    numMatrices = rend->getNumWorldTransforms();
	
	if (numMatrices > 0)
	{
	    rend->getWorldTransforms(mTempXform);

		if (mCameraRelativeRendering && !rend->getUseIdentityView())
		{
			for (unsigned short i = 0; i < numMatrices; ++i)
			{
				mTempXform[i].setTrans(mTempXform[i].getTrans() - mCameraRelativePosition);
			}
		}
		
		if (numMatrices > 1)
		{
			mDestRenderSystem->_setWorldMatrices(mTempXform, numMatrices);
		}
		else
		{
			mDestRenderSystem->_setWorldMatrix(*mTempXform);
		}
	}
    // Issue view / projection changes if any
    useRenderableViewProjMode(rend);

    if (!mSuppressRenderStateChanges)
    {
        bool passSurfaceAndLightParams = true;

        if (pass->isProgrammable())
        {
            // Tell auto params object about the renderable change
            mAutoParamDataSource->setCurrentRenderable(rend);
            // Tell auto params object about the world matrices, eliminated query from renderable again
            mAutoParamDataSource->setWorldMatrices(mTempXform, numMatrices);
            pass->_updateAutoParamsNoLights(mAutoParamDataSource);
            if (pass->hasVertexProgram())
            {
                passSurfaceAndLightParams = pass->getVertexProgram()->getPassSurfaceAndLightStates();
            }
        }

        // Reissue any texture gen settings which are dependent on view matrix
        Pass::ConstTextureUnitStateIterator texIter =  pass->getTextureUnitStateIterator();
        size_t unit = 0;
        while(texIter.hasMoreElements())
        {
            TextureUnitState* pTex = texIter.getNext();
            if (pTex->hasViewRelativeTextureCoordinateGeneration())
            {
                mDestRenderSystem->_setTextureUnitSettings(unit, *pTex);
            }
            ++unit;
        }

        // Sort out normalisation
		// Assume first world matrix representative - shaders that use multiple
		// matrices should control renormalisation themselves
		if ((pass->getNormaliseNormals() || mNormaliseNormalsOnScale)
			&& mTempXform[0].hasScale())
			mDestRenderSystem->setNormaliseNormals(true);
		else
			mDestRenderSystem->setNormaliseNormals(false);

		// Sort out negative scaling
		// Assume first world matrix representative 
		if (mFlipCullingOnNegativeScale)
		{
			CullingMode cullMode = mPassCullingMode;

			if (mTempXform[0].hasNegativeScale())
			{
				switch(mPassCullingMode)
				{
				case CULL_CLOCKWISE:
					cullMode = CULL_ANTICLOCKWISE;
					break;
				case CULL_ANTICLOCKWISE:
					cullMode = CULL_CLOCKWISE;
					break;
				};
			}

			// this also copes with returning from negative scale in previous render op
			// for same pass
			if (cullMode != mDestRenderSystem->_getCullingMode())
				mDestRenderSystem->_setCullingMode(cullMode);
		}

		// Set up the solid / wireframe override
		// Precedence is Camera, Object, Material
		// Camera might not override object if not overrideable
		PolygonMode reqMode = pass->getPolygonMode();
		if (pass->getPolygonModeOverrideable() && rend->getPolygonModeOverrideable())
		{
            PolygonMode camPolyMode = mCameraInProgress->getPolygonMode();
			// check camera detial only when render detail is overridable
			if (reqMode > camPolyMode)
			{
				// only downgrade detail; if cam says wireframe we don't go up to solid
				reqMode = camPolyMode;
			}
		}
		mDestRenderSystem->_setPolygonMode(reqMode);

		if (doLightIteration)
		{
            // Create local light list for faster light iteration setup
            static LightList localLightList;


			// Here's where we issue the rendering operation to the render system
			// Note that we may do this once per light, therefore it's in a loop
			// and the light parameters are updated once per traversal through the
			// loop
			const LightList& rendLightList = rend->getLights();

			bool iteratePerLight = pass->getIteratePerLight();

			// deliberately unsigned in case start light exceeds number of lights
			// in which case this pass would be skipped
			int lightsLeft = 1;
			if (iteratePerLight)
			{
				lightsLeft = static_cast<int>(rendLightList.size()) - pass->getStartLight();
				// Don't allow total light count for all iterations to exceed max per pass
				if (lightsLeft > static_cast<int>(pass->getMaxSimultaneousLights()))
				{
					lightsLeft = static_cast<int>(pass->getMaxSimultaneousLights());
				}
			}


			const LightList* pLightListToUse;
			// Start counting from the start light
			size_t lightIndex = pass->getStartLight();
			size_t depthInc = 0;

			while (lightsLeft > 0)
			{
				// Determine light list to use
				if (iteratePerLight)
				{
					// Starting shadow texture index.
					size_t shadowTexIndex = mShadowTextures.size();
					if (mShadowTextureIndexLightList.size() > lightIndex)
						shadowTexIndex = mShadowTextureIndexLightList[lightIndex];

					localLightList.resize(pass->getLightCountPerIteration());

					LightList::iterator destit = localLightList.begin();
					unsigned short numShadowTextureLights = 0;
					for (; destit != localLightList.end() 
							&& lightIndex < rendLightList.size(); 
						++lightIndex, --lightsLeft)
					{
						Light* currLight = rendLightList[lightIndex];

						// Check whether we need to filter this one out
						if (pass->getRunOnlyForOneLightType() && 
							pass->getOnlyLightType() != currLight->getType())
						{
							// Skip
							// Also skip shadow texture(s)
							if (isShadowTechniqueTextureBased())
							{
								shadowTexIndex += mShadowTextureCountPerType[currLight->getType()];
							}
							continue;
						}

						*destit++ = currLight;

						// potentially need to update content_type shadow texunit
						// corresponding to this light
						if (isShadowTechniqueTextureBased())
						{
							size_t textureCountPerLight = mShadowTextureCountPerType[currLight->getType()];
							for (size_t j = 0; j < textureCountPerLight && shadowTexIndex < mShadowTextures.size(); ++j)
							{
								// link the numShadowTextureLights'th shadow texture unit
								unsigned short tuindex = 
									pass->_getTextureUnitWithContentTypeIndex(
									TextureUnitState::CONTENT_SHADOW, numShadowTextureLights);
								if (tuindex > pass->getNumTextureUnitStates()) break;

								// I know, nasty const_cast
								TextureUnitState* tu = 
									const_cast<TextureUnitState*>(
										pass->getTextureUnitState(tuindex));
								const TexturePtr& shadowTex = mShadowTextures[shadowTexIndex];
								tu->_setTexturePtr(shadowTex);
								Camera *cam = shadowTex->getBuffer()->getRenderTarget()->getViewport(0)->getCamera();
								tu->setProjectiveTexturing(!pass->hasVertexProgram(), cam);
								mAutoParamDataSource->setTextureProjector(cam, numShadowTextureLights);
								++numShadowTextureLights;
								++shadowTexIndex;
								// Have to set TU on rendersystem right now, although
								// autoparams will be set later
								mDestRenderSystem->_setTextureUnitSettings(tuindex, *tu);
							}
						}



					}
					// Did we run out of lights before slots? e.g. 5 lights, 2 per iteration
					if (destit != localLightList.end())
					{
						localLightList.erase(destit, localLightList.end());
						lightsLeft = 0;
					}
					pLightListToUse = &localLightList;

					// deal with the case where we found no lights
					// since this is light iteration, we shouldn't render at all
					if (pLightListToUse->empty())
						return;

				}
				else // !iterate per light
				{
					// Use complete light list potentially adjusted by start light
					if (pass->getStartLight() || pass->getMaxSimultaneousLights() != OGRE_MAX_SIMULTANEOUS_LIGHTS)
					{
						// out of lights?
						// skip manual 2nd lighting passes onwards if we run out of lights, but never the first one
						if (pass->getStartLight() > 0 &&
							pass->getStartLight() >= rendLightList.size())
						{
							lightsLeft = 0;
							break;
						}
						else
						{
							localLightList.clear();
							LightList::const_iterator copyStart = rendLightList.begin();
							std::advance(copyStart, pass->getStartLight());
							LightList::const_iterator copyEnd = copyStart;
							// Clamp lights to copy to avoid overrunning the end of the list
							size_t lightsToCopy = std::min(
								static_cast<size_t>(pass->getMaxSimultaneousLights()), 
								rendLightList.size() - pass->getStartLight());
							std::advance(copyEnd, lightsToCopy);
							localLightList.insert(localLightList.begin(), 
								copyStart, copyEnd);
							pLightListToUse = &localLightList;
						}
					}
					else
					{
						pLightListToUse = &rendLightList;
					}
					lightsLeft = 0;
				}


				// Do we need to update GPU program parameters?
				if (pass->isProgrammable())
				{
					// Update any automatic gpu params for lights
					// Other bits of information will have to be looked up
					mAutoParamDataSource->setCurrentLightList(pLightListToUse);
					pass->_updateAutoParamsLightsOnly(mAutoParamDataSource);
					// NOTE: We MUST bind parameters AFTER updating the autos

					if (pass->hasVertexProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_VERTEX_PROGRAM, 
							pass->getVertexProgramParameters());
					}
					if (pass->hasGeometryProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_GEOMETRY_PROGRAM,
							pass->getGeometryProgramParameters());
					}
					if (pass->hasFragmentProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_FRAGMENT_PROGRAM, 
							pass->getFragmentProgramParameters());
					}
				}
				// Do we need to update light states? 
				// Only do this if fixed-function vertex lighting applies
				if (pass->getLightingEnabled() && passSurfaceAndLightParams)
				{
					mDestRenderSystem->_useLights(*pLightListToUse, pass->getMaxSimultaneousLights());
				}
				// optional light scissoring & clipping
				ClipResult scissored = CLIPPED_NONE;
				ClipResult clipped = CLIPPED_NONE;
				if (lightScissoringClipping && 
					(pass->getLightScissoringEnabled() || pass->getLightClipPlanesEnabled()))
				{
					// if there's no lights hitting the scene, then we might as 
					// well stop since clipping cannot include anything
					if (pLightListToUse->empty())
						continue;

					if (pass->getLightScissoringEnabled())
						scissored = buildAndSetScissor(*pLightListToUse, mCameraInProgress);
				
					if (pass->getLightClipPlanesEnabled())
						clipped = buildAndSetLightClip(*pLightListToUse);

					if (scissored == CLIPPED_ALL || clipped == CLIPPED_ALL)
						continue;
				}
				// issue the render op		
				// nfz: check for gpu_multipass
				mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
				// We might need to update the depth bias each iteration
				if (pass->getIterationDepthBias() != 0.0f)
				{
					float depthBiasBase = pass->getDepthBiasConstant() + 
						pass->getIterationDepthBias() * depthInc;
					// depthInc deals with light iteration 
					
					// Note that we have to set the depth bias here even if the depthInc
					// is zero (in which case you would think there is no change from
					// what was set in _setPass(). The reason is that if there are
					// multiple Renderables with this Pass, we won't go through _setPass
					// again at the start of the iteration for the next Renderable
					// because of Pass state grouping. So set it always

					// Set modified depth bias right away
					mDestRenderSystem->_setDepthBias(depthBiasBase, pass->getDepthBiasSlopeScale());

					// Set to increment internally too if rendersystem iterates
					mDestRenderSystem->setDeriveDepthBias(true, 
						depthBiasBase, pass->getIterationDepthBias(), 
						pass->getDepthBiasSlopeScale());
				}
				else
				{
					mDestRenderSystem->setDeriveDepthBias(false);
				}
				depthInc += pass->getPassIterationCount();

				if (rend->preRender((SceneManager*)this, mDestRenderSystem))
					mDestRenderSystem->_render(ro);
				rend->postRender(this, mDestRenderSystem);

				if (scissored == CLIPPED_SOME)
					resetScissor();
				if (clipped == CLIPPED_SOME)
					resetLightClip();
			} // possibly iterate per light
		}
		else // no automatic light processing
		{
			// Even if manually driving lights, check light type passes
			bool skipBecauseOfLightType = false;
			if (pass->getRunOnlyForOneLightType())
			{
				if (!manualLightList ||
					(manualLightList->size() == 1 && 
					manualLightList->at(0)->getType() != pass->getOnlyLightType())) 
				{
					skipBecauseOfLightType = true;
				}
			}

			if (!skipBecauseOfLightType)
			{
				// Do we need to update GPU program parameters?
				if (pass->isProgrammable())
				{
					// Do we have a manual light list?
					if (manualLightList)
					{
						// Update any automatic gpu params for lights
						mAutoParamDataSource->setCurrentLightList(manualLightList);
						pass->_updateAutoParamsLightsOnly(mAutoParamDataSource);
					}

					if (pass->hasVertexProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_VERTEX_PROGRAM, 
							pass->getVertexProgramParameters());
					}
					if (pass->hasGeometryProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_GEOMETRY_PROGRAM,
							pass->getGeometryProgramParameters());
					}
					if (pass->hasFragmentProgram())
					{
						mDestRenderSystem->bindGpuProgramParameters(GPT_FRAGMENT_PROGRAM, 
							pass->getFragmentProgramParameters());
					}
				}

				// Use manual lights if present, and not using vertex programs that don't use fixed pipeline
				if (manualLightList && 
					pass->getLightingEnabled() && passSurfaceAndLightParams)
				{
					mDestRenderSystem->_useLights(*manualLightList, pass->getMaxSimultaneousLights());
				}

				// optional light scissoring
				ClipResult scissored = CLIPPED_NONE;
				ClipResult clipped = CLIPPED_NONE;
				if (lightScissoringClipping && manualLightList && pass->getLightScissoringEnabled())
				{
					scissored = buildAndSetScissor(*manualLightList, mCameraInProgress);
				}
				if (lightScissoringClipping && manualLightList && pass->getLightClipPlanesEnabled())
				{
					clipped = buildAndSetLightClip(*manualLightList);
				}
	
				// don't bother rendering if clipped / scissored entirely
				if (scissored != CLIPPED_ALL && clipped != CLIPPED_ALL)
				{
					// issue the render op		
					// nfz: set up multipass rendering
					mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
					if (rend->preRender((SceneManager*)this, mDestRenderSystem))
						mDestRenderSystem->_render(ro);
					rend->postRender((SceneManager*)this, mDestRenderSystem);
				}
				if (scissored == CLIPPED_SOME)
					resetScissor();
				if (clipped == CLIPPED_SOME)
					resetLightClip();
				
			} // !skipBecauseOfLightType
		}

	}
	else // mSuppressRenderStateChanges
	{
		// Just render
		mDestRenderSystem->setCurrentPassIterationCount(1);
		if (rend->preRender((SceneManager*)this, mDestRenderSystem))
			mDestRenderSystem->_render(ro);
		rend->postRender((SceneManager*)this, mDestRenderSystem);
	}
	
    // Reset view / projection changes if any
    resetViewProjMode();
   
   /// <!-- START OF CUSTOM CODE -->
	#ifndef OCCLUSION_CULLING
   if (g_InspectRenderable == rend)
   {
      ///
      /// Stop the HardwareOcclusionQuery
      ///
      hardwareOcclusionQuery->endOcclusionQuery();

      ///
      /// GET THE PIXEL COUNT
      ///
      hardwareOcclusionQuery->pullOcclusionQuery(&g_PixelCount);

      ///
      /// DELETE HardwareOcclusionQuery
      ///
      Ogre::Root::getSingleton().getRenderSystem()->destroyHardwareOcclusionQuery(hardwareOcclusionQuery);
      hardwareOcclusionQuery = 0;
   }
	#else
		hardwareOcclusionQuery->endOcclusionQuery();

      ///
      /// GET THE PIXEL COUNT
      ///
      hardwareOcclusionQuery->pullOcclusionQuery(&occ[rend]);
		
      ///
      /// DELETE HardwareOcclusionQuery
      ///
      Ogre::Root::getSingleton().getRenderSystem()->destroyHardwareOcclusionQuery(hardwareOcclusionQuery);
      hardwareOcclusionQuery = 0;
	#endif
	
	#ifdef OCCLUSION_CULLING2
	  hardwareOcclusionQuery->endOcclusionQuery();

      ///
      /// GET THE PIXEL COUNT
      ///
      hardwareOcclusionQuery->pullOcclusionQuery(&occ[rend]);
		
      ///
      /// DELETE HardwareOcclusionQuery
      ///
      Ogre::Root::getSingleton().getRenderSystem()->destroyHardwareOcclusionQuery(hardwareOcclusionQuery);
      hardwareOcclusionQuery = 0;
	#endif
   /// <!-- END OF CUSTOM CODE -->
}
 
RenderableOcclusionPair::RenderableOcclusionPair(Ogre::Renderable* _renderable) :
    m_Renderable                    (_renderable),
    m_PixelCount                    (0),
    m_PixelState                    (RenderableOcclusionPair::READY_FOR_RENDER)
{
    ///
    /// CREATE HardwareOcclusionQuery
    ///
    m_HardwareOcclusionQuery = Ogre::Root::getSingleton().getRenderSystem()->createHardwareOcclusionQuery();
    assert (m_HardwareOcclusionQuery);
}
 
RenderableOcclusionPair::~RenderableOcclusionPair()
{
    ///
    /// DELETE HardwareOcclusionQuery
    ///
    Ogre::Root::getSingleton().getRenderSystem()->destroyHardwareOcclusionQuery(m_HardwareOcclusionQuery);
    m_HardwareOcclusionQuery = 0;
}
 
///
/// Returns the pixel count, and changes the state
/// to continue the next hardware query
///
unsigned int RenderableOcclusionPair::GetPixelCount()
{
    switch (m_PixelState)
    {
    case RenderableOcclusionPair::READY_FOR_ACCESS :
        {
            ///
            /// GET THE PIXEL COUNT
            ///
            m_HardwareOcclusionQuery->pullOcclusionQuery(&m_PixelCount);
 
            ///
            /// CHANGE THE STATE
            ///
            m_PixelState = RenderableOcclusionPair::READY_FOR_RENDER;
        }
    }
 
    return m_PixelCount;
}
 
///
/// Returns the renderable to query
///
Ogre::Renderable* RenderableOcclusionPair::GetRenderable()
{
    return m_Renderable;
}
 
///
/// Start the query, if the state
/// is ready to render.
///
void RenderableOcclusionPair::BeginOcclusionQuery()
{
    switch (m_PixelState)
    {
    case RenderableOcclusionPair::READY_FOR_RENDER :
        {
            ///
            /// START THE QUERY
            ///
            m_HardwareOcclusionQuery->beginOcclusionQuery();
 
            ///
            /// CHANGE THE STATE
            ///
            m_PixelState = RenderableOcclusionPair::QUERY_STARTED;
        }
    }
}
 
///
/// Finish the query, if the state
/// is ready to render. Changes
/// the state to wait for access
///
void RenderableOcclusionPair::EndOcclusionQuery()
{
    switch (m_PixelState)
    {
    case RenderableOcclusionPair::QUERY_STARTED :
        {
            ///
            /// START THE QUERY
            ///
            m_HardwareOcclusionQuery->endOcclusionQuery();
 
            ///
            /// CHANGE THE STATE
            ///
            m_PixelState = RenderableOcclusionPair::READY_FOR_ACCESS;
        }
    }
}