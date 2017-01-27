
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "HUD.h"
#include "global.h"

template<> HUD *Singleton<HUD>::ms_Singleton=0;

HUD::HUD()
{
initial=false;
startRollout=false;
leftOrRight=true;
wait=false;
shootCrosshair=false;
wTime=0;
wTime2=100000;
textFlash=false;
textFlash2=false;
bloodonscreen=false;
wTime3=0;
HUDAllowed=true;
}

HUD::~HUD()
{

}

void HUD::setHUDAllowed(bool allowed)
{
	HUDAllowed=allowed;
}

void HUD::SSBlood()
{
	   wTime3=0;
		bloodonscreen=true;
		CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"ColorCorrection");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"ColorCorrection",true);
		MaterialPtr mActiveMaterial = Ogre::MaterialManager::getSingleton().getByName("Ogre/Compositor/RunColorCorrection");
			GpuProgramPtr mActiveFragmentProgram = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgram();
			mActiveFragmentParameters = mActiveMaterial->getSupportedTechnique(0)->getPass(0)->getFragmentProgramParameters();
			mActiveFragmentParameters->setNamedConstant("rc",1.0f);
			mActiveFragmentParameters->setNamedConstant("bc",0.0f);
			mActiveFragmentParameters->setNamedConstant("gc",0.0f);
}


void HUD::setTextColor(ColourValue tc)
{
	ColourValue texcolor = gMsgTextArea->getColour();
	texcolor.r=tc.r;
	texcolor.g=tc.g;
	texcolor.b=tc.b;
	gMsgTextArea->setColour(texcolor);
// unused -> replaced by flashPanel
}

void HUD::setTextSize(Real size)
{
	
	((TextAreaOverlayElement*)gMsgTextArea)->setCharHeight(size);
	LogManager::getSingleton().logMessage("Set to:"+
		StringConverter::toString(((TextAreaOverlayElement*)gMsgTextArea)->getCharHeight()));
// unused -> replaced by flashPanel
}


void HUD::resetTextColor()
{
	ColourValue texcolor = gMsgTextArea->getColour();
	texcolor.r=1;
	texcolor.g=1;
	texcolor.b=1;
	((TextAreaOverlayElement*)gMsgTextArea)->setColour(texcolor);
}

void HUD::resetTextSize()
{
	((TextAreaOverlayElement*)gMsgTextArea)->setCharHeight(defaultCharHeight);
}

void HUD::flashText(String text,Real fade,Real stay)
{
	textFlash=true;
	gMsgTextArea->setCaption(text);
	//gMsgTextArea->
	gameMsg->show();
	fadeTime=fade;
	stayTime=stay;
	wTime2 = 0;
	ColourValue texcolor = gMsgTextArea->getColour();
			texcolor.a=0;
			gMsgTextArea->setColour(texcolor);
// unused -> replaced by flashPanel
}

void HUD::flashText2(String text,Real fade,Real stay)
{
	//textFlash=true;
	textFlash2=true;
	flText2=text;
	flashText("",fade,stay);
}

void HUD::init(bool show,Ogre::Root* root)
{
	mainHUD=OverlayManager::getSingleton().getByName("Run3/HUD");
	mainHUD->show();
	gameMsg=OverlayManager::getSingleton().getByName("Run3/GameText");
	gameMsg->hide();
	HUDAmmo=0;
	HUDAmmoInd=0;
	HUDAmmo=mainHUD->getChild("Run3/HUDAmmo");
	gMsgText=gameMsg->getChild("Run3/GameTextCont");
	gMsgTextArea=gMsgText->getChild("Run3/TextArea");
	gMsgTextArea->setCaption("");
	defaultCharHeight=((TextAreaOverlayElement*)gMsgTextArea)->getCharHeight();
	crosshair=OverlayManager::getSingleton().getByName("Run3/CrosshairO");
	crosshair->hide();
	if (crosshair->getChild("Run3/CrossRing"))
	{
			crossTS=crosshair->getChild("Run3/CrossRing")->getTop();
	crossLS=crosshair->getChild("Run3/CrossRing")->getLeft();
			crosshair->getChild("Run3/CrossRing")->hide();
	}
	HUDHealth=mainHUD->getChild("Run3/HUDHealth");
	HUDREnergy=mainHUD->getChild("Run3/HUDEnergyR");
	HUDGEnergy=mainHUD->getChild("Run3/HUDEnergyG");
	HUDBEnergy=mainHUD->getChild("Run3/HUDEnergyB");
	HealthIndicator=HUDHealth->getChild("Run3/HUDHealthInd");
	HealthIndicator->setCaption("100");
	sHeight=HUDREnergy->getHeight();
	if (show)
	{
		mainHUD->show();
	}
	else
	{
		mainHUD->hide();
	}
	initial=true;
	infoPanel=OverlayManager::getSingleton().getByName("Run3/Info");
	infoPanel->hide();
	infoPanelCont=infoPanel->getChild("Run3/InfoPanelOverl");
	text=infoPanelCont->getChild("Run3/InfoText");
	root->addFrameListener(this);
}

