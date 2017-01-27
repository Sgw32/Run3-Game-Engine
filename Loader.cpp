#include "Loader.h"
#include "SoftwareOcclusionCulling.h"

template<> Loader *Singleton<Loader>::ms_Singleton=0;

Loader::Loader(){
depthshadowmap=false;
deferred=false;
material_disable=false;
pssm=false;
}

Loader::~Loader()
{

}

void Loader::init(SceneManager* scene,Viewport *LeftViewport,Viewport *RightViewport)
{
mSceneMgr=scene;
gLeftViewport=LeftViewport;
gRightViewport=RightViewport;
}

String Loader::getPrefix()
{
	return mapPrefix;
}


void Loader::parseGFX()
{
	LogManager::getSingleton().logMessage("Loader: started parseGFX()");
	String result,bloomen;
	Real texLOD;
	Real meshLOD;
	//Real shadowDist;
//	int res;
	bool stereo;
	bool PCF;
	cf.load("run3/core/perfomance.cfg");
	result = cf.getSetting("ShadowTech");
	material_disable=StringConverter::parseBool(cf.getSetting("MaterialDisable","","false"));
	SoftwareOcclusionCulling::getSingleton().setEnabled(StringConverter::parseBool(cf.getSetting("OCEnabled","","false")));
	mapPrefix=cf.getSetting("tech");
	//mSceneMgr->setsh
	if (result == "texmodul")
		mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
	if (result == "texadd")
		mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);
	if (result == "stlmodul")
	{
		LogManager::getSingleton().logMessage("Run3: using Stencil shadows");
		mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);
	}
	if (result == "stladd")
		mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);
GlowMaterialListener *gml=0;
	if (StringConverter::parseBool(cf.getSetting("enableGlow","","true")))
{
	LogManager::getSingleton().logMessage("Glow is now enabled!");
	Camera* mCamera = global::getSingleton().getCamera();
		CompositorManager::getSingleton().removeCompositor(mCamera->getViewport(), "Glow");
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(), "Glow");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", true);
		gml = new GlowMaterialListener();
		Ogre::MaterialManager::getSingleton().addListener(gml);
	}
if (StringConverter::parseBool(cf.getSetting("enableGlass","","true")))
{
	LogManager::getSingleton().logMessage("Glass is now enabled!");
	Camera* mCamera = global::getSingleton().getCamera();
		CompositorManager::getSingleton().addCompositor(mCamera->getViewport(), "Run3Glass");
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Run3Glass", true);
		if (gml!=0)
		{
		gml = new GlowMaterialListener();
		Ogre::MaterialManager::getSingleton().addListener(gml);
		}
	}
if (StringConverter::parseBool(cf.getSetting("enableSSAO","","false")))
{
	enableSSAO();
}

if (StringConverter::parseBool(cf.getSetting("COMEnable","","false")))
	{
		int baud =StringConverter::parseInt(cf.getSetting("COMBaud","","2400"));
		int port =StringConverter::parseInt(cf.getSetting("COMPort","","8"));
		global::getSingleton().port=new CSerial();
		LogManager::getSingleton().logMessage("Serial: init COM"+cf.getSetting("COMPort","8"));
		if (!global::getSingleton().port->Open(port,baud))
			LogManager::getSingleton().logMessage("error");
	}

