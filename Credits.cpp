/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Credits.h"

template<> Credits *Singleton<Credits>::ms_Singleton=0;

Credits::Credits()
{
	timeElapsed=0;
	started=false;
}

Credits::~Credits()
{
}


void Credits::init()
{
	lua_register(global::getSingleton().getLuaState(), "startCredits",startCredits);
}

void Credits::start(String fileName)
{
	cf.load(fileName);
	String overlayName = cf.getSetting("OverlayName");
	String datafile = cf.getSetting("FileName");
	speed = StringConverter::parseReal(cf.getSetting("Speed"))*global::getSingleton().getWindow()->getWidth()/480;
	cf.load(datafile);
	ConfigFile::SettingsMultiMap *settings = cf.getSectionIterator().getNext();
   ConfigFile::SettingsMultiMap::iterator b;
  // String curN;
   cred_data="";
   for (b = settings->begin(); b != settings->end(); ++b)
   {
              cred_data=cred_data+"\n"+b->second;
			  LogManager::getSingleton().logMessage(b->second);
   }
   started=true;
	credOv=OverlayManager::getSingleton().getByName(overlayName);
	credOv->show();

	creditTextItem=credOv->getChild("Run3/Credits01Panel")->getChild("Run3/CreditsCredits");
	creditTextItem->setCaption(cred_data);
	creditTextItem->setMetricsMode(GMM_PIXELS);
	creditTextItem->setTop(0);
	creditTextItem->setLeft(0);
	creditTextItem->setHeight(global::getSingleton().getWindow()->getHeight());
	creditTextItem->setWidth(global::getSingleton().getWindow()->getWidth());
	char_height=0.03*global::getSingleton().getWindow()->getHeight()*(settings->size()+1)+StringConverter::parseReal(cf.getSetting("Delta"));
	LogManager::getSingleton().logMessage(StringConverter::toString(char_height));

}	

void Credits::pause()
{

}

void Credits::stop()
{

}

void Credits::update(const Ogre::FrameEvent &evt)
{
	if (started)
	{
		timeElapsed+=evt.timeSinceLastFrame;
	
		//LogManager::getSingleton().logMessage(StringConverter::toString(creditTextItem->getTop()));
		creditTextItem->setTop(creditTextItem->getTop()-evt.timeSinceLastFrame*speed*TIME_SHIFT);
		if (abs(creditTextItem->getTop())>char_height)
			started=false;
	}
}

void Credits::cleanup()
{
	if (started)
	{
		creditTextItem->setTop(0);
		started=false;
	}
}