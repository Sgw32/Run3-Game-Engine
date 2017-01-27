#pragma once
#include "Ogre.h"
#include "global.h"
#include "LuaHelperFunctions.h"
#include "DisplayLuaCallback.h"

  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
#include <list>

#define ELISA

#ifdef ELISA
	#include "Eliza.h"
#endif

using namespace Ogre;
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

//#define LUASTRINGDISPLAYMACROS 
//String toWrite;

class Display: public Ogre::Singleton<Display>, FrameListener
{
public:
	Display();
	~Display();
	void init(bool shows,lua_State* pL);
	void   setVisible(bool visible);
    bool   isVisible(){return visible;}
    void shutdown();
	void show();
	void hide();
	void setText();
	void appendText();
	void writeLnD(String& text);
	void runScript(String luas);
	void activateElisa()
	{
		LogManager::getSingleton().logMessage("Initializing Elisa...");
#ifdef ELISA
		if (elisa_chat)
			return;
		try
		{
		eliza.load_data();
		eliza.start();
		}
		catch(...)
		{
			LogManager::getSingleton().logMessage("Error initializing Elisa!");
		}
#endif
		LogManager::getSingleton().logMessage("Chatterbot Eliza v2.0 Copyright(c) 2005 - 2006 Gonzales Cenelia");
		elisa_chat=true;
		writeLnD(String("**** STARTED CHAT ****"));
	}
	void deactivateElisa()
	{
		LogManager::getSingleton().logMessage("Saving Elisa database...");
#ifdef ELISA
		try
		{
			if(eliza.learn()) {
				eliza.save_data();
			}
			eliza.save_unknown_sentences();
		}
		catch(...)
		{
			LogManager::getSingleton().logMessage("Error saving Elisa!");
		}
#endif
		elisa_chat=false;
		writeLnD(String("**** ENDED CHAT ****"));
	}

	void setAllowVirtualDisplay(bool allow)
	{
		allowVirtualDisplay=allow;
	}
	void clrScr()
	{
		lines.clear();
	}
	void setColor(ColourValue c)
	{
		textbox->setColour(c);
	};

   virtual bool frameStarted(const Ogre::FrameEvent &evt);
   virtual bool frameEnded(const Ogre::FrameEvent &evt);

   void onKeyPressed(const OIS::KeyEvent &arg);
    void setMaterial(String name);

	void processLuaFunction(vector<String> params);

	void reset();
	
	String getCommand()
	{
		String mCmd = mCommand;
		mCommand="";
		return mCmd;
	}

	static int setDimensions(lua_State* pL)
	{
				int n = lua_gettop(pL);
		if (n!=4)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL, 3)&&lua_isstring(pL, 4))
		{
			DisplayLuaCallback::getSingleton().setDimensions(StringConverter::parseReal(lua_tostring(pL, 1)),StringConverter::parseReal(lua_tostring(pL, 2)),StringConverter::parseReal(lua_tostring(pL, 3)),StringConverter::parseReal(lua_tostring(pL, 4)));
		}
		return 1;
	}
static int resetDimensions(lua_State* pL)
	{
			DisplayLuaCallback::getSingleton().resetDimensions();
			return 0;
	}

	static int s_activateElisa(lua_State* pL)
	{
			Display::getSingleton().activateElisa();
			return 0;
	}
	static int s_deactivateElisa(lua_State* pL)
	{
			Display::getSingleton().deactivateElisa();
			return 0;
	}

	static int s_activateKeyExit(lua_State* pL)
	{
			Display::getSingleton().activateEasyExit(true);
			return 0;
	}

	static int s_deactivateKeyExit(lua_State* pL)
	{
			Display::getSingleton().activateEasyExit(false);
			return 0;
	}

	static int turnOff(lua_State* pL)
	{
		LogManager::getSingleton().logMessage("Display lua: turn off");
			DisplayLuaCallback::getSingleton().shutdown();
			return 0;
	}
	static int writeLn(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
			DisplayLuaCallback::getSingleton().setWriteLnText(lua_tostring(pL, 1));//toWrite=
		global::getSingleton().getRoot()->renderOneFrame();
		return 1;
	}

	static int setInChar(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			DisplayLuaCallback::getSingleton().setFirstChar(tex);
		}
		return 1;
	}

	static int setMaxLines(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			DisplayLuaCallback::getSingleton().setLines(tex);
		}
		return 1;
	}

static int setTextColor(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=4)
		{
			return 0;
		}
		if (lua_isstring(pL, 1)&&lua_isstring(pL, 2)&&lua_isstring(pL,3)&&lua_isstring(pL, 4))
		{
			ColourValue c(StringConverter::parseReal(lua_tostring(pL,1)),StringConverter::parseReal(lua_tostring(pL,2)),StringConverter::parseReal(lua_tostring(pL,3)),StringConverter::parseReal(lua_tostring(pL,4)));
				Display::getSingleton().setColor(c);
		}
		return 1;
	}


	static int newLine(lua_State* pL)
	{
		DisplayLuaCallback::getSingleton().setNewLine();
		global::getSingleton().getRoot()->renderOneFrame();
		return 1;
	}

	static int setFontSize(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			DisplayLuaCallback::getSingleton().setFontSize(tex);
		}
		return 1;
	}

	static int setFont(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			DisplayLuaCallback::getSingleton().setFont(tex);
		}
		return 1;
	}

	static int clearScreen(lua_State* pL)
	{
		DisplayLuaCallback::getSingleton().setClear();
		return 1;
	}
	static int setTexture(lua_State* pL)
	{
		int n = lua_gettop(pL);
		if (n!=1)
		{
			return 0;
		}
		if (lua_isstring(pL, 1))
		{
			String tex = lua_tostring(pL, 1);
			DisplayLuaCallback::getSingleton().setDTex(tex);//toWrite=
		}
		return 1;
	}

	void activateEasyExit(bool ac)
	{
		mEasyExit=ac;
	}

	//int cpp_GetAIMove(lua_State* pL);
	//int cpp_writeLn(lua_State* pL);
	void* curcomputer;
	OverlayElement *textbox;
private:
	bool elisa_chat;
#ifdef ELISA
	Eliza eliza;
#endif
	String prevTex;
	bool initialized;
	bool mEasyExit;
	Ogre::Overlay* dispOverlay;
	lua_State* pLuaState;
	Ogre::String fontName;
    bool            visible;
    Root         *root;
	int maxLines;
	String mCommand;
    SceneManager   *scene;
    Rectangle2D   *rect;
    SceneNode      *noder;
	Ogre::TexturePtr lTextureWithRtt;
	Ogre::Camera * lRttCamera;
	Ogre::SceneNode* lRTTCameraNode;
   Ogre::Viewport* lRttViewport1;
    Overlay      *overlay;
	OverlayContainer* dispCont;
	MaterialPtr dispMat;
	Technique* dispTech;
	String prefix;
	Pass* dispPass;
	TextureUnitState* dispTUstate;
    float            height;
    bool            update_overlay;
	bool allowVirtualDisplay;
    int               start_line;
    list<String>      lines;
    String            prompt;
};

#endif