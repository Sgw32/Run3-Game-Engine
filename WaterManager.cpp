#include "WaterManager.h"

template<> WaterManager *Singleton<WaterManager>::ms_Singleton=0;

WaterManager::WaterManager()
{
		mHydrax=0;
}

WaterManager::~WaterManager()
{
		
}

void WaterManager::init()
{

}

void WaterManager::upd(const FrameEvent& evt)
{
	if (mHydrax)
	mHydrax->update(evt.timeSinceLastFrame);
}

void WaterManager::create(Vector3 sunPosition,Vector3 sunColor)
{
	mHydrax = new Hydrax::Hydrax(global::getSingleton().getSceneManager(), global::getSingleton().getCamera(), global::getSingleton().getWindow()->getViewport(0));
	// Create our projected grid module  
	Hydrax::Module::ProjectedGrid *mModule 
			= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
			                                    mHydrax,
												// Noise module
			                                    new Hydrax::Noise::Perlin(/*Generic one*/),
												// Base plane
			                                    Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
												// Normal mode
												Hydrax::MaterialManager::NM_VERTEX,
												// Projected grid options
										        Hydrax::Module::ProjectedGrid::Options(/*264 /*Generic one*/));

		// Set our module
		mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		mHydrax->loadCfg("HydraxDemo.hdx");
		mHydrax->setSunPosition(sunPosition);
        mHydrax->setSunColor(sunColor);
        // Create water
        mHydrax->create();
}

void WaterManager::create(Vector3 sunPosition,Vector3 sunColor,String fileName,Vector3 position)
{
	mHydrax = new Hydrax::Hydrax(global::getSingleton().getSceneManager(), global::getSingleton().getCamera(), global::getSingleton().getWindow()->getViewport(0));
	// Create our projected grid module  
	Hydrax::Module::ProjectedGrid *mModule 
			= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
			                                    mHydrax,
												// Noise module
			                                    new Hydrax::Noise::Perlin(/*Generic one*/),
												// Base plane
			                                    Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
												// Normal mode
												Hydrax::MaterialManager::NM_VERTEX,
												// Projected grid options
										        Hydrax::Module::ProjectedGrid::Options(/*264 /*Generic one*/));

		// Set our module
		mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		mHydrax->loadCfg(fileName);
		mHydrax->setSunPosition(sunPosition);
        mHydrax->setSunColor(sunColor);
		mHydrax->getMaterialManager()->addDepthTechnique(
			static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName("Island"))
			->createTechnique());
		mHydrax->setPosition(position);
        // Create water
        mHydrax->create();
}

void WaterManager::create()
{
	mHydrax = new Hydrax::Hydrax(global::getSingleton().getSceneManager(), global::getSingleton().getCamera(), global::getSingleton().getWindow()->getViewport(0));
	// Create our projected grid module  
	Hydrax::Module::ProjectedGrid *mModule 
			= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
			                                    mHydrax,
												// Noise module
			                                    new Hydrax::Noise::Perlin(/*Generic one*/),
												// Base plane
			                                    Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
												// Normal mode
												Hydrax::MaterialManager::NM_VERTEX,
												// Projected grid options
										        Hydrax::Module::ProjectedGrid::Options(/*264 /*Generic one*/));

		// Set our module
		mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		mHydrax->loadCfg("HydraxDemo.hdx");
		mHydrax->setSunPosition(Ogre::Vector3(0,10000,0));
        mHydrax->setSunColor(Ogre::Vector3(1, 0.9, 0.6));
        // Create water
        mHydrax->create();
}

void WaterManager::cleanup()
{
	delete mHydrax;
	mHydrax=0;
}