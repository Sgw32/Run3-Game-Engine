#include "Display.h"
#include "Computer.h"
#include "Loader.h"

template<> Display *Ogre::Singleton<Display>::ms_Singleton=0;

#define DISPLAY_LINE_LENGTH 85
#define DISPLAY_LINE_COUNT 9

Display::Display()
{
	 initialized=false;
	 prefix="] ";
	curcomputer=0;
	allowVirtualDisplay=true;
	lRttViewport1=0;
	elisa_chat=false;
}

Display::~Display()
{

}
void Display::init(bool shows,lua_State* pL)
{
		if(shows)
			show();
		//LUA
		maxLines=DISPLAY_LINE_COUNT;
		pLuaState=pL;
		lua_register(pL, "dCompPrint", writeLn);
		lua_register(pL, "dMaterialSet", setTexture);
		lua_register(pL, "dClear", clearScreen);
		lua_register(pL, "dPrefix", setInChar);
		lua_register(pL, "dSetFont", setFont);
		lua_register(pL, "dSetFontSize", setFontSize);
		lua_register(pL, "dNewLine", newLine);
		lua_register(pL, "dSetMaxLines", setMaxLines);
		lua_register(pL, "dSetDimensions", setDimensions);
		lua_register(pL, "dResetDimensions", resetDimensions);
		lua_register(pL, "dActivateElisa", s_activateElisa);
		lua_register(pL, "dDeactivateElisa", s_deactivateElisa);
		lua_register(pL, "dActivateKeyExit", s_activateKeyExit);
		lua_register(pL, "dDeactivateKeyExit", s_deactivateKeyExit);
		lua_register(pL, "shutdown", turnOff);
		lua_register(pL, "logoff", turnOff);
		lua_register(pL, "exit", turnOff);
		lua_register(pL, "return", turnOff);
		lua_register(pL, "quit", turnOff);
		lua_register(pL, "dSetTextColor", turnOff);
		RunLuaScript(pL, "run3/lua/display.lua");
		lua_settop(pL, 0);
		lua_getglobal(pL, "fontName");
		lua_getglobal(pL, "overlayName");
		Ogre::String fontName = lua_tostring(pL, 1);
		Ogre::String overlayName = lua_tostring(pL, 2);
		//LUA
		//textbox->set
		//FrameListener
		root = global::getSingleton().getRoot();
		scene=root->getSceneManagerIterator().getNext();
		root->addFrameListener(this);


		/*//Text box
		textbox=OverlayManager::getSingleton().createOverlayElement("TextArea","DisplayTXT");
		textbox->setCaption("hello");
		textbox->setMetricsMode(GMM_RELATIVE);
		textbox->setPosition(0,0);
		textbox->setParameter("font_name",fontName);
		textbox->setParameter("colour_top","1 1 1");
		textbox->setParameter("colour_bottom","1 1 1");
		textbox->setParameter("char_height","0.03");
		//Overlay
		overlay=OverlayManager::getSingleton().create("Display"); 
		dispCont=(OverlayContainer*)textbox;
		dispCont->addChild(textbox);
		overlay->add2D(dispCont);
		overlay->show();*/
		visible=shows;
		overlay = OverlayManager::getSingleton().getByName("Run3/Display");
		dispCont=overlay->getChild("Run3/DisplayP");
		textbox=dispCont->getChild("Display/Text");
		textbox->setCaption("hello");
		dispMat=dispCont->getMaterial();
		dispTech = dispMat->getTechnique(0);
		dispPass = dispTech->getPass(0);
		dispTUstate = dispPass->getTextureUnitState(0);
		
		//String txt = "hasflahsilfhasfklahsfklahsfklahsflaksdfha";
		//writeLnD(txt );
		/*textbox->setMetricsMode(GMM_RELATIVE);
		textbox->setPosition(0,0);
		textbox->setParameter("font_name",fontName);
		textbox->setParameter("colour_top","1 1 1");
		textbox->setParameter("colour_bottom","1 1 1");
		textbox->setParameter("char_height","0.03");*/
		
		//creating computer Render to Texture
		
/*Ogre::String lMaterialName = "MyRttMaterial";
	{
			Ogre::MaterialManager& lMaterialManager = Ogre::MaterialManager::getSingleton();
		Ogre::MaterialPtr lMaterial = lMaterialManager.create(lMaterialName,"General");
		Ogre::Technique * lTechnique = lMaterial->getTechnique(0);
		Ogre::Pass* lPass = lTechnique->getPass(0);
		Ogre::TextureUnitState* lTextureUnit = lPass->createTextureUnitState();
		lTextureUnit->setTextureName(lTextureName);
				lTextureUnit->setNumMipmaps(0);
		lTextureUnit->setTextureFiltering(Ogre::TFO_BILINEAR);
	}*/
		
		initialized=true;
}


