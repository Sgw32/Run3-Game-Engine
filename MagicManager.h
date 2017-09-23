#pragma once
#include <Ogre.h>
//#include "OgreMagic.h"
#include "LuaHelperFunctions.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "Inventory.h"
#include "buttonGUI.h"
#include "InputManager2.h"
#include "Manager_Template.h"
#include "ComputerRenderQueue.h"
using namespace Ogre;

enum WRAP_TYPE
{ 
CENTER,
TOP_LEFT,
TOP_LEFT_COMP
};



class MagicManager: public Singleton<MagicManager>, public managerTemplate, public OIS::MouseListener, public OIS::KeyListener, public Ogre::WindowEventListener
{
public:
	MagicManager();
	~MagicManager();
	virtual void init(){};
	virtual void init(Camera* mCamera,RenderWindow* mWindow,String path);
	void activateMagic(bool act);

	void activateMagic(bool act,String lua);

	void activateInventory(bool act);
	void handleButtonEvent(buttonGUI::buttonEvent * e);

	Real getCursorPosX()
	{
	}

	Real getCursorPosY()
	{
	}

//	virtual void create
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
	
	inline void setGlowWasActive(bool glow)
	{
	glowWasEnabled = glow;
	}

		bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );	

	buttonGUI::buttonManager* getBMgr(void){return buttonMgr;}

	void windowMoved(Ogre::RenderWindow* rw);
	void windowResized(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);

	//buttonGUI lua binding
	
	/*Real getCursX()
	{
		MagicManager::getSingleton().curs->getPosition((short &)x,(short &)y);
	}*/
	
	static int buttonGUI_getCursX(lua_State* pL)
	{
		int x,y;
		//buttonGUI::relativePosition rel = buttonGUI::relativePosition::TOP_LEFT;
		MagicManager::getSingleton().curs->getPosition((short &)x,(short &)y);
		lua_pushnumber(pL,x);
		return 1;
	}

	static int buttonGUI_getCursY(lua_State* pL)
	{
		int x,y;
		//buttonGUI::buttonPosition* p;
		MagicManager::getSingleton().curs->getPosition((short &)x,(short &)y);
		lua_pushnumber(pL,y);
		return 1;
	}

	static int buttonGUI_hideCursor(lua_State* pL)
	{
		MagicManager::getSingleton().getBMgr()->hideCursor();
		return 1;
	}
	
	static int buttonGUI_showCursor(lua_State* pL)
	{
		MagicManager::getSingleton().getBMgr()->showCursor();
		return 1;
	}
	
	static int buttonGUI_getX(lua_State* pL)
	{
		short x,y;
		x=y=0;
		//buttonGUI::relativePosition rel = buttonGUI::relativePosition::TOP_LEFT;
		String name = lua_tostring(pL, 1);
		MagicManager::getSingleton().getBMgr()->getButton(name)->getPosition((short &)x,(short &)y);
		LogManager::getSingleton().logMessage(StringConverter::toString(x));
		lua_pushnumber(pL,x);
		return 1;
	}

	static int buttonGUI_getY(lua_State* pL)
	{
		short x,y;
		x=y=0;
		//buttonGUI::relativePosition rel = buttonGUI::relativePosition::TOP_LEFT;
		String name = lua_tostring(pL, 1);
		MagicManager::getSingleton().getBMgr()->getButton(name)->getPosition((short &)x,(short &)y);
		lua_pushnumber(pL,y);
		return 1;
	}

	static int buttonGUI_setPos(lua_State* pL)
	{
		//short x,y;
		String name = lua_tostring(pL, 1);
		Real x1 = StringConverter::parseReal(lua_tostring(pL, 2));
		Real y1 = StringConverter::parseReal(lua_tostring(pL, 3));
		MagicManager::getSingleton().getBMgr()->getButton(name)->setPosition(buttonGUI::buttonPosition(x1,y1));
		return 1;
	}
	
	static int buttonGUI_createButton(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=5)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5))
		{
		String name = lua_tostring(pL, 1);
		String matname =  lua_tostring(pL, 2);
		Ogre::Vector2 position = StringConverter::parseVector2(lua_tostring(pL, 3));
		Ogre::Vector2 size = StringConverter::parseVector2(lua_tostring(pL, 4));
		if (MagicManager::getSingleton().center640)
		{
			Real x = MagicManager::getSingleton().mCam->getViewport()->getActualWidth();
			Real y = MagicManager::getSingleton().mCam->getViewport()->getActualHeight();
			switch (MagicManager::getSingleton().wrap_type)
			{
			case CENTER:
				position+=Vector2(x/2-320,y/2-240);
				break;
			case TOP_LEFT:
				position.x*=x/640;
				position.y*=y/480;
				size.x*=x/640;
				size.y*=y/480;
				break;
			case TOP_LEFT_COMP:
				LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
				LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
				position.x*=512.0f/640.0f;
				position.y*=512.0f/480.0f;
				size.x*=512.0f/640.0f;
				size.y*=512.0f/480.0f;
				LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
				LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
				break;
			default:
				break;
			}
			
		}
		String lue = lua_tostring(pL, 5);
		if (MagicManager::getSingleton().getBMgr()->getButton(name)==0)
		{
		MagicManager::getSingleton().getBMgr()->
			createButton(name, matname, buttonGUI::buttonPosition(position.x,position.y), size.x,size.y,0,true,true,lue);
		}
		else
		{
		MagicManager::getSingleton().getBMgr()->getButton(name)->show(true);
		}
		}
		return 1;
	}


	static int buttonGUI_create3DButton(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=6)
		{
			LogManager::getSingleton().logMessage("Creating 3d button!");
			buttonGUI::button* btn = MagicManager::getSingleton().getBMgr()->createButton("star1", "BLANK", buttonGUI::buttonPosition(buttonGUI::CENTER, -128, -128), 150,150,2,true,true,"");
			btn->setMovable(true);
			btn->addButtonMesh("star1mesh", "ninjaStar.mesh", 0,0, 150,150)->setZoom(50);
			return 1;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5)&&lua_isstring(pL, 6))
		{
			LogManager::getSingleton().logMessage("Creating 3d button!");

			String name = lua_tostring(pL, 1);
			String meshname =  lua_tostring(pL, 2);
			Ogre::Vector2 position = StringConverter::parseVector2(lua_tostring(pL, 3));
			Ogre::Vector2 size = StringConverter::parseVector2(lua_tostring(pL, 4));

			if (MagicManager::getSingleton().center640)
			{
				Real x = MagicManager::getSingleton().mCam->getViewport()->getActualWidth();
				Real y = MagicManager::getSingleton().mCam->getViewport()->getActualHeight();
				switch (MagicManager::getSingleton().wrap_type)
				{
				case CENTER:
					position+=Vector2(x/2-320,y/2-240);
					break;
				case TOP_LEFT:
					position.x*=x/640;
					position.y*=y/480;
					size.x*=x/640;
					size.y*=y/480;
					break;
				case TOP_LEFT_COMP:
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
					position.x*=512.0f/640.0f;
					position.y*=512.0f/480.0f;
					size.x*=512.0f/640.0f;
					size.y*=512.0f/480.0f;
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
					break;
				default:
					break;
				}
				
			}
			String lue = lua_tostring(pL, 6);

			if (MagicManager::getSingleton().getBMgr()->getButton(name)==0)
			{
			buttonGUI::button* btn = MagicManager::getSingleton().getBMgr()->
				createButton(name, "BLANK", buttonGUI::buttonPosition(position.x,position.y), size.x,size.y,0,true,true,lue);
			btn->setMovable(true);
			btn->addButtonMesh(name+"_mesh", meshname, 0,0, size.x,size.y)->setZoom(StringConverter::parseReal(lua_tostring(pL, 5)));
			}
			else
			{
			MagicManager::getSingleton().getBMgr()->getButton(name)->show(true);
			}
		}
		return 1;
	}

	static int buttonGUI_create3DButtonQuat(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=7)
		{
			LogManager::getSingleton().logMessage("Creating 3d button!");
			buttonGUI::button* btn = MagicManager::getSingleton().getBMgr()->createButton("star1", "BLANK", buttonGUI::buttonPosition(buttonGUI::CENTER, -128, -128), 150,150,2,true,true,"");
			btn->setMovable(true);
			btn->addButtonMesh("star1mesh", "ninjaStar.mesh", 0,0, 150,150)->setZoom(50);
			return 1;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5)&&lua_isstring(pL, 6)&&lua_isstring(pL, 6))
		{
			LogManager::getSingleton().logMessage("Creating 3d button!");

			String name = lua_tostring(pL, 1);
			String meshname =  lua_tostring(pL, 2);
			Ogre::Vector2 position = StringConverter::parseVector2(lua_tostring(pL, 3));
			Ogre::Vector2 size = StringConverter::parseVector2(lua_tostring(pL, 4));

			if (MagicManager::getSingleton().center640)
			{
				Real x = MagicManager::getSingleton().mCam->getViewport()->getActualWidth();
				Real y = MagicManager::getSingleton().mCam->getViewport()->getActualHeight();
				switch (MagicManager::getSingleton().wrap_type)
				{
				case CENTER:
					position+=Vector2(x/2-320,y/2-240);
					break;
				case TOP_LEFT:
					position.x*=x/640;
					position.y*=y/480;
					size.x*=x/640;
					size.y*=y/480;
					break;
				case TOP_LEFT_COMP:
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
					position.x*=512.0f/640.0f;
					position.y*=512.0f/480.0f;
					size.x*=512.0f/640.0f;
					size.y*=512.0f/480.0f;
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(position));
					LogManager::getSingleton().logMessage("Transform: "+StringConverter::toString(size));
					break;
				default:
					break;
				}
				
			}
			String lue = lua_tostring(pL, 6);

			if (MagicManager::getSingleton().getBMgr()->getButton(name)==0)
			{
			buttonGUI::button* btn = MagicManager::getSingleton().getBMgr()->
				createButton(name, "BLANK", buttonGUI::buttonPosition(position.x,position.y), size.x,size.y,0,true,true,lue);
			btn->setMovable(true);
			btn->addButtonMesh(name+"_mesh", meshname, 0,0, size.x,size.y)->setZoom(StringConverter::parseReal(lua_tostring(pL, 5)))->setRotation(StringConverter::parseQuaternion(lua_tostring(pL, 6)));
			}
			else
			{
			MagicManager::getSingleton().getBMgr()->getButton(name)->show(true);
			}
		}
		return 1;
	}

