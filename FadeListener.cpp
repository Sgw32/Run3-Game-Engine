/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "FadeListener.h"

template<> FadeListener *Singleton<FadeListener>::ms_Singleton=0;

FadeListener::FadeListener(){
inited=false;
fadeb=false;
wait=0;
returnafter=false;
}

FadeListener::~FadeListener()
{

}

void FadeListener::init(Ogre::Root *_root)
{
   root=_root;
   root->addFrameListener(this);
   fader = new Fader("Overlays/FadeInOut", "Materials/OverlayMaterial");
   inited=true;
}

void FadeListener::rebuild()
{
	if (inited && !fadeb)
	{
	delete fader;
	OverlayManager::getSingleton().getByName("Overlays/FadeInOut")->getChild("FadeInOutPanel")->setMaterialName("Materials/OverlayMaterial");
	fader = new Fader("Overlays/FadeInOut", "Materials/OverlayMaterial");
	}
}

void FadeListener::rebuild(String mat,bool return1)
{
	if (inited && !fadeb)
	{
	delete fader;
	OverlayManager::getSingleton().getByName("Overlays/FadeInOut")->getChild("FadeInOutPanel")->setMaterialName(mat);
	fader = new Fader("Overlays/FadeInOut", mat.c_str());
	}
	returnafter=return1;
}

void FadeListener::setDuration(Real dur)
{
	duration=dur;
}

void FadeListener::setDurationF(Real durf)
{
	durationf=durf;
}

void FadeListener::startIN()
{
	LogManager::getSingleton().logMessage("Fader:Fading in!");
	if (returnafter)
	{
		rebuild();
		returnafter=false;
	}
	fadeb=true;
	fader->startFadeIn(duration);
}

void FadeListener::startOUT()
{
	LogManager::getSingleton().logMessage("Fader:Fading out!");
	if (returnafter)
	{
		rebuild();
		returnafter=false;
	}
	fadeb=true;
	fader->startFadeOut(duration);
}

bool FadeListener::frameStarted(const Ogre::FrameEvent &evt)
{
	if (fadeb && (wait>0))
		wait-=evt.timeSinceLastFrame;
   if (fadeb && (wait<=0))
   {
		fader->fade(evt.timeSinceLastFrame);
   }
   return true;
}