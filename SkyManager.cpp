#include "SkyManager.h"

template<> SkyManager *Singleton<SkyManager>::ms_Singleton=0;

SkyManager::SkyManager()
{
		mSkyX=0;
}

SkyManager::~SkyManager()
{
		
}

void SkyManager::init()
{

}

void SkyManager::upd(const FrameEvent& evt)
{
	if (mSkyX)
	mSkyX->update(evt.timeSinceLastFrame);
}

void SkyManager::setTimeMultiplier(Real timestep)
{
	mSkyX->setTimeMultiplier(timestep);
}

void SkyManager::setHeightPosition(Real height)
{
	mSkyXOptions.HeightPosition = height;
}

void SkyManager::setSampleNumber(int n)
{
	mSkyXOptions.NumberOfSamples = n;
}

void SkyManager::setOuterRadius(Real outr)
{
	mSkyXOptions.OuterRadius = outr;
}

void SkyManager::setInnerRadius(Real inrr)
{
	mSkyXOptions.InnerRadius = inrr;
}

void SkyManager::setExposure(Real exps)
{
	mSkyXOptions.Exposure = exps;
}

void SkyManager::setMie(Real mie)
{
	mSkyXOptions.MieMultiplier = mie;
}

void SkyManager::setRayleighMultiplier(Real rayl)
{
	mSkyXOptions.RayleighMultiplier = rayl;
}

void SkyManager::updateParameters()
{
	mSkyX->getAtmosphereManager()->setOptions(mSkyXOptions);
}

void SkyManager::create()
{
	mSkyX = new SkyX::SkyX(global::getSingleton().getSceneManager(), global::getSingleton().getCamera());
	mSkyX->create();
	mSkyX->getCloudsManager()->add(SkyX::CloudLayer::Options(/* Default options */));
	mSkyX->setTimeMultiplier(0.0f);
	mSkyXOptions = mSkyX->getAtmosphereManager()->getOptions();

}

void SkyManager::cleanup()
{
	if (mSkyX!=NULL)
	{
	delete mSkyX;
	mSkyX=0;
	}
}