void HUD::setCrosshairShiftTopLeft(Real top_s,Real left_s)
{
	if (crosshair)
	{
		//crossTS
			if (crosshair->getChild("Run3/CrossRing"))
			{
				//LogManager::getSingleton().logMessage("X="+StringConverter::toString(top_s)+"Y="+StringConverter::toString(left_s));
				crosshair->getChild("Run3/CrossRing")->setTop(crossTS-top_s);
				crosshair->getChild("Run3/CrossRing")->setLeft(crossLS-left_s);
			}
	}
}

void HUD::ChangeHUDOverlay(String overlay,bool energy)
{
	if (initial)
	{
		mainHUD->hide();
		mainHUD=OverlayManager::getSingleton().getByName(overlay);
		HUDHealth=mainHUD->getChild(overlay+"Health");
		HUDAmmo=0;
		HUDAmmoInd=0;
		HUDAmmo=mainHUD->getChild(overlay+"Ammo");
		if (HUDAmmo)
			HUDAmmoInd=HUDAmmo->getChild(overlay+"AmmoInd");
		if (energy)
		{
		HUDREnergy=mainHUD->getChild(overlay+"EnergyR");
		HUDGEnergy=mainHUD->getChild(overlay+"EnergyG");
		HUDBEnergy=mainHUD->getChild(overlay+"EnergyB");
		}
		if (HealthIndicator)
		{
		HealthIndicator=HUDHealth->getChild(overlay+"HealthInd");
		HealthIndicator->setCaption("100");
		}
		if (HUDREnergy)
			sHeight=HUDREnergy->getHeight();
		mainHUD->show();
	}
}

void HUD::SetHealth(int health)
{
	if (HealthIndicator)
	HealthIndicator->setCaption(StringConverter::toString(health));
}

void HUD::Show()
{
	if (HUDAllowed)
	{
	mainHUD->show();
	crosshair->show();
	}
}

void HUD::ShowOverride()
{
	mainHUD->show();
	crosshair->show();
}


void HUD::Hide()
{
	mainHUD->hide();
	crosshair->hide();
}

void HUD::HideOverride()
{
	if (!HUDAllowed)
	{
	mainHUD->hide();
	crosshair->hide();
	}
}

void HUD::Hide_crosshair()
{
	crosshair->hide();
}

void HUD::Show_crosshair()
{
	crosshair->show();
}

bool HUD::IsVisibleCross()
{
	return crosshair->isVisible();
}

void HUD::shutdown()
{
	if (!initial) 
		return;
   mainHUD->hide();
   mainHUD->clear();
   gameMsg->hide();
   gameMsg->clear();
   infoPanel->hide();
   infoPanel->clear();
}

void HUD::processFlashText2String(Real wTime)
{
	if (!textFlash2)
		return;
	Real sym_count;
	/*if ((wTime>fadeTime)&&(wTime<(fadeTime+stayTime/2)))
	{
	sym_count = flText2.length()*(wTime-fadeTime)/(stayTime/2); // int, of course.
	}
	else
	{
		if ((wTime>(fadeTime+stayTime/2))&&(wTime<(fadeTime+stayTime)))
		{
		sym_count = flText2.length()*(fadeTime+stayTime-wTime)/(stayTime/2); // int, of course.
		}
		else
		{
		sym_count=0;
		}
	}*/
	if ((wTime>fadeTime)&&(wTime<(fadeTime+stayTime)))
	{
		sym_count = flText2.length()*(wTime-fadeTime)/(stayTime); // int, of course.
	}
	else
	{
		if (wTime>(fadeTime+stayTime))
		{
			sym_count = flText2.length();
		}
		else
		{
			sym_count=0;
		}
	}
	if ((int)(wTime*2)%2==0) // 2 Hz loop.
	{
		gMsgTextArea->setCaption(flText2.substr(0,(int)sym_count)+"_");
	}
	if ((int)(wTime*2)%2==1) // 2 Hz loop.
	{
		gMsgTextArea->setCaption(flText2.substr(0,(int)sym_count));
	}
}