//buttonGUI lua binding
	static int buttonGUI_deleteAllButtons(lua_State* pL)
	{
		
		std::vector<buttonGUI::button*>::iterator i;
		std::vector<buttonGUI::button*> b = MagicManager::getSingleton().getBMgr()->getAllButtons();
		for (i=b.begin();i!=b.end();i++)
			(*i)->hide(true);
		return 1;
	}

	static int buttonGUI_activateCenter640(lua_State* pL)
	{
		
		MagicManager::getSingleton().center640=true;
		MagicManager::getSingleton().wrap_type=CENTER;
		return 1;
	}

	static int buttonGUI_activateTopLeft640(lua_State* pL)
	{
		
		MagicManager::getSingleton().center640=true;
		MagicManager::getSingleton().wrap_type=TOP_LEFT;
		return 1;
	}

	static int buttonGUI_activateTopLeftComp640(lua_State* pL)
	{
		
		MagicManager::getSingleton().center640=true;
		MagicManager::getSingleton().wrap_type=TOP_LEFT_COMP;
		return 1;
	}

	static int buttonGUI_deactivateCenter640(lua_State* pL)
	{
		
		MagicManager::getSingleton().center640=false;
		return 1;
	}

	static int buttonGUI_createButtonS(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=5)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5))
		{
		String name = lua_tostring(pL, 1);
		String matname =  lua_tostring(pL, 2);
		Ogre::Vector2 position = StringConverter::parseVector2(lua_tostring(pL, 3));
		Ogre::Vector2 size = StringConverter::parseVector2(lua_tostring(pL, 4));
		if (MagicManager::getSingleton().center640)
		{
			Real x = MagicManager::getSingleton().mCam->getViewport()->getActualWidth();
			Real y = MagicManager::getSingleton().mCam->getViewport()->getActualHeight();
			position+=Vector2(x/2-320,y/2-240);
		}
		String lue = lua_tostring(pL, 5);
		if (MagicManager::getSingleton().getBMgr()->getButton(name)==0)
		{
			buttonGUI::button* b = MagicManager::getSingleton().getBMgr()->
			createButton(name, matname, buttonGUI::buttonPosition(position.x,position.y), size.x,size.y,0,true,true,lue);
			b->setMovable(false);
		}
		}
		return 1;
	}

	static int buttonGUI_createDummy(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=5)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4)&&lua_isstring(pL, 5))
		{
		String name = lua_tostring(pL, 1);
		String matname =  lua_tostring(pL, 2);
		Ogre::Vector2 position = StringConverter::parseVector2(lua_tostring(pL, 3));
		Ogre::Vector2 size = StringConverter::parseVector2(lua_tostring(pL, 4));
		if (MagicManager::getSingleton().center640)
		{
			Real x = MagicManager::getSingleton().mCam->getViewport()->getActualWidth();
			Real y = MagicManager::getSingleton().mCam->getViewport()->getActualHeight();
			position+=Vector2(x/2-320,y/2-240);
		}
		String lue = lua_tostring(pL, 5);
		if (MagicManager::getSingleton().getBMgr()->getButton(name)==0)
		{
			buttonGUI::button* b = MagicManager::getSingleton().getBMgr()->
			createButton(name, matname, buttonGUI::buttonPosition(position.x,position.y), size.x,size.y,0,true,true,lue);
			b->setActive(false);
		}
		}
		return 1;
	}
	
	bool center640;
	int wrap_type;
	Camera* mCam;
private:
	String mPath;
	
	bool activated;
	bool glowWasEnabled;
	//MagicRenderQueue* mQueue;
	lua_State* pLua;
buttonGUI::buttonManager* buttonMgr;
buttonGUI::button* curs;
	buttonGUI::InputManager2* inputMgr;
	SceneManager* mSceneMgr;
	RenderWindow* mWindow;
};