#pragma once
#include <run3/input/Input.hpp>
#include <Ogre.h>
#include <OgreFrameListener.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayManager.h>
#include <OgreRectangle2D.h>
#include <functional>
#include <list>
#include <utility>
#include <vector>

using namespace Ogre;
using namespace std;

class OgreConsole : public Singleton<OgreConsole>, FrameListener, LogListener,
                    public run3::IInputListener {
public:
  OgreConsole();
  ~OgreConsole();

  void init(Ogre::Root *root);
  void shutdown();

  void setVisible(bool visible);
  bool isVisible() { return visible; }
  void print(const String &text);

  virtual bool frameStarted(const Ogre::FrameEvent &evt);
  virtual bool frameEnded(const Ogre::FrameEvent &evt);

  void onKeyPressed(const run3::InputEvent &arg);
  bool onInputEvent(const run3::InputEvent &event) override;

  void addCommand(const String &command, void (*)(vector<String> &));
  void removeCommand(const String &command);

  void activateLua(bool act) { lua_activated = act; }
  void setLuaCommandHandler(
      std::function<void(vector<String> &)> handler) {
    luaCommandHandler = std::move(handler);
  }
  // log
  void messageLogged(const String &message, LogMessageLevel lml, bool maskDebug,
                     const String &logName) {
    print(logName + ": " + message);
  }

private:
  bool visible;
  bool initialized;
  bool lua_activated;

  Root *root;
  SceneManager *scene;
  Rectangle2D *rect;
  SceneNode *noder;
  OverlayElement *textbox;
  Overlay *overlay;

  float height;
  bool update_overlay;
  int start_line;
  list<String> lines;
  String prompt;
  String bprompt;
  map<String, void (*)(vector<String> &)> commands;
  std::function<void(vector<String> &)> luaCommandHandler;
};