void Display::reset()
{
		
		//RunLuaScript(pLuaState, "run3/lua/resetcomp.lua");
	LogManager::getSingleton().logMessage("Display reset requested!");
	if (curcomputer)
	{
		Computer* comp = (Computer*)curcomputer;
		LogManager::getSingleton().logMessage("LOGGING OFF");
		if (comp)
		{
		comp->logoff();
		}
		curcomputer=0;
	}
}


void Display::setMaterial(String name)
{
dispCont->setMaterialName(name);
}

void Display::shutdown(){
   if(!initialized)
      return;
   overlay->remove2D(dispCont);
   overlay->hide();
   overlay->clear();
}

void Display::onKeyPressed(const OIS::KeyEvent &arg){
   if(!visible)
      return;
	
	if (mEasyExit)
	{
		if ((arg.key == OIS::KC_E)||(arg.key == OIS::KC_ESCAPE))
		{
			if (isVisible())
			{
						reset();
			}
		}
	}

   if (arg.key == OIS::KC_RETURN)
   {
      //split the parameter list
	   if (!elisa_chat)
	   {
		   Ogre::StringUtil::trim(prompt); // A useful addition 
		   if (prompt.empty())
			   return;
			const char *str=prompt.c_str();
			vector<String> params;
			String param="";
			for(int c=0;c<prompt.length();c++){
				if(str[c]==' '){
					if(param.length())
					params.push_back(param);
					param="";
				}
				else
					param+=str[c];
			}
			if(param.length())
				params.push_back(param);
			if (prompt.length()>0)
			{
				mCommand=params[0];
			}
			if (!params.empty())
			{
				//params.push_back(String(""));
			//try to execute the command
				processLuaFunction(params);
			}
			writeLnD(prompt);
	   }
	   else
	   {
		   writeLnD(prompt);
#ifdef ELISA
		   eliza.get_input(prompt);
		   eliza.respond();
			if (eliza.quit())
			{
				deactivateElisa();
			}
#endif
	   }
		

      
      prompt="";
   }
   if (arg.key == OIS::KC_BACK)
      prompt=prompt.substr(0,prompt.length()-1);
   if (arg.key == OIS::KC_PGUP)
   {
      if(start_line>0)
         start_line--;
   }

  /* if ((arg.key == OIS::KC_LSHIFT)||
	   (arg.key == OIS::KC_RSHIFT)||
	   (arg.key == OIS::KC_LSHIFT)||*/

   if (arg.key == OIS::KC_PGDOWN)
   {
      if(start_line<lines.size())
         start_line++;
   }
   else
   {
	   //OIS::setTextTranslation(
	   
      char legalchars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+!\"#%&/()=?[]\\*-_.:,; ";
      for(int c=0;c<sizeof(legalchars);c++){
         if(legalchars[c]==arg.text){
			 if (arg.text!=0)
			 {
				prompt+=arg.text;
			 
            break;
			}
         }
      }
   }
   update_overlay=true;
}

