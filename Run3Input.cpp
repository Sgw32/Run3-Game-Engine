/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Run3Input.h"
#include "Run3SoundRuntime.h"
#include "ogreconsole.h"

template<> Run3Input *Ogre::Singleton<Run3Input>::ms_Singleton=0;

Run3Input::Run3Input()
{

}

Run3Input::~Run3Input()
{

}

void Run3Input::init(Ogre::Overlay* menuOverlay,CEGUI::Window* shit)
{
	player = global::getSingleton().getPlayer();
	mMenuOverlay = menuOverlay;
	sheet = shit;
}
void Run3Input::processPress(const OIS::KeyEvent &arg,bool &GUIorGame,bool ingame)
{
	if (global::getSingleton().GUIorGame)
		{
		CEGUI::System *sys = CEGUI::System::getSingletonPtr();
        sys->injectKeyDown(arg.key);
        sys->injectChar(arg.text);
        CEGUI::System::getSingleton().injectKeyUp(arg.key);
			if (arg.key == OIS::KC_ESCAPE && ingame)
			{
				mMenuOverlay->hide();
				sheet->hide();
				OgreConsole::getSingleton().print("unpaused..");
				CEGUI::MouseCursor::getSingleton().hide();
				HUD::getSingleton().Show();
				global::getSingleton().GUIorGame=false;
				return;
			}
		}

		if (!global::getSingleton().GUIorGame)
		{
			if (!OgreConsole::getSingleton().isVisible())
			{
				if (arg.key == OIS::KC_ESCAPE)
				{
					global::getSingleton().GUIorGame=true;
					mMenuOverlay->show();
					sheet->show();
					 CEGUI::MouseCursor::getSingleton().show();
					 OgreConsole::getSingleton().print("paused..");
					 HUD::getSingleton().Hide();
					 return;
				}
				if (arg.key==OIS::KC_SLASH)
				{
					player->cnoclip();
				}
				bool dispVisible = Display::getSingleton().isVisible();
				if (!dispVisible)
				{
				if (arg.key==OIS::KC_P)
				{
					player->debug++;
				}
				if (arg.key==OIS::KC_E)
				{
					player->processUse();
				}
				if (arg.key==OIS::KC_B)
				{
					SuperFX::getSingleton().toggleBloom();
				}

				if (arg.key==OIS::KC_F)
				{
					player->toggleFlashLight();
				}
				if (arg.key==OIS::KC_I)
				{
					Inventory::getSingleton().display();
				}				
					if (arg.key==OIS::KC_M)
				{
					if (global::getSingleton().deferred)
					{
					DeferredShadingSystem* iSystem = global::getSingleton().iSystem;

			iSystem->setMode(
				(DeferredShadingSystem::DSMode)
				((iSystem->getMode() + 1)%DeferredShadingSystem::DSM_COUNT)
				);
					}
				}
				player->FCPress(arg);
				}
				/*if (arg.key==OIS::KC_Q)
				{
					if (Display::getSingleton().isVisible)
					{
						//Display::getSingleton().hide();
						Display::getSingleton().reset();
					}
					
				}*/
				/*if (arg.key==OIS::KC_O)
				{
					//SuperFX::getSingleton().init();
					SuperFX::getSingleton().toggleOldTV();
				}
				if (arg.key==OIS::KC_N)
				{
					SuperFX::getSingleton().toggleNightvision();
				}*/
				
			}
		}
			//Only ingame with noclip:
		if (!global::getSingleton().GUIorGame && player->noclip)
		{
				switch (arg.key)
				{
					case OIS::KC_ESCAPE: 
						global::getSingleton().GUIorGame=true;
						mMenuOverlay->show();
						sheet->show();
						CEGUI::MouseCursor::getSingleton().show();
						OgreConsole::getSingleton().print("paused..");
						HUD::getSingleton().Hide();
						break;
					default:
						break;
				}
		}

		if (!global::getSingleton().GUIorGame && !player->noclip)
		{

		}
		
}
void Run3Input::processRelease(const OIS::KeyEvent &arg,bool GUIorGame,bool ingame)
{
	if (!global::getSingleton().GUIorGame)
		{
			if (!OgreConsole::getSingleton().isVisible())
			{
				bool dispVisible = Display::getSingleton().isVisible();
				if (!dispVisible)
				{
if (arg.key==OIS::KC_I)
				{
					Inventory::getSingleton().destroy();
				}	
				}
			}
		}
	
}