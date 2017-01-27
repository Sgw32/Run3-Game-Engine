/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "SuperFX.h"

template<> SuperFX *Singleton<SuperFX>::ms_Singleton=0;

SuperFX::SuperFX()
{
BloomEnabled=false;
NightvisionEnabled = false;
OldTVEnabled=false;
HDREnabled=false;
radBlurEnabled=false;
motionBlur=false;
}

SuperFX::~SuperFX()
{
}

void SuperFX::init()
{
CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Old TV");
CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Old TV", false);
CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Night Vision");
CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Night Vision", false);
CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Bloom");
CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Bloom", false);
CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Radial");
CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Radial", false);
CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "DOF");
CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "DOF", false);
lua_State* pLuaState=global::getSingleton().getLuaState();
lua_register(pLuaState, "toggleNightvision",toggleNightvision);


		//CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), comp3->getName(),true);
		//LogManager::getSingleton().logMessage("Motian blur");
}


void SuperFX::toggleMotionBlur()
{
	if (allowMb)
	{
		//LogManager::getSingleton().logMessage("Motian blur:"+comp3->getName()+" "+StringConverter::toString(comp3->isLoaded()));
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Radial",!motionBlur);
		motionBlur=!motionBlur;//LogManager::getSingleton().logMessage("Motian blur");
	}
}
void SuperFX::setBloomEnabled(bool enabled)
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Bloom", true);
	BloomEnabled=true;
}
void SuperFX::setHDREnabled(bool enabled)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getPlayer()->get_camera()->getViewport(), "NewHDR");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "NewHDR", true);
	HDREnabled=true;
}
void SuperFX::setOldTvEnabled(bool enabled)
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Old TV", true);
	OldTVEnabled=enabled;
}

void SuperFX::setNightvisionEnabled(bool enabled)
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Night Vision", true);
	NightvisionEnabled = enabled;
}

void SuperFX::toggleRadialBlur()
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Radial", !radBlurEnabled);
	radBlurEnabled=!radBlurEnabled;
}

void SuperFX::toggleNightvision()
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Night Vision", !NightvisionEnabled);
	NightvisionEnabled=!NightvisionEnabled;
}

void SuperFX::toggleOldTV()
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Old TV", !OldTVEnabled);
	OldTVEnabled=!OldTVEnabled;
}

void SuperFX::toggleBloom()
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Bloom", !BloomEnabled);
	BloomEnabled=!BloomEnabled;
}

void SuperFX::turn_off_all()
{
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Night Vision", false);
	CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getPlayer()->get_camera()->getViewport(), "Old TV", false);
}