void Display::processLuaFunction(vector<String> params)
{

   lua_getglobal(pLuaState, params[0].c_str());

   //check that it is there
   if (lua_isfunction(pLuaState, -1))
   {
     
   

   /*//push some variables onto the lua stack
   lua_pushnumber(pL, 5);
   lua_pushnumber(pL, 8);*/
	if (params.size()>0)
	{
	int i;
	for (i=1;i!=params.size();i++)
	{
		lua_pushstring(pLuaState,params[i].c_str());
	}
	}
   //calling the function with parameters to set the number of parameters in
   //the lua func and how many return values it returns. Puts the result at
   //the top of the stack
   lua_call(pLuaState, params.size()-1, 1);

   //grab the result from the top of the stack
   if (lua_isnumber(pLuaState,-1))
   {
		int result = lua_tonumber(pLuaState, -1);

		//lua_pop(pLuaState, 1);
		writeLnD(StringConverter::toString(result));
		//writeLnD(String("FASD"));
		return;
   }
   if (lua_isstring(pLuaState,-1))
   {
		String res = lua_tostring(pLuaState,-1);
		//lua_pop(pLuaState,1);
		writeLnD(res);
		return;
   }
   }
}




bool Display::frameStarted(const Ogre::FrameEvent &evt){
	// solve LUA functions!
	//LogManager::getSingleton().logMessage("bums1");
			if ((Loader::getSingleton().rtt_display)&&(lRttViewport1))
	lRttViewport1->update();

		//	LogManager::getSingleton().logMessage("bums2");
	if (visible)
	{
	String txt = DisplayLuaCallback::getSingleton().getWriteLnText();
	if (!(txt==""))
	{
		writeLnD(txt);
	}

	String matt = DisplayLuaCallback::getSingleton().getDTex();

	if (!matt.empty())
	{
		setMaterial(matt);
	}

	txt = DisplayLuaCallback::getSingleton().getFirstChar();
	if (!txt.empty())
	{
		prefix=txt;
	}
	
	int maxLine = DisplayLuaCallback::getSingleton().getLines();
	if (maxLine!=-1)
	{
		maxLines=maxLine;
	}

	bool clr = DisplayLuaCallback::getSingleton().getClear();
	if (clr)
	{
		clrScr();
	}

	clr = DisplayLuaCallback::getSingleton().getNewLine();
	if (clr)
	{
		writeLnD(String("\n"));
	}

	txt = DisplayLuaCallback::getSingleton().getFont();
	if (!txt.empty())
	{
		textbox->setParameter("font_name",txt);
	}

	txt = DisplayLuaCallback::getSingleton().getFontSize();
	if (!txt.empty())
	{
		textbox->setParameter("char_height",txt);
	}
	// END

   if(update_overlay){
      String text;
      list<String>::iterator i,start,end;
      
      //make sure is in range
      if(start_line>lines.size())
         start_line=lines.size();

      int lcount=0;
      start=lines.begin();
      for(int c=0;c<start_line;c++)
         start++;
      end=start;
      for(int c=0;c<maxLines;c++){
         if(end==lines.end())
            break;
         end++;
      }
      for(i=start;i!=end;i++)
         text+=(*i)+"\n";
      
      //add the prompt
      text+=prefix+prompt;

      textbox->setCaption(text);
      update_overlay=false;
   }
	}
   return true;
}

bool Display::frameEnded(const Ogre::FrameEvent &evt){

   return true;
}

