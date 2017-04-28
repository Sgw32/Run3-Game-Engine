#include "MagicManager.h"
#include "global.h"

template<> MagicManager *Ogre::Singleton<MagicManager>::ms_Singleton=0;

MagicManager::MagicManager()
{
	center640=false;
	//mQueue=0;
}

MagicManager::~MagicManager()
{
}

void MagicManager::init(Camera* mCamera,RenderWindow* win,String path)
{
	new Inventory;

	pLua=global::getSingleton().getLuaState();

	lua_register(pLua, "buttonGUI_createButton",buttonGUI_createButton);
	lua_register(pLua, "buttonGUI_create3DButton",buttonGUI_create3DButton);
	lua_register(pLua, "buttonGUI_create3DButtonQuat",buttonGUI_create3DButtonQuat);
	lua_register(pLua, "buttonGUI_createButtonS",buttonGUI_createButtonS);
	lua_register(pLua, "buttonGUI_createDummy",buttonGUI_createDummy);
	lua_register(pLua, "buttonGUI_deleteAllButtons",buttonGUI_deleteAllButtons);
	lua_register(pLua, "buttonGUI_getCursX",buttonGUI_getCursX);
	lua_register(pLua, "buttonGUI_getCursY",buttonGUI_getCursY);
	lua_register(pLua, "buttonGUI_hideCursor", buttonGUI_hideCursor);
	lua_register(pLua, "buttonGUI_showCursor", buttonGUI_showCursor);
	lua_register(pLua, "buttonGUI_getX",buttonGUI_getX);
	lua_register(pLua, "buttonGUI_getY",buttonGUI_getY);
	lua_register(pLua, "buttonGUI_setPos",buttonGUI_setPos);
	lua_register(pLua, "buttonGUI_activateCenter640",buttonGUI_activateCenter640);
	lua_register(pLua, "buttonGUI_deactivateCenter640",buttonGUI_deactivateCenter640);
	lua_register(pLua, "buttonGUI_activateTopLeft640",buttonGUI_activateTopLeft640);
	lua_register(pLua, "buttonGUI_activateTopLeftComp640",buttonGUI_activateTopLeftComp640);
	lua_register(pLua, "buttonGUI_deactivate640",buttonGUI_deactivateCenter640);
	mSceneMgr=global::getSingleton().getSceneManager();
	WindowEventUtilities::addWindowEventListener(win, this);
	buttonGUI::textScheme vip("vipond",20, 0,1,0,1);  //here, i declare a simple textscheme to start with.  (just make it green all the time)

	buttonGUI::textScheme t("neuropol",22, 1.0f, 1.0f ,1.0f, 1.0f);			//for this textscheme i will be a little more specific and specify different colors for some of the events and gradients for each.
		t.cMouseOver.cTop = Ogre::ColourValue( 1.0f, 1.0f ,1.0f, 1.0f);
		t.cMouseOver.cBottom = Ogre::ColourValue( .5f, 1.0f ,.5f, 1.0f);
		t.cMouseOff.cTop = Ogre::ColourValue( 1.0f, 1.0f ,1.0f, 1.0f);
		t.cMouseOff.cBottom = Ogre::ColourValue( 0.0f, .2f ,1.0f, 1.0f);
		t.cOnRelease.cTop = Ogre::ColourValue( 0.0f, 1.0f ,1.0f, 1.0f);
		t.cOnRelease.cBottom = Ogre::ColourValue( 0.0f, .5f ,.5f, 1.0f);

	// Create the buttonManager to make our buttons
		buttonMgr = new buttonGUI::buttonManager("darkInput",t, mSceneMgr,global::getSingleton().getCamera()->getName());
		inputMgr = buttonGUI::InputManager2::getSingletonPtr();
		curs = buttonMgr->setCursor("cursorMat1", 32, 32, 3,3);
		buttonMgr->hideCursor();

		/*global::getSingleton().getCamera()->setNearClipDistance(0.01f);
		buttonMgr->showCursor(); //!!!!!
		buttonGUI::button* btn = buttonMgr->createButton("star1", "BLANK", buttonGUI::buttonPosition(buttonGUI::CENTER, -128, -128), 150,150,2,true,true,"");
			btn->setMovable(true);
			btn->addButtonMesh("star1mesh", "ninjaStar.mesh", -25,-25, 200,200)->setZoom(3);*/

    inputMgr->initialise(win);
    inputMgr->addMouseListener(this, "MagicManagerMouseListener");
	inputMgr->addKeyListener(this, "MagicManagerKeyListener");
	Inventory::getSingleton().init(buttonMgr);
	mCam=mCamera;
/*//****************************************
        /// Magic Stuff
        /// and our RenderQueueListener
        ///****************************************
	mPath=path;
	mSceneMgr=global::getSingleton().getSceneManager();
	mWindow=win;
         std::string RenderName;
         RenderName = Ogre::Root::getSingleton().getRenderSystem()->getName();
         if(RenderName == "Direct3D9 Rendering Subsystem")
          {
             void* DX9Device;
             int screenwidth  = mCamera->getViewport()->getActualWidth();
             int screenheight = mCamera->getViewport()->getActualHeight();
             mWindow->getCustomAttribute("D3DDEVICE", &DX9Device);
             if(DX9Device != 0)
              {
                   Ogre::LogManager::getSingleton().logMessage("MagicDXManager: ok , getting D3DDEVICE\n");
              }
             InitialMagicLibrary(DX9Device,screenwidth,screenheight);

		  }*/


}




