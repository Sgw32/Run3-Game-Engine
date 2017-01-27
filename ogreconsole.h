#pragma once
#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>
#include <list>
#include <vector>

using namespace Ogre;
using namespace std;

class OgreConsole: public Singleton<OgreConsole>, FrameListener, LogListener
{
public:
   OgreConsole();
   ~OgreConsole();

   void   init(Ogre::Root *root);
   void   shutdown();

   void   setVisible(bool visible);
   bool   isVisible(){return visible;}
   void   print(const String &text);

   virtual bool frameStarted(const Ogre::FrameEvent &evt);
   virtual bool frameEnded(const Ogre::FrameEvent &evt);

   void onKeyPressed(const OIS::KeyEvent &arg);

   void addCommand(const String &command, void (*)(vector<String>&));
   void removeCommand(const String &command);
	
   void activateLua(bool act)
   {
	   lua_activated=act;
   }
   //log
   void messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName ) {print(logName+": "+message);}
private:
   bool            visible;
   bool            initialized;
   bool lua_activated;

   Root         *root;
   SceneManager   *scene;
   Rectangle2D   *rect;
   SceneNode      *noder;
   OverlayElement *textbox;
   Overlay      *overlay;

   float            height;
   bool            update_overlay;
   int               start_line;
   list<String>      lines;
   String            prompt;
   String bprompt;
   map<String,void (*)(vector<String>&)>  commands;
};