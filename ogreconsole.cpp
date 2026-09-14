/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "ogreconsole.h"
#include <OgreMaterialManager.h>

template <> OgreConsole *Singleton<OgreConsole>::msSingleton = nullptr;

#define CONSOLE_LINE_LENGTH 85
#define CONSOLE_LINE_COUNT 15
OgreConsole::OgreConsole() {
  initialized = false;
  lua_activated = false;
  start_line = 0;
}

OgreConsole::~OgreConsole() {}

void OgreConsole::init(Ogre::Root *root) {
  // if(!root->getSceneManagerIterator().hasMoreElements())
  //    OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "No scene manager found!",
  //    "init" );

  this->root = root;
  const Ogre::SceneManagerInstanceMap &sceneManagers = root->getSceneManagers();
  if (sceneManagers.empty()) {
    OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "No scene manager exists",
                "OgreConsole::init");
  }
  scene = sceneManagers.begin()->second;
  root->addFrameListener(this);

  height = 1;
  // Create background rectangle covering the whole screen
  rect = new Rectangle2D(true);
  rect->setCorners(-1, 1, 1, 1 - height);
  rect->setMaterial(MaterialManager::getSingleton().getByName(
      "console/background"));
  rect->setRenderQueueGroup(RENDER_QUEUE_OVERLAY);
  rect->setBoundingBox(AxisAlignedBox(-100000.0 * Vector3::UNIT_SCALE,
                                      100000.0 * Vector3::UNIT_SCALE));
  noder = scene->getRootSceneNode()->createChildSceneNode("#Console");
  noder->attachObject(rect);
  textbox = OverlayManager::getSingleton().createOverlayElement("TextArea",
                                                                "ConsoleText");
  textbox->setCaption("hello");
  textbox->setMetricsMode(GMM_RELATIVE);
  textbox->setPosition(0, 0);
  textbox->setParameter("font_name", "Console");
  textbox->setParameter("colour_top", "1 1 1");
  textbox->setParameter("colour_bottom", "1 1 1");
  textbox->setParameter("char_height", "0.03");
  overlay = OverlayManager::getSingleton().create("Console");
  overlay->add2D((OverlayContainer *)textbox);
  overlay->show();
  overlay->setZOrder(599);
  LogManager::getSingleton().getDefaultLog()->addListener(this);
  initialized = true;
}

void OgreConsole::shutdown() {
  if (!initialized)
    return;
  noder->detachAllObjects();
  overlay->remove2D((OverlayContainer *)textbox);
  overlay->hide();
  overlay->clear();
}
void OgreConsole::onKeyPressed(const run3::InputEvent &arg) {
  if (!visible)
    return;
  if (arg.key == run3::Key::Return) {
    bprompt = prompt;
    // split the parameter list
    Ogre::StringUtil::trim(prompt); // A useful addition
    const char *str = prompt.c_str();
    vector<String> params;
    String param = "";
    for (int c = 0; c < prompt.length(); c++) {
      if (str[c] == ' ') {
        if (param.length())
          params.push_back(param);
        param = "";
      } else
        param += str[c];
    }
    if (param.length())
      params.push_back(param);

    if (lua_activated) {
      if (!params.empty()) {
        // params.push_back(String(""));
        // try to execute the command
        if (luaCommandHandler) {
          luaCommandHandler(params);
        } else {
          LogManager::getSingleton().logMessage(
              "Console Lua command ignored: no script handler is installed",
              LML_WARNING);
        }
      }
    }
    // try to execute the command
    map<String, void (*)(vector<String> &)>::iterator i;
    if (prompt.length() > 0) {
      for (i = commands.begin(); i != commands.end(); i++) {
        if ((*i).first == params[0]) {
          if ((*i).second)
            (*i).second(params);
          break;
        }
      }
    }
    print(prompt);
    prompt = "";
  }
  if (arg.key == run3::Key::Backspace && !prompt.empty())
    prompt.pop_back();
  if (arg.key == run3::Key::PageUp) {
    if (start_line > 0)
      start_line--;
  }
  if (arg.key == run3::Key::PageDown) {
    if (start_line < lines.size())
      start_line++;
  }
  if (arg.key == run3::Key::Up) {
    prompt = bprompt;
  }
  update_overlay = true;
}

bool OgreConsole::onInputEvent(const run3::InputEvent &event) {
  if (!visible)
    return false;
  if (event.type == run3::InputEventType::KeyPressed) {
    onKeyPressed(event);
    return true;
  }
  if (event.type == run3::InputEventType::TextEntered) {
    prompt += event.text;
    update_overlay = true;
    return true;
  }
  return false;
}
bool OgreConsole::frameStarted(const Ogre::FrameEvent &evt) {
  if (visible && height < 1) {
    height += evt.timeSinceLastFrame * 2;
    textbox->show();
    if (height >= 1) {
      height = 1;
    }
  } else if (!visible && height > 0) {
    height -= evt.timeSinceLastFrame * 2;
    if (height <= 0) {
      height = 0;
      textbox->hide();
    }
  }

  textbox->setPosition(0, (height - 1) * 0.5);
  rect->setCorners(-1, 1 + height, 1, 1 - height);

  if (update_overlay) {
    String text;
    list<String>::iterator i, start, end;

    // make sure is in range
    if (start_line > lines.size())
      start_line = lines.size();

    int lcount = 0;
    start = lines.begin();
    for (int c = 0; c < start_line; c++)
      start++;
    end = start;
    for (int c = 0; c < CONSOLE_LINE_COUNT; c++) {
      if (end == lines.end())
        break;
      end++;
    }
    for (i = start; i != end; i++)
      text += (*i) + "\n";

    // add the prompt
    text += "] " + prompt;

    textbox->setCaption(text);
    update_overlay = false;
  }
  return true;
}

void OgreConsole::print(const String &text) {
  // subdivide it into lines
  const char *str = text.c_str();
  int start = 0, count = 0;
  int len = text.length();
  String line;
  for (int c = 0; c < len; c++) {
    if (str[c] == '\n' || line.length() >= CONSOLE_LINE_LENGTH) {
      lines.push_back(line);
      line = "";
    }
    if (str[c] != '\n')
      line += str[c];
  }
  if (line.length())
    lines.push_back(line);
  if (lines.size() > CONSOLE_LINE_COUNT)
    start_line = lines.size() - CONSOLE_LINE_COUNT;
  else
    start_line = 0;
  update_overlay = true;
}

bool OgreConsole::frameEnded(const Ogre::FrameEvent &evt) { return true; }

void OgreConsole::setVisible(bool visible) { this->visible = visible; }

void OgreConsole::addCommand(const String &command,
                             void (*func)(vector<String> &)) {
  commands[command] = func;
}

void OgreConsole::removeCommand(const String &command) {
  commands.erase(commands.find(command));
}
