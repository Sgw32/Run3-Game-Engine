#pragma once
#include "Ogre.h"
using namespace Ogre;

class LightFlasher : public ControllerValue<Real>
{
protected:
	Light* mLight;
	ColourValue mMaxColour;
	ColourValue mColourF;
	Real intensity;
public:
	LightFlasher(Light* light, ColourValue maxColour)
	{
		
		mLight = light;
		mMaxColour = maxColour;
		mColourF=light->getDiffuseColour();
		LogManager::getSingleton().logMessage("LightFlasher: light pass max color "+StringConverter::toString(maxColour)+" cur color "+StringConverter::toString(mColourF));
	}

	virtual Real  getValue (void) const
	{
		return intensity;
	}

	virtual void  setValue (Real value)
	{
		intensity = value;

		ColourValue newColour;

		// Attenuate the brightness of the light
		newColour.r = fmod(mMaxColour.r * intensity+mColourF.r,255);
		newColour.g = fmod(mMaxColour.g * intensity+mColourF.g,255);
		newColour.b = fmod(mMaxColour.b * intensity+mColourF.b,255);
		
		mLight->setDiffuseColour(newColour);
	}
};


/** Controller function mapping waveform to light intensity */
class LightFlasherControllerFunction : public WaveformControllerFunction
{
public:
	LightFlasherControllerFunction(WaveformType wavetype, Real frequency, Real phase) : WaveformControllerFunction(wavetype, 0, frequency, phase, 1, true)
	{

	}
};