rtt_display=StringConverter::parseBool(cf.getSetting("rttDisplay","","true"));
	if (result == "depthshadowmap")
	{
		PCF=StringConverter::parseBool(cf.getSetting("pcfEnable",StringUtil::BLANK,"false"));
		// Allow self shadowing (note: this only works in conjunction with the shaders defined above)
 //mSceneMgr->setShadowTextureSelfShadow(true);
 // Set the caster material which uses the shaders defined above
 //mSceneMgr->setShadowTextureCasterMaterial("Ogre/DepthShadowmap/Caster/Float");
 // Set the caster material which uses the shaders defined above
 //mSceneMgr->setShadowTextureRecieverMaterial("Ogre/DepthShadowmap/Caster/Float");
 // Set the pixel format to floating point
 //mSceneMgr->setShadowTexturePixelFormat(PF_FLOAT32_R);
 // You can switch this on or off, I suggest you try both and see which works best for you
 //mSceneMgr->setShadowCasterRenderBackFaces(false);
 // Finally enable the shadows using texture additive integrated
 //mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
 //depthshadowmap=true;

 mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
		//mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);
		mSceneMgr->setShadowTextureSettings(1536,3);
		mSceneMgr->setShadowColour(ColourValue(0.1, 0.1, 0.1));


		mSceneMgr->setShadowTextureSelfShadow(true);
		mSceneMgr->setShadowTexturePixelFormat(PF_FLOAT32_R);
		mSceneMgr->setShadowTextureCasterMaterial("Ogre/DepthShadowmap/Caster/Float");
	}

	if (result=="perfomancetest01")
		depthshadowmap=true;
	//SSAO Demo
	if (result=="run3shadow")
	{
		 // we'll be self shadowing
    mSceneMgr->setShadowTextureSelfShadow(true);

    // our caster material
    mSceneMgr->setShadowTextureCasterMaterial("shadow_caster");
    // note we have no "receiver".  all the "receivers" are integrated.

    // get the shadow texture count from the cfg file
    std::string tempData = cf.getSetting("shadowTextureCount", "","4");
    // (each light needs a shadow texture)
    mSceneMgr->setShadowTextureCount(Ogre::StringConverter::parseInt(tempData));

    // the size, too (1024 looks good with 3x3 or more filtering)
    tempData = cf.getSetting("shadowTextureRes", "","256");
    mSceneMgr->setShadowTextureSize(Ogre::StringConverter::parseInt(tempData));

    // float 16 here.  we need the R and G channels.
    // float 32 works a lot better with a low/none VSM epsilon (wait till the shaders)
    // but float 16 is good enough and supports bilinear filtering on a lot of cards
    // (we should use _GR, but OpenGL doesn't really like it for some reason)
    mSceneMgr->setShadowTexturePixelFormat(Ogre::PF_FLOAT16_RGB);

    // big NONO to render back faces for VSM.  it doesn't need any biasing
    // so it's worthless (and rather problematic) to use the back face hack that
    // works so well for normal depth shadow mapping (you know, so you don't
    // get surface acne)
   mSceneMgr->setShadowCasterRenderBackFaces(false);

    const unsigned numShadowRTTs = mSceneMgr->getShadowTextureCount();
    for (unsigned i = 0; i < numShadowRTTs; ++i)
    {
        Ogre::TexturePtr tex =mSceneMgr->getShadowTexture(i);
        Ogre::Viewport *vp = tex->getBuffer()->getRenderTarget()->getViewport(0);
        vp->setBackgroundColour(Ogre::ColourValue(1, 1, 1, 1));
        vp->setClearEveryFrame(true);
    }

    // enable integrated additive shadows
    // actually, since we render the shadow map ourselves, it doesn't
    // really matter whether they are additive or modulative
    // as long as they are integrated v(O_o)v
    mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);

    // and add the shader listener
    mSceneMgr->addListener(&shadowCameraUpdater);
	//depthshadowmap=true;
	}
		
	if (result=="deferred")
	{
		//iSystem
		DeferredShadingSystem* mSystem = new DeferredShadingSystem(global::getSingleton().getWindow()->getViewport(0), global::getSingleton().getSceneManager(), global::getSingleton().getCamera());
		global::getSingleton().iSystem=mSystem;
		deferred=true;
		
	}
	
	

	bloomen=cf.getSetting("BloomEnable");
	String proj = cf.getSetting("projection");
	DefaultShadowCameraSetup* dscs = new DefaultShadowCameraSetup();
	//
	mCurrentShadowCameraSetup = 
						ShadowCameraSetupPtr(dscs);
	if (proj=="UNIFORM_FOCUSED")
	{
	mCurrentShadowCameraSetup = 
						ShadowCameraSetupPtr(new FocusedShadowCameraSetup());
	}
	if (proj=="LISPSM")
	{
	mLiSPSMSetup = new LiSPSMShadowCameraSetup();
						//mLiSPSMSetup->setUseAggressiveFocusRegion(false);
						mCurrentShadowCameraSetup = ShadowCameraSetupPtr(mLiSPSMSetup);
	}
	//mCurrentShadowCameraSetup->getShadowCamera();
mSceneMgr->setShadowCameraSetup(mCurrentShadowCameraSetup);

