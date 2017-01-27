#include "DisplayLuaCallback.h"
#include "Display.h"

template<> DisplayLuaCallback *Ogre::Singleton<DisplayLuaCallback>::ms_Singleton=0;

DisplayLuaCallback::DisplayLuaCallback()
{
	fChar="";
	wrtLn = "";
	dtex = "";
	dfont="";
	lines=-1;
	newLine = false;
	fontSize="";
	doClear=false;
}

DisplayLuaCallback::~DisplayLuaCallback()
{

}

void DisplayLuaCallback::setDimensions(Real x1,Real y1,Real x2,Real y2)
{
	Display::getSingleton().textbox->setTop(x1);
	Display::getSingleton().textbox->setLeft(y1);
	Display::getSingleton().textbox->setHeight(y2);
	Display::getSingleton().textbox->setWidth(x2);
	//Display::getSingleton().textbox->setDimensions(x2,y2);
}
void DisplayLuaCallback::shutdown()
{
if (Display::getSingleton().isVisible())
					{
						//Display::getSingleton().hide();
						Display::getSingleton().reset();
					}
}


void DisplayLuaCallback::resetDimensions()
{
	Display::getSingleton().textbox->setTop(0);
	Display::getSingleton().textbox->setLeft(0);
	Display::getSingleton().textbox->setDimensions(1,1);
}