void MagicManager::activateMagic(bool act)
{
	  /*           /*MyRenderQueue* MyRQueue = new MyRenderQueue(mWindow);
             mSceneMgr->addRenderQueueListener(MyRQueue);
	if (act && mQueue)
	{
		LogManager::getSingleton().logMessage("MagicManager: invalid request");
		return;
	}
	if (!act)
	{
		LogManager::getSingleton().logMessage("MagicManager: removing Render Queue");
		
		if (mQueue)
		{
			LogManager::getSingleton().logMessage("MagicManager: true");
		mSceneMgr->removeRenderQueueListener(mQueue);
		delete mQueue;	
		mQueue=0;
		}
		LogManager::getSingleton().logMessage("MagicManager: false");
	}
	if (act && !mQueue)
	{
		LogManager::getSingleton().logMessage("MagicManager: creating Render Queue");
		mQueue = new MagicRenderQueue(mWindow,mPath);
        mSceneMgr->addRenderQueueListener(mQueue);
	}*/
	/*if (act)
	{
		LogManager::getSingleton().logMessage("buttonGUIManager: invalid request");
		return;
	}*/
	if (!act)
	{
		LogManager::getSingleton().logMessage("buttonGUIManager: removing Render Queue");
		//buttonMgr->deleteAllButtons();
		std::vector<buttonGUI::button*>::iterator i;
		std::vector<buttonGUI::button*> b = buttonMgr->getAllButtons();
		for (i=b.begin();i!=b.end();i++)
			(*i)->hide(false);
		buttonMgr->hideCursor();
		LogManager::getSingleton().logMessage("buttonGUIManager: false");
		//CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", glowWasEnabled);
	}
	if (act)
	{
		LogManager::getSingleton().logMessage("buttonGUIManager: creating Render Queue1");
		buttonMgr->deleteAllButtons();
		buttonMgr->showCursor();
		//CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", false);
		//glowWasEnabled = CompositorManager::getSingleton().getCompositorEnabled(mCamera->getViewport(), "Glow");
			//create red button in the middle
		//buttonMgr->createButton("centerButton", "middleButtonMat", buttonGUI::buttonPosition(450,384), 128,128);		//notice that all the material variants are automatically detected and used

	}
}


void MagicManager::activateMagic(bool act,String lua)
{

		if (!act)
	{
		LogManager::getSingleton().logMessage("buttonGUIManager: removing Render Queue");
		buttonMgr->deleteAllButtons();
		buttonMgr->hideCursor();
		LogManager::getSingleton().logMessage("buttonGUIManager: false");
		//CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", glowWasEnabled);
	}
	if (act)
	{
		LogManager::getSingleton().logMessage("buttonGUIManager: creating Render Queue2");
		buttonMgr->showCursor();
		RunLuaScript(pLua,lua.c_str());
		//CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(), "Glow", false);
			//create red button in the middle
		//buttonMgr->createButton("centerButton", "middleButtonMat", buttonGUI::buttonPosition(450,384), 128,128);		//notice that all the material variants are automatically detected and used

	}
}


bool MagicManager::mouseMoved(const OIS::MouseEvent &arg)
{
	if(arg.state.Z.rel != 0) buttonMgr->injectMouseWheel(arg.state.Z.rel);

	return buttonMgr->injectMouseMove(arg.state.X.abs, arg.state.Y.abs);
}

bool MagicManager::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return buttonMgr->injectMouseDown(id);
}

bool MagicManager::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return buttonMgr->injectMouseUp(id);
}

bool MagicManager::keyPressed( const OIS::KeyEvent &arg )
{
	return buttonMgr->injectKeyPressed(arg);
}

bool MagicManager::keyReleased( const OIS::KeyEvent &arg )
{
	

	return buttonMgr->injectKeyReleased(arg);
}

void MagicManager::windowMoved(RenderWindow* rw) {}

void MagicManager::windowResized(RenderWindow* rw) 
{
//	inputMgr->setWindowExtents(rw->getWidth(), rw->getHeight());
	buttonMgr->resetScreenResolution();
}

void MagicManager::windowClosed(RenderWindow* rw) 
{
//	shouldQuit = true;
}

void MagicManager::windowFocusChange(RenderWindow* rw) {}


void MagicManager::upd(const FrameEvent& evt)
{
	//Inventory::getSingleton().upd(evt);
	buttonGUI::buttonEvent * e = buttonMgr->getEvent();			//THE FOLLOWING LOOP IS HOW TO GET EVENTS FROM buttonGUI
	while(e)
	{
		handleButtonEvent(e);
		e = buttonMgr->getEvent();			
	}	
	buttonMgr->update();

}

void MagicManager::handleButtonEvent(buttonGUI::buttonEvent * e)
{
	std::string name;
	if (e->actionButton)
		name = *(e->actionButton->getName()) ;  //store the name of the main button.

	if ((e->action == buttonGUI::ONCLICK)&&(e->mouseButton==OIS::MB_Left))
	{
		if (e->actionButton->mLua!="")
			RunLuaScript(pLua,e->actionButton->mLua.c_str());
		//	CWeapon::getSingleton().addWeapon("vint");
	}
}

void MagicManager::cleanup()
{

}