if (result=="r3pssm")
	{
		//mSceneMgr->addListener(new PSSMShadowListener(mSceneMgr,light,mCurrentShadowCameraSetup,mCamera));
		mSceneMgr->setShadowTextureCount(3);
	 
		mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
		mSceneMgr->setShadowTextureSelfShadow(true);
		mSceneMgr->setShadowTextureFadeStart(1.f);
		mSceneMgr->setShadowTextureFadeEnd(1.f);

		   // mSceneMgr->setShadowTextureSelfShadow(true);

    // our caster material
		mSceneMgr->setShadowTextureCasterMaterial("PSSM/shadow_caster");
		mSceneMgr->setShadowCameraSetup(mCurrentShadowCameraSetup);
		mSceneMgr->setShadowCasterRenderBackFaces(true);
		mSceneMgr->setShadowTextureSize(512);
		pssm=true;
	}
	mSceneMgr->setShadowFarDistance(StringConverter::parseReal(cf.getSetting("shadowDist",StringUtil::BLANK,"100000")));
	if (result=="r3pssm2")
	{
		 mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
   mSceneMgr->setShadowFarDistance(50000);
   mSceneMgr->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);
   mSceneMgr->setShadowTextureCount(3);
   mSceneMgr->setShadowTextureConfig(0,2048,2048,PF_FLOAT16_RGB);
   mSceneMgr->setShadowTextureConfig(1,2048,2048,PF_FLOAT16_RGB);
   mSceneMgr->setShadowTextureConfig(2,2048,2048,PF_FLOAT16_RGB);
   mSceneMgr->setShadowTextureSelfShadow(true);
   mSceneMgr->setShadowTextureCasterMaterial("PSSM/shadow_caster");
	mSceneMgr->setShadowCasterRenderBackFaces(true);
	mSceneMgr->setShadowDirLightTextureOffset(500);
   PSSMShadowCameraSetup* pssmSetup = new PSSMShadowCameraSetup();
   pssmSetup->calculateSplitPoints(3,
      global::getSingleton().getCamera()->getNearClipDistance(),mSceneMgr->getShadowFarDistance());
   pssmSetup->setSplitPadding(global::getSingleton().getCamera()->getNearClipDistance());
   pssmSetup->setOptimalAdjustFactor(0, -1);
   pssmSetup->setOptimalAdjustFactor(1, -1);
   pssmSetup->setOptimalAdjustFactor(2, -1);
   mSceneMgr->setShadowCameraSetup(ShadowCameraSetupPtr(pssmSetup));


   // get split points
   Ogre::Vector4 splitPoints;
   const Ogre::PSSMShadowCameraSetup::SplitPointList& splitPointList = pssmSetup->getSplitPoints();
	LogManager::getSingleton().logMessage("Loader: passing PSSM split points");
   for (int i = 0; i < 3; ++i)
   {
      splitPoints[i] = splitPointList[i];
	  LogManager::getSingleton().logMessage(StringConverter::toString(splitPoints[i])+" number "+StringConverter::toString(i));
   }
	}

	if (bloomen=="true")
		SuperFX::getSingleton().setBloomEnabled(true);
	if (bloomen=="false")
		SuperFX::getSingleton().setBloomEnabled(false);
	bool hdron = StringConverter::parseBool(cf.getSetting("HDREnable",StringUtil::BLANK,"true"));
	if (hdron)
	{
		/*CompositorManager::getSingleton().addCompositor(mainView, "NewHDR");
		CompositorManager::getSingleton().setCompositorEnabled(mainView, "NewHDR", true);*/
		SuperFX::getSingleton().setHDREnabled(true);
	}
	SuperFX::getSingleton().allowMotionBlur(StringConverter::parseBool(cf.getSetting("allowMotionBlur",StringUtil::BLANK,"true")));
	
	texLOD=StringConverter::parseInt(cf.getSetting("textureLOD",StringUtil::BLANK,"-1"));
	meshLOD=StringConverter::parseInt(cf.getSetting("meshLOD",StringUtil::BLANK,"-1"));
	global::getSingleton().setMeshLod(meshLOD);
	global::getSingleton().setTexLod(texLOD);
	stereo=StringConverter::parseBool(cf.getSetting("enableStereo",StringUtil::BLANK,"false"));
	if (stereo)
	{
	try
		{
			// try to load the config file
			mStereoManager.init(gLeftViewport, gRightViewport, "run3/core/stereo.cfg");
		}
		catch(Ogre::Exception e)
		{
			// if the config file is not found, it throws an exception
			if(e.getNumber() == Ogre::Exception::ERR_FILE_NOT_FOUND)
			{
				// init the manager with defaults parameters
				mStereoManager.init(gLeftViewport, gRightViewport, StereoManager::SM_DUALOUTPUT);
				mStereoManager.setEyesSpacing(EYES_SPACING);
				mStereoManager.setFocalLength(SCREEN_DIST);
				mStereoManager.saveConfig("run3/core/stereo.cfg");
				//mStereoManager.useScreenWidth(SCREEN_WIDTH);
			}
			else
				throw e;
		}
	}
	LogManager::getSingleton().logMessage("Loader: ended parseGFX()");
}

void Loader::enableSSAO()
{
	ssao = Ogre::CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(), "ssao");
		ssao->setEnabled(true);
		ssao->addListener(&ssaoParamUpdater);
}

void Loader::enaSSAO()
{
	ssao->setEnabled(true);
}

void Loader::disableSSAO()
{
		ssao->setEnabled(false);
}