bool HUD::frameRenderingQueued(const Ogre::FrameEvent &evt)
{
	//mainHUD
	if (bloodonscreen)
	{
		wTime3+=evt.timeSinceLastFrame;
		cc=wTime3/DAMAGE_BLOOD;
		CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"ColorCorrection");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"ColorCorrection",true);
		mActiveFragmentParameters->setNamedConstant("bc",cc);
		mActiveFragmentParameters->setNamedConstant("gc",cc);
		if (wTime3>DAMAGE_BLOOD)
		{
			wTime3=0;
			bloodonscreen=false;
			CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"ColorCorrection");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"ColorCorrection",false);
			mActiveFragmentParameters->setNamedConstant("rc",1.0f);
			mActiveFragmentParameters->setNamedConstant("bc",1.0f);
			mActiveFragmentParameters->setNamedConstant("gc",1.0f);
		}
	}

	// textFlash and textFlash2 function
	if ((textFlash) &&(evt.timeSinceLastFrame<3.0f))
		wTime2+=evt.timeSinceLastFrame;

	
	if (wTime2<(fadeTime*2+stayTime)) // If so, it means that fade is in processing. 
	{
		processFlashText2String(wTime2);
		if (wTime2 < fadeTime) // Starting fade. 
		{
			ColourValue texcolor = gMsgTextArea->getColour();
			texcolor.a=wTime2/fadeTime; // Great! From 0 to 1. 
			//LogManager::getSingleton().logMessage("1 "+StringConverter::toString(texcolor.a));
			gMsgTextArea->setColour(texcolor);
		}
		if (wTime2 >= fadeTime && wTime2<= (fadeTime+stayTime)) // Static or dynamic display.
		{
			ColourValue texcolor = gMsgTextArea->getColour();
			texcolor.a=1;
			//LogManager::getSingleton().logMessage("2 "+StringConverter::toString(texcolor.a));
			gMsgTextArea->setColour(texcolor);
			//starting fade from zero alpha
			
		}
		if (wTime2> (fadeTime+stayTime))
		{
			ColourValue texcolor = gMsgTextArea->getColour();
			texcolor.a=1-(wTime2-stayTime-fadeTime)/fadeTime;
			//LogManager::getSingleton().logMessage("3 "+StringConverter::toString(texcolor.a));
			gMsgTextArea->setColour(texcolor);
			//starting fade to zero alpha
			
		}
		
	}
	else
	{
		//LogManager::getSingleton().logMessage("HIIDRE");
		ColourValue texcolor = gMsgTextArea->getColour();
		texcolor.a=0;
			gMsgTextArea->setColour(texcolor);
			gMsgTextArea->setCaption("");
			gameMsg->hide();
			wTime2=fadeTime*2+stayTime;
			//New 
			textFlash=false;
			textFlash2=false;
	}


	if (startRollout&&leftOrRight)
	{
		Real left = infoPanelCont->getLeft();
		if (!wait)
		{
		
		infoPanelCont->setLeft(left-evt.timeSinceLastFrame*300);
		}
		if (left<-MAX_PANEL_ROLLOUT)
		{
			//startRollout=false;
			infoPanelCont->setLeft(-MAX_PANEL_ROLLOUT);
			wait=true;
			//leftOrRight=!leftOrRight;
		}
	}

	if (wait)
	{
		wTime+=evt.timeSinceLastFrame;
		if (wTime>5)
		{
			wait=false;
			leftOrRight=!leftOrRight;
			wTime=0;
		}
	}

	if ((startRollout)&&(!leftOrRight))
	{
		Real left = infoPanelCont->getLeft();
		infoPanelCont->setLeft(left+evt.timeSinceLastFrame*300);
		if (left>0)
		{
			startRollout=false;
			leftOrRight=true;
			infoPanelCont->setLeft(0);
			startRollout=false;
		}
	}

	if (shootCrosshair)
	{
		crosshair->setScale(crosshair->getScaleX()*1.2,crosshair->getScaleY()*1.2);
		if (crosshair->getScaleX()>2)
		{
			crosshair->setScale(1,1);
			shootCrosshair=false;
		}
	}

	return true;
}

void HUD::flashPanel(String tex)
{
	infoPanel->show();
	infoPanelCont->setLeft(0);
	this->text->setCaption(tex);
	startRollout=true;
	wait=false;
	leftOrRight=true;
}

bool HUD::frameStarted(const Ogre::FrameEvent &evt)
{
	return true;
}

void HUD::SetRedEnergy(Real energy)
{
	if (HUDREnergy)
	HUDREnergy->setHeight(sHeight*energy);
}

void HUD::SetGreenEnergy(Real energy)
{
	if (HUDGEnergy)
	HUDGEnergy->setHeight(sHeight*energy);
}

void HUD::SetBlueEnergy(Real energy)
{
	if (HUDBEnergy)
	HUDBEnergy->setHeight(sHeight*energy);
}