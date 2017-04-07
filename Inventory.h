#pragma once
#include <Ogre.h>

/*#include "LuaHelperFunctions.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>*/

#include "buttonGUI.h"
#include "Manager_Template.h"

using namespace Ogre;
using namespace Ogre;

class Inventory : public Singleton<Inventory>, public managerTemplate
{
public:
	Inventory();
	~Inventory();
	virtual void init();
	void init(buttonGUI::buttonManager* bMgr){buttonMgr=bMgr;};
	void setVisible(bool vis);
	void display();
	void destroy();
	void enableInventory();
	void disableInventory();
	void handleButtonEvent(buttonGUI::buttonEvent * e);
	bool isVisible(){return visi;}
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
	inline void setGlowWasActive(bool glow)
	{
	glowWasEnabled = glow;
	}
private:
	bool enabled;
	bool glowWasEnabled;
	bool visi;
	//lua_State* pLua;
	buttonGUI::buttonManager* buttonMgr;
};