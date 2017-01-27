/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "OgreConsole.h"
#include <OIS/OIS.h>

#define MAX_PANEL_ROLLOUT 500
#define DAMAGE_BLOOD 4

class HUD: public Singleton<HUD>, FrameListener
{
public:
   HUD();
   ~HUD();
   void init(bool show,Ogre::Root* root);
   void ChangeHUDOverlay(String overlay,bool energy);
   void shutdown();
   void flashText(String text,Real fadein,Real fadeout);
   void flashText2(String text,Real fadein,Real fadeout);
   void flashPanel(String text);
   void shoot(){shootCrosshair=true;}
   void destroyPanel();
   void hidePanel();
   void Show();
   void ShowOverride();
   void setTextColor(ColourValue tc);
   void setTextSize(Real size);
   void resetTextColor();
   void resetTextSize();
   void Show_crosshair();
   bool IsVisibleCross();
   void Hide();
   void HideOverride();
   void Hide_crosshair();
   void SetHealth(int health);
   void SetRedEnergy(Real energy);
   void SetGreenEnergy(Real energy);
   void SetBlueEnergy(Real energy);
   void setCrosshairShiftTopLeft(Real top_s,Real left_s);
   void SSBlood();
   
   void activateCrosshairRing(bool act)
   {
	 if (crosshair->getChild("Run3/CrossRing"))
	 {
		 if (act)
		 {
			crosshair->getChild("Run3/CrossRing")->show();
			}
		 else
			crosshair->getChild("Run3/CrossRing")->hide();
	 }
   }
   void setRingMaterial(String mat)
   {
	   if (crosshair->getChild("Run3/CrossRing"))
			crosshair->getChild("Run3/CrossRing")->setMaterialName(mat);
   }
    void setCrosshairMaterial(String mat)
   {
	   //LogManager::getSingleton().logMessage(mat);
	   if (crosshair->getChild("Run3/CrossPanel"))
			crosshair->getChild("Run3/CrossPanel")->setMaterialName(mat);
   }
   void setAmmo(String txt)
   {
	   if (HUDAmmo)
		   HUDAmmoInd->setCaption(txt);
   }

   void processFlashText2String(Real wTime);

   void setHUDAllowed(bool allowed);
   Overlay* crosshair;
   virtual bool frameStarted(const Ogre::FrameEvent &evt);
   virtual bool frameRenderingQueued(const Ogre::FrameEvent &evt);
private:
	GpuProgramParametersSharedPtr mActiveFragmentParameters;
	Overlay* mainHUD;
	Overlay* gameMsg;
	OverlayContainer* gMsgText;
	OverlayContainer* HUDHealth;
	OverlayContainer* HUDREnergy;
	OverlayContainer* HUDGEnergy;
	OverlayContainer* HUDBEnergy;
	OverlayContainer* HUDAmmo;
	OverlayElement* HealthIndicator;
	OverlayElement* HUDAmmoInd;
	OverlayElement* gMsgTextArea;
	Real crossTS,crossLS;
	Real sHeight;
	Real defaultCharHeight;
	bool bloodonscreen;
	bool initial;
	bool textFlash;
	bool HUDAllowed;
	Real cc;
	Overlay* infoPanel;
	OverlayContainer* infoPanelCont;
	OverlayElement* text;
	String flText2;
	Real wTime;
	Real wTime2,wTime3;
	Real fadeTime,stayTime;
	bool shootCrosshair;
	bool startRollout;
	bool textFlash2;
	bool leftOrRight;
	bool wait;
};