void Display::show()
{
	visible=true;
	mEasyExit=false;
	overlay->show();

if ((Loader::getSingleton().rtt_display)&&(allowVirtualDisplay))
		{
			global::getSingleton().getCamera()->getViewport()->setOverlaysEnabled(false);
		Ogre::TextureManager& lTextureManager = Ogre::TextureManager::getSingleton();
	Ogre::String lTextureName = "DisplayRtt";
	bool lGammaCorrection = false;
	unsigned int lAntiAliasing = 0;
	unsigned int lNumMipmaps = 0;
	lTextureWithRtt = lTextureManager.createManual(lTextureName,"General", 
		Ogre::TEX_TYPE_2D, 512, 512, lNumMipmaps,
		Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, lGammaCorrection, lAntiAliasing);

	Ogre::RenderTexture* lRenderTarget = NULL;
	{
		Ogre::HardwarePixelBufferSharedPtr lRttBuffer = lTextureWithRtt->getBuffer();
		lRenderTarget = lRttBuffer->getRenderTarget();
		lRenderTarget->setAutoUpdated(true);
			lRttCamera = scene->createCamera("RttCamera");
		lRttCamera->setNearClipDistance(1.5f);
		lRttCamera->setFarClipDistance(3000.0f); 
		lRttCamera->setAspectRatio(1.0f);
		lRTTCameraNode =scene->getRootSceneNode()->createChildSceneNode();
		lRTTCameraNode->attachObject(lRttCamera);
		lRTTCameraNode->setPosition( 7, 7, -8);
		Ogre::Vector3 lTargetPointToLookAt( 0, 0, -10.0 );
		lRTTCameraNode->lookAt( lTargetPointToLookAt, Ogre::Node::TS_WORLD);
		lRttViewport1 = lRenderTarget->addViewport(lRttCamera, 50, 0.00f, 0.00f, 1.0f, 1.0f);
		//lRttViewport1->setAutoUpdated(true);
		Ogre::ColourValue lBgColor1(0.0,0.0,0.0,1.0);
		lRttViewport1->setBackgroundColour(lBgColor1);
	}
		
				Ogre::MaterialManager& lMaterialManager = Ogre::MaterialManager::getSingleton();
		Ogre::MaterialPtr lMaterial = lMaterialManager.getByName(((Computer*)curcomputer)->mDispMatName);
		Ogre::Technique * lTechnique = lMaterial->getTechnique(0);
		Ogre::Pass* lPass = lTechnique->getPass(0);
		Ogre::TextureUnitState* lTextureUnit = lPass->getTextureUnitState(0);
		prevTex=lTextureUnit->getTextureName();
		lTextureUnit->setTextureName("DisplayRtt");
				lTextureUnit->setNumMipmaps(0);
		lTextureUnit->setTextureFiltering(Ogre::TFO_BILINEAR);
		}

	//((Entity*)((Computer*)curcomputer)->getComp()->getAttachedObject(0))->setMaterialName("MyRttMaterial");
}
void Display::hide()
{
	visible=false;
if ((Loader::getSingleton().rtt_display)&&(allowVirtualDisplay))
		{
	Ogre::MaterialManager& lMaterialManager = Ogre::MaterialManager::getSingleton();
		Ogre::MaterialPtr lMaterial = lMaterialManager.getByName(((Computer*)curcomputer)->mDispMatName);
		Ogre::Technique * lTechnique = lMaterial->getTechnique(0);
		Ogre::Pass* lPass = lTechnique->getPass(0);
		Ogre::TextureUnitState* lTextureUnit = lPass->getTextureUnitState(0);
		lTextureUnit->setTextureName(prevTex);
	global::getSingleton().getCamera()->getViewport()->setOverlaysEnabled(true);
	TextureManager::getSingleton().remove( lTextureWithRtt->getName());
	lRTTCameraNode->detachAllObjects();
	scene->destroyCamera(lRttCamera);
	lRTTCameraNode=0;
	lRttCamera=0;
	lRttViewport1=0;
	
		}
	overlay->hide();
}
void Display::setText()
{

}

void Display::runScript(String luas)
{
	RunLuaScript(pLuaState, luas.c_str());
}

void Display::writeLnD(String& text)
{
 const char *str=text.c_str();
   int start=0,count=0;
   int len=text.length();
   String line;
   for(int c=0;c<len;c++){
      if(str[c]=='\n'||line.length()>=DISPLAY_LINE_LENGTH){
         lines.push_back(line);
         line="";
      }
      if(str[c]!='\n')
         line+=str[c];
   }
   if(line.length())
      lines.push_back(line);
   if(lines.size()>maxLines)
      start_line=lines.size()-maxLines;
   else
      start_line=0;
   update_overlay=true;
}

void Display::appendText()
{

}

