#include "Inventory.h"
#include "global.h"

template<> Inventory *Ogre::Singleton<Inventory>::ms_Singleton=0;

Inventory::Inventory()
{
	visi=false;
	enabled=true;
}

Inventory::~Inventory()
{
}

void Inventory::init()
{

}

void Inventory::enableInventory()
{
	enabled=true;
}

void Inventory::disableInventory()
{
	enabled=false;
}

void Inventory::setVisible(bool vis)
{
	if (vis)
		display();
	else
		destroy();
}

void Inventory::display()
{
	if (enabled)
	{
		visi=true;

		LogManager::getSingleton().logMessage("buttonGUIManager: creating Render Queue");
		buttonMgr->showCursor();
		HUD::getSingleton().ShowOverride();
		RunLuaScript(global::getSingleton().getLuaState(),"run3/lua/funcs/inventory.lua");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", false);
	}
			//create red button in the middle
		//buttonMgr->createButton("centerButton", "middleButtonMat", buttonGUI::buttonPosition(450,384), 128,128);		//notice that all the material variants are automatically detected and used

}

void Inventory::destroy()
{
	visi=false;
	HUD::getSingleton().HideOverride();
			buttonMgr->deleteAllButtons();
		buttonMgr->hideCursor();
		LogManager::getSingleton().logMessage("buttonGUIManager: false");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", glowWasEnabled);
	
}
	
void Inventory::upd(const FrameEvent& evt)
{
	/*buttonGUI::buttonEvent * e = buttonMgr->getEvent();			//THE FOLLOWING LOOP IS HOW TO GET EVENTS FROM buttonGUI
	while(e)
	{
		handleButtonEvent(e);
		e = buttonMgr->getEvent();			
	}	
	buttonMgr->update();*/
}

void Inventory::handleButtonEvent(buttonGUI::buttonEvent * e)
{
	/*std::string name;
	if (e->actionButton)
		name = *(e->actionButton->getName()) ;  //store the name of the main button.

	if ((e->action == buttonGUI::ONCLICK))
	{
			CWeapon::getSingleton().addWeapon("vint");
	}*/
}

void Inventory::cleanup()
{
}

