/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
///////////////Run3 main event system,based on ExampleApplication:)//
///////////////BUGS:                                               //
///////////////On exit by pressing "exit" button, showing exception:(// 
/////////////////////////////////////////////////////////////////////
extern "C"
{
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}

#include "MainModules.h" //Все глобальные там.
#include "MainUtils.h" //Функции

class MainListener : public Run3FrameListener, public OIS::MouseListener, public OIS::KeyListener
{
public:
    MainListener(RenderWindow* win, Camera* cam,SceneManager *sceneMgr)
        : Run3FrameListener(win, cam, true, true)
    {
        global::getSingleton().GUIorGame=true;
		mShutdownRequested=false;
		g_ShutdownRequested=false;
		first=true;
		mContinue=true;
	    mConsole=false;   
		ingame=false;
		noclip=false;
        mSceneMgr = sceneMgr;
		MapSm = sceneMgr;
        mRotate = 0.13;
        mMove = 250;
	    mMouse->setEventCallback(this);
        mKeyboard->setEventCallback(this);
		mKeyboard->setTextTranslation(OIS::Keyboard::TextTranslationMode::Ascii);
		global::getSingleton().setOISKeyboard(mKeyboard);
	    mMenuOverlay = OverlayManager::getSingleton().getByName(overlay);
	   // mMenuOverlay->setZOrder(100);
	    mMenuOverlay->show();
	    g_MenuOverlayUnsafe = mMenuOverlay;
		mMouse->setEventCallback(this);
        mKeyboard->setEventCallback(this);
		mDirection = Vector3::ZERO;
	    OgreConsole::getSingleton().addCommand("quit" ,Quit);
	    OgreConsole::getSingleton().addCommand("hideoverlay" ,hOverlay);
		OgreConsole::getSingleton().addCommand("mouse" ,ToggleMouse);
		OgreConsole::getSingleton().addCommand("map" ,Map);
		OgreConsole::getSingleton().addCommand("clear" ,Clear);
		//OgreConsole::getSingleton().addCommand("noclip" ,Noclip);
		OgreConsole::getSingleton().addCommand("phys" ,Phys);
		Run3Input::getSingleton().init(mMenuOverlay,sheet);
		if (backMap!="no_map_run3")
		{
			if ((backMap!="multiple")||((backMap=="multiple")&&(backMaps.size()>0)))
			{
			LogManager::getSingleton().logMessage("Run3: Now loading background map...");
			loadBackgroundMap();
			}
		}
		//soundMgr->playAudio( audioId, true );
    } 
    
	static void Quit(vector<String>& param)
	{
		g_ShutdownRequested=true;
	}
	static void Clear(vector<String>& param)
	{
	   if (!first)
	   {
		   LoadMap::getSingleton().UnloadM(MapSm);
	   }
	}
    static void  ToggleMouse(vector<String>& param)
	{ 
		if (param[1] == "0")
		{
			if (CEGUI::MouseCursor::getSingleton().isVisible())
			{
			    CEGUI::MouseCursor::getSingleton().hide();
			}
		}
		else
		{
			g_MouseVisible = CEGUI::MouseCursor::getSingleton().isVisible();
			if (!g_MouseVisible)
			{
			   CEGUI::MouseCursor::getSingleton().show();
			}
		}
	}

	static void Map(vector<String>& param)
	{ 
	   global::getSingleton().GUIorGame=false;
       g_MenuOverlayUnsafe->hide();
       sheet->hide(); 
	   CEGUI::MouseCursor::getSingleton().hide();
	   if (!first)
	   {
	   LoadMap::getSingleton().UnloadM(MapSm);
	   }
	   HUD::getSingleton().Show();
	   LoadMap::getSingleton().LoadM(param[1],"Ogre Testmap","/scripts","run3/maps","General",MapSm);
	   first=false;
	   ingame=true;
	   player->noclip=false;
	}
	static void Phys(vector<String>& param)
	{
		PhysObject* phys = new PhysObject;
		try
		{
			phys->init(MapSm,mWorld);
			phys->CreateObject(param[1],param[2],MapSm->getSceneNode("rootn"),100.0,false,"");
		}
		catch(Ogre::Exception &/*e*/)
		{
			LogManager::getSingleton().logMessage("[Console] Error loading a physics object!");
		}
	}
	static void hOverlay(vector<String>& param)
	{ 
		g_MenuOverlayUnsafe->hide();
	}
	void hideOverlay(void)
	{
		mMenuOverlay->hide();
	}
	void requestShutdown(void)
    {
        mShutdownRequested = true;
    }

    bool frameStarted(const FrameEvent &evt)
    {
		if (global::getSingleton().restData[17])
			return mContinue;
		//LogManager::getSingleton().logMessage("g1");
		global::getSingleton().getRoot()->getRenderSystem()->clearFrameBuffer(Ogre::FBT_COLOUR | Ogre::FBT_DEPTH);
		//LogManager::getSingleton().logMessage("g2");
		//global::getSingleton().getWindow()->swapBuffers();
		Ogre::WindowEventUtilities::messagePump();
		//LogManager::getSingleton().logMessage("g3");
		if (!(global::getSingleton().restData[0]))
			buttonGUI::InputManager2::getSingletonPtr()->capture();
		//LogManager::getSingleton().logMessage("g4");
        mKeyboard->capture();
		//LogManager::getSingleton().logMessage("g5");
        mMouse->capture();
		//LogManager::getSingleton().logMessage("g6");
		if (!(global::getSingleton().restData[1]))
			player->FCUpdate(evt);
		//LogManager::getSingleton().logMessage("g7");
		time = evt.timeSinceLastFrame;
		//LogManager::getSingleton().logMessage("g8");
		bool change = global::getSingleton().changemap_now;
		//LogManager::getSingleton().logMessage("g9");
		if (!(global::getSingleton().restData[2]))
			Modulator::getSingleton().frameStarted(evt);
		//LogManager::getSingleton().logMessage("g10");
		if (change)
		{
			//LogManager::getSingleton().logMessage("g10f");
				   global::getSingleton().GUIorGame=false;
					g_MenuOverlayUnsafe->hide();
					sheet->hide(); 
					CEGUI::MouseCursor::getSingleton().hide();
					if (!first)
					{
					LoadMap::getSingleton().UnloadM(MapSm);
					}
					first=false;
					ingame=true;
					player->noclip=false;
					HUD::getSingleton().Show();
					LoadMap::getSingleton().LoadM(global::getSingleton().mMap,"Ogre Testmap","/scripts","run3/maps","General",MapSm);
					return true;
		}
		//LogManager::getSingleton().logMessage("g11");
		global::getSingleton().setNULLChange();
		if (!(global::getSingleton().restData[3]))
			mChCtrl.step(evt.timeSinceLastFrame);
		//LogManager::getSingleton().logMessage("g12");
		if (!(global::getSingleton().restData[4]))
			MusicPlayer::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g13");
		if (!(global::getSingleton().restData[5]))
			WaterManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g14");
		if (!(global::getSingleton().restData[6]))
			SkyManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g15");
		if (!(global::getSingleton().restData[7]))
			BloodEmitter::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g16");
		if (!(global::getSingleton().restData[8]))
			Credits::getSingleton().update(evt);
		//LogManager::getSingleton().logMessage("g17");
		if (!(global::getSingleton().restData[9]))
			Run3SoundRuntime::getSingleton().frameStarted(evt);
		//LogManager::getSingleton().logMessage("g18");
			NPCManager::getSingleton().frameStarted(evt);
		//LogManager::getSingleton().logMessage("g19");
		if (!(global::getSingleton().restData[10]))
			ExplosionManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g20");
		if (!(global::getSingleton().restData[11]))
			CrosshairOperator::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g21");
		if (!(global::getSingleton().restData[12]))
			GibManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g22");
		if (!(global::getSingleton().restData[13]))
			LightPerfomanceManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g23");
		if (!(global::getSingleton().restData[14]))
			BulletHitManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g24");
		if (!(global::getSingleton().restData[15]))
			MagicManager::getSingleton().upd(evt);
		//LogManager::getSingleton().logMessage("g25");
		if (!(global::getSingleton().restData[16]))
		{
			for (unsigned int i=0;i!=managers.size();i++)
				managers[i]->upd(evt);
		}
		Run3Benchmark::getSingleton().frameStarted(evt);
		//LogManager::getSingleton().logMessage("g26");
		return mContinue;
    }

//background map loading
    void loadBackgroundMap(void)
	{
		LogManager::getSingleton().logMessage("GUIorGame set to false");
		
	   LogManager::getSingleton().logMessage("..finished!");
	   if (!first)
	   {
		   LogManager::getSingleton().logMessage("Now unloading map(realy?)");
	   LoadMap::getSingleton().UnloadM(MapSm);
	   }
	   
	   LogManager::getSingleton().logMessage("Ready to load the map!");

		String mapToLoad = backMap;
		
		if (mapToLoad=="multiple")
		{
			mapToLoad = backMaps[rand()%backMaps_number];
		}

	   LoadMap::getSingleton().LoadM(mapToLoad,"Ogre Testmap","/scripts","run3/maps","General",mSceneMgr);
	   LogManager::getSingleton().logMessage("map is loaded!");
	   // soundMgr->playAudio( audioId, true );
	  first=false;
	}

    bool frameEnded(const FrameEvent& evt)
    {
        if (mShutdownRequested || g_ShutdownRequested)
		{
         return false;
		}
		else
		{
     			return Run3FrameListener::frameEnded(evt);
		}
	}
    bool quit(const CEGUI::EventArgs &e)
    {
        mContinue = false;
        return true;
    }

    // MouseListener
    bool mouseMoved(const OIS::MouseEvent &arg)
    {
		if (global::getSingleton().GUIorGame)
		{
        CEGUI::System::getSingleton().injectMouseMove(arg.state.X.rel, arg.state.Y.rel);
		}

		if (!global::getSingleton().GUIorGame && player->noclip)
		{
			
   
		}
		if (!global::getSingleton().GUIorGame)
		{
		player->MouseMove(arg,time);
		}
        return true;
    }

    bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
    {
		if (global::getSingleton().GUIorGame)
		{
        CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id));
		}
		if (!global::getSingleton().GUIorGame)
		{
				player->MousePress(arg,id);
		}
        return true;
    }

    bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
    {
		if (global::getSingleton().GUIorGame)
		{        
		CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id));
		}
		if (!global::getSingleton().GUIorGame)
		{
				player->MouseRelease(arg,id);
		}
        return true;
    }

    // KeyListener
    bool keyPressed(const OIS::KeyEvent &arg)
    {
		
		OgreConsole::getSingleton().onKeyPressed(arg);
		


		Run3Input::getSingleton().processPress(arg,global::getSingleton().GUIorGame,ingame);
		Display::getSingleton().onKeyPressed(arg);
		if (arg.key == OIS::KC_PERIOD)
		{
				OgreConsole::getSingleton().setVisible(mConsole);
				mConsole=!mConsole;		
			    return true;
		}
        return true;
    }

    bool keyReleased(const OIS::KeyEvent &arg)
    {
		//ingame noclip
		Run3Input::getSingleton().processRelease(arg,global::getSingleton().GUIorGame,ingame);
		//ingame
		if (!global::getSingleton().GUIorGame && !OgreConsole::getSingleton().isVisible())
		{
		player->FCRelease(arg);
		}
		//ingame phys-x
		if (!global::getSingleton().GUIorGame && !(player->noclip))
		{
			
		}
        return true;
    }

private:
    Real mRotate;          // Константа вращения
    Real mMove;            // Констана перемещения
    bool mContinue;        // Продолжать рендеринг или нет
	bool mConsole;         // Открыта ли консоль
	Real time;				// Время с предыдущего фрэйма(не используется)
    Vector3 mDirection;     // Значения перемещения в правильном направлении
	SceneManager *mSceneMgr;   // Текущий SceneManager
    SceneNode *mCamNode;   // SceneNode, к которому присоединена текущая Камера
	Overlay* mMenuOverlay;
	bool mShutdownRequested;
};


class Run3App : public Run3Application
{
public:
    Run3App()
        : mGUISystem(0), mGUIRenderer(0), mEditorGuiSheet(0)
    {
		new OgreConsole();
		new LoadMap;
		new SharedLuaCallback;
		mWorld = new OgreNewt::World();
		new HUD();
		new Loader;
		new SceneLoadOverlay;
		new global;
		new Sequence;
		new SuperFX;
		new Run3Input;
		new POs;
		new Energy(mWorld);
		new Display;
		new DisplayLuaCallback;
		new SequenceLuaCallback;
		new AIManager;
		new NPCManager;
		new Run3Shadowing;
		new Run3SoundRuntime;
		new MirrorManager;
		new Modulator;
		new MusicPlayer;
		new WaterManager;
		new Credits;
		new BloodEmitter;
		new ExplosionManager;
		new CrosshairOperator;
		new MagicManager;
		new GibManager;
		new LightPerfomanceManager;
		new BulletHitManager;
		new MeshDecalMgr;
		new Generator;
		new Run3Batcher;
		new ZonePortalManager;
		new Timeshift;
		new SkyManager;
		new SaveGame;
		new SoftwareOcclusionCulling;
		new FacialAnimationManager;
		new Run3Benchmark;
	}

    ~Run3App() 
    {
        if(mEditorGuiSheet)
        {
            CEGUI::WindowManager::getSingleton().destroyWindow(mEditorGuiSheet);
        }
        if (mGUISystem)
            delete mGUISystem;

        if (mGUIRenderer)
            delete mGUIRenderer;
        if (soundMgr)
		{
			soundMgr->resumeAllAudio();
			soundMgr->stopAudio( audioId );
            soundMgr->releaseAudio( audioId );

            delete soundMgr;
		}
		OgreConsole::getSingleton().shutdown();
		if (mWorld)
		{
		delete mWorld;
		}
		player->shutdown();
		HUD::getSingleton().shutdown();
		
    }
protected:
   CEGUI::OgreCEGUIRenderer* mGUIRenderer;
   CEGUI::System* mGUISystem;
   CEGUI::Window* mMainMenu;
   Run3LoadingBar mLoadingBar;
   Run3LoadingBar mLoadingGame;
   
   CEGUI::Window* mEditorGuiSheet;
   CEGUI::String skinname;
   Ogre::String soundpath;
   Ogre::String sound;
   Ogre::String click;
   Ogre::String move;
   CEGUI::String newgame;
   CEGUI::String newgame2;
   CEGUI::String newgame3;
   CEGUI::String options_t;
   CEGUI::String quit;
   SceneNode* nodeC;
   ConfigFile cfg;
   OgreNewt::World* mWorld;
   OgreNewt::BasicFrameListener* mOgreNewtListener;
   int AIType;
   
   unsigned int audioId2;
   unsigned int audioId3;
   unsigned int audioId4;
   unsigned int audioId5;
   unsigned int audioId6;
   unsigned int audioId7;
   unsigned int audioId8;
   
   Real physUpdate;

	void loadResources(void)
	{

		mLoadingBar.start(mWindow, 3, 3, 1);

		// Turn off rendering of everything except overlays
		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->addSpecialCaseRenderQueue(RENDER_QUEUE_OVERLAY);
		mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_INCLUDE);

		// Set up the world geometry link

		// Initialise the rest of the resource groups, parse scripts etc
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		ResourceGroupManager::getSingleton().loadResourceGroup(
			ResourceGroupManager::getSingleton().getWorldResourceGroupName(),
			false, true);

		// Back to full rendering
		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_EXCLUDE);

		mLoadingBar.finish();
       

         
	}
	///
	void createCamera(void)
    {
        // create camera, but leave at default position
        mCamera = mSceneMgr->createCamera("PlayerCam"); 
        mCamera->setNearClipDistance(10);
    }
	///
    void loadNewGame(void)
	{
		soundMgr->releaseAudio(audioId);
	   global::getSingleton().GUIorGame=false;
       static_cast<MainListener*>(mFrameListener)->hideOverlay();
       sheet->hide();
       //parseDotScene("scene.xml","General",mSceneMgr);   
	   CEGUI::MouseCursor::getSingleton().hide();
	   if (!first)
	   {
	   LoadMap::getSingleton().UnloadM(MapSm);
	   }
	   HUD::getSingleton().Show();
	   global::getSingleton().getPlayer()->setHealthRegeneration(1,100);
	   LoadMap::getSingleton().LoadM(mChCtrl.getMapToLoad(),"Ogre Testmap","/scripts","run3/maps","General",mSceneMgr);//chapter01,"Ogre Testmap","/scripts","run3/maps","General",mSceneMgr);
	   first=false;
	   ingame=true;
	   player->noclip=false;
	   
	}

	void loadSaveGame(void)
	{
		soundMgr->releaseAudio(audioId);
	   global::getSingleton().GUIorGame=false;
       static_cast<MainListener*>(mFrameListener)->hideOverlay();
       sheet->hide();
       //parseDotScene("scene.xml","General",mSceneMgr);   
	   CEGUI::MouseCursor::getSingleton().hide();
	   if (!first)
	   {
	   LoadMap::getSingleton().UnloadM(MapSm);
	   }
	   HUD::getSingleton().Show();
	   LoadMap::getSingleton().LoadM(SaveGame::getSingleton().openLast(),"Ogre Testmap","/scripts","run3/maps","General",mSceneMgr);
	   first=false;
	   ingame=true;
	   player->noclip=false;
	   
	}


	///
    void createScene(void)
    {
		mCamera->setAutoAspectRatio(true);
		menuShown=false;
		gLeftViewport = mWindow->getViewport(0);
		
		managers.push_back(FacialAnimationManager::getSingletonPtr());
		managers.push_back(ZonePortalManager::getSingletonPtr());
		managers.push_back(SoftwareOcclusionCulling::getSingletonPtr());
#ifdef MULTIHEAD
		/*** Multihead : add a viewport to the second screen */
		if(mWindow2)
			gRightViewport = mWindow2->addViewport(mCamera);
#endif

		Run3Batcher::getSingleton().init(mSceneMgr);
		global::getSingleton().setSceneManager(mSceneMgr);
		SoftwareOcclusionCulling::getSingleton().init();
		Timeshift::getSingleton().init();
		global::getSingleton().setLuaState(pL);
		global::getSingleton().setCamera(mCamera);
		Generator::getSingleton().init();
		ExplosionManager::getSingleton().init();
		GibManager::getSingleton().init();
		Credits::getSingleton().init();
		SaveGame::getSingleton().init();
	    cfg.load(mSoundPath + "sound.cfg");
        soundpath = cfg.getSetting("Path");	
	    cfg.load(mGameMenu + "/menu.cfg");

		sound = cfg.getSetting("Sound");
		if (sound!="none")
		sound = mSoundPath+"/"+sound;

	    click = cfg.getSetting("Click");
		click = mSoundPath+"/"+click;

	    move = cfg.getSetting("MouseEnter");
		move = mSoundPath+"/"+move;

	    newgame = cfg.getSetting("New");
		options_t = cfg.getSetting("Options");
	    newgame2 = cfg.getSetting("Test01");
		newgame3 = cfg.getSetting("Test02");
		quit = cfg.getSetting("Quit");	
		overlay = cfg.getSetting("Overlay");
		chapter01 = cfg.getSetting("Chapter01Map","","techdemo_01");
		physUpdate = StringConverter::parseReal(cfg.getSetting("PhysUPD","","60"));
		backMap = cfg.getSetting("backgroundMap","","no_map_run3");
		
		if (backMap=="multiple")
		{
			LogManager::getSingleton().logMessage("Run3 multiple background!");
			backMaps_number = StringConverter::parseUnsignedInt(cfg.getSetting("backMapsN","","0"));
			backMaps = cfg.getMultiSetting("backMap","");
		}

		//ai config
		cfg.load("run3/core/ai.cfg");
		AIType=StringConverter::parseInt(cfg.getSetting("aiType"));
		Vector3 genNodes(StringConverter::parseReal(cfg.getSetting("XGenNode")),StringConverter::parseReal(cfg.getSetting("YGenNode")),StringConverter::parseReal(cfg.getSetting("ZGenNode")));
		bool autoGen = StringConverter::parseBool(cfg.getSetting("autoCreateNPCNodes"));
		//MaterialManager::
		//GUI Def
        mGUIRenderer = new CEGUI::OgreCEGUIRenderer(mWindow, Ogre::RENDER_QUEUE_OVERLAY, true, 3000, mSceneMgr);
        mGUISystem = new CEGUI::System(mGUIRenderer);
        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
        CEGUI::SchemeManager::getSingleton().loadScheme((CEGUI::utf8*)"TaharezLook.scheme");
        mGUISystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
        mGUISystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-12");
        sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"run3gui.layout"); 
        mGUISystem->setGUISheet(sheet);
		
        setupEventHandlers();
		/* ///****************************************
        /// Magic Stuff
        /// and our RenderQueueListener
        ///****************************************
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
		//cfg.load("run3/core/magic.cfg");
		MagicManager::getSingleton().init(mCamera,mWindow,"");//cfg.getSetting("magicPath","","run3/"));

		//camera defs
        /*nodeC = mSceneMgr->getRootSceneNode()->createChildSceneNode("CamNode1", Vector3(-400, 200, 400));
        nodeC->yaw(Degree(-45));
        nodeC->attachObject(mCamera);
		new CWeapon;
		player->init(mSceneMgr,mWorld,mCamera,soundMgr,mRoot);
		player->noclip=true;
		player->debug=false;*/
		
		global::getSingleton().setRoot(mRoot);
		global::getSingleton().setWindow(mWindow);
        //Map loader def
		
		//effects
		BloodEmitter::getSingleton().init();
        //Menu music def
        soundMgr = SoundManager::createManager(); 
        soundMgr->init(); 
		global::getSingleton().setSoundManager(soundMgr);
		soundMgr->setAudioPath((char*)".\\");
        
		soundMgr->setListenerPosition(Vector3(0,0,0),Vector3::ZERO,Quaternion::IDENTITY);
		MusicPlayer::getSingleton().init();

		Run3SoundRuntime::getSingleton().init();
		new CWeapon;
		HUD::getSingleton().init(false,mRoot);
		HUD::getSingleton().SetHealth(100);
		Energy::getSingleton().init(1000,0.1f,true);
		player->init(mSceneMgr,mWorld,mCamera,soundMgr,mRoot);
		player->noclip=true;
		player->debug=false;
		CrosshairOperator::getSingleton().init();
		global::getSingleton().setPlayer(player);
		global::getSingleton().setWorld(mWorld);
		BlastWave* bw = new BlastWave("BlastWave");
		bw->init();
		global::getSingleton().mBw=bw;
		Modulator::getSingleton().init();
		SuperFX::getSingleton().init();
		BulletHitManager::getSingleton().init();
		Loader::getSingleton().init(mSceneMgr,gLeftViewport,gRightViewport);
		Loader::getSingleton().parseGFX();
		MirrorManager::getSingleton().init();
		LightPerfomanceManager::getSingleton().init();
		//console vars and commands
		new ConCommands;
		ConCommands::getSingleton().init(player,mRoot);
	 	OgreConsole::getSingleton().init(mRoot);
		ConCommands::getSingleton().Compile();
	    LoadMap::getSingleton().init(mSceneMgr,mWorld,mCamera,soundMgr,player);
		//mCamera->setFarClipDistance(10000.0f);
		//global::getSingleton().setMapLoader(ml);
		OgreConsole::getSingleton().setVisible(false);

		//NPCs
		NODELIST_NEW
		/*NPCNode* n1 = new NPCNode(mSceneMgr,Vector3::ZERO);
		nList->addNPCNode(n1);*/
		global::getSingleton().setNodeList(nList);
		AIManager::getSingleton().init(AIType,genNodes,autoGen);
		NPCManager::getSingleton().init();
		//shadows
		
		//effects
		new FadeListener;
		FadeListener::getSingleton().init(mRoot);

		//weapon singleton creation
		
		//Computer display overlay
	    Display::getSingleton().init(false,pL);
		//Display::getSingleton().runScript("run3/lua/c64.lua");
		//overley for loading levels
		
		SceneLoadOverlay::getSingleton().init(mRoot);
		//sequence int
		Sequence::getSingleton().init(mRoot,mSceneMgr,soundMgr);
    }

    void createFrameListener(void)
    {
        mFrameListener= new MainListener(mWindow, mCamera, mSceneMgr);
        mRoot->addFrameListener(mFrameListener);
		/// Physics
		mOgreNewtListener = new OgreNewt::BasicFrameListener(mWindow, mCamera, mSceneMgr, mWorld,physUpdate );
		mRoot->addFrameListener( mOgreNewtListener );
    }

/*
    static void Quit(vector<String>& param)
    {
       CEGUI::WindowManager& qwmgr = CEGUI::WindowManager::getSingleton();
	   qwmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/QuitButton")->fireEvent(CEGUI::PushButton::EventClicked, CEGUI::EventArgs());
    }*/
    void setupEventHandlers(void)
    {
		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
		//quit button
		wmgr.getWindow((CEGUI::utf8*)"root/")->hide();
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/QuitButton")->setText(quit);
        wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/QuitButton")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleQuit, this));
        wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/QuitButton")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMoveExit, this));     
		//new button
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton")->setText(newgame);
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleNew, this));
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMove, this));  
		//new game test button
		
		// Options

		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton1")->setText(options_t);
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton1")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleOptions, this));
		wmgr.getWindow((CEGUI::utf8*)"Run3Gui/TabCtrl/Page1/NewButton1")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMove, this));  

		//setup Chapters
		
		//Setup windows
		
  
	}
    void setupNewHandler(void)
    {
	    CEGUI::WindowManager& wmgr2 = CEGUI::WindowManager::getSingleton();
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton")->setText(newgame2);
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleNewGame, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMoveNewG, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton1")->setText(newgame3);
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton1")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleLoadGame, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton1")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMoveNewG, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/PrvButton")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMoveNewG, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/NextButton")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMoveNewG, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/PrvButton")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handlePrevChapter, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/NextButton")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleNextChapter, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/PrvButton")->hide();

		mChCtrl.setPrevNextWindows(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/PrvButton"),wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/NextButton"));
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/"),"tlwintro");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/2"),"tlwhome01");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/3"),"tlwhome02");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/4"),"tlwstations01");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/5"),"tlwstations02");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/6"),"tlwstations03");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/7"),"tlwdolg");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/8"),"tlwcao");
		mChCtrl.pushBackChapter(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/9"),"tlwoutro");
		mChCtrl.finalize();
		//wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/")->setLookNFeel("TaharezLook/StaticShared3T");
		//wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/")->setProperty("Image", "Chapter01/ClientBrush");
		//String dta = Ogre::String(wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/")->getProperty("Image").c_str());
		//LogManager::getSingleton().logMessage(dta);
		//CEGUI::Window *mChapter = wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton1");
		//mChapter->getProperty
		//Options
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton2")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleOptionsAccept, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton2")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMove, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton3")->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&Run3App::handleOptionsCancel, this));
		wmgr2.getWindow((CEGUI::utf8*)"Run3Win2/MainWindow/NewGameButton3")->subscribeEvent(CEGUI::PushButton::EventMouseEnters, CEGUI::Event::Subscriber(&Run3App::handleMove, this));
	}

	bool handleOptionsAccept(const CEGUI::EventArgs& e)
    {
		//CEGUI::Slider* sldr =  (CEGUI::Slider*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//11");
		CEGUI::Scrollbar* sldr=  (CEGUI::Scrollbar*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//11");
		//Mouse sens
		Real sens = sldr->getScrollPosition();
		player->setMouseSensivity(sens);
		//sldr =  (CEGUI::Slider*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//1");

		CEGUI::Checkbox* alls = (CEGUI::Checkbox*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//Subt");
		
		bool allowsubtitles = alls->isSelected();
		player->setAllowSubtitles(allowsubtitles);
		CEGUI::Window* editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
		editorWindow->getChild((CEGUI::utf8*)"Run3Menu/")->hide();
        return true;
    }

	bool handleOptionsCancel(const CEGUI::EventArgs& e)
    {
		CEGUI::Window* editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
		editorWindow->getChild((CEGUI::utf8*)"Run3Menu/")->hide();
        return true;
    }

    bool handleQuit(const CEGUI::EventArgs& e)
    {
        static_cast<MainListener*>(mFrameListener)->requestShutdown();
		//soundMgr->setSoundPosition(audioId5,player->get_location());
        //soundMgr->playAudio( audioId5, true );
		/*soundMgr->setSoundPosition(audioId2,player->get_location());
        soundMgr->playAudio( audioId2, true );*/
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
        return true;
    }

	bool handleNextChapter(const CEGUI::EventArgs& e)
    {
		if (!mChCtrl.getBusy())
		{
		mChCtrl.nextChapter();
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/tank_readyfire1.wav",2,false);
		}
        return true;
    }

	bool handlePrevChapter(const CEGUI::EventArgs& e)
    {
		if (!mChCtrl.getBusy())
		{
		mChCtrl.prevChapter();
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/tank_readyfire1.wav",2,false);
		}
        return true;
    }

    bool handleNew(const CEGUI::EventArgs& e)
    {
		//if (!menuShown)
		//{
		//	menuShown=true;
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
        if (!menuShown)
		{
			menuShown=true;
        if(mEditorGuiSheet)
            CEGUI::WindowManager::getSingleton().destroyWindow(mEditorGuiSheet);
        
		
        mEditorGuiSheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"run3menu.layout");
		
        CEGUI::Window* editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
        editorWindow->addChildWindow(mEditorGuiSheet->getChild((CEGUI::utf8*)"Run3Win2/MainWindow"));
		editorWindow->addChildWindow(mEditorGuiSheet->getChild((CEGUI::utf8*)"Run3Menu/"));
		editorWindow->getChild((CEGUI::utf8*)"Run3Menu/")->hide();

		
		
		setupNewHandler();

		}
		else
		{
			CEGUI::Window* editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
			editorWindow->getChild((CEGUI::utf8*)"Run3Win2/MainWindow")->show();
			editorWindow->getChild((CEGUI::utf8*)"Run3Menu/")->hide();
		}
		
        return true;
    }

	bool handleOptions(const CEGUI::EventArgs& e)
    {
		CEGUI::Window* editorWindow;
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
		if (!menuShown)
		{
			menuShown=true;
        if(mEditorGuiSheet)
            CEGUI::WindowManager::getSingleton().destroyWindow(mEditorGuiSheet);
        
		
        mEditorGuiSheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"run3menu.layout");
		
        editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
        editorWindow->addChildWindow(mEditorGuiSheet->getChild((CEGUI::utf8*)"Run3Win2/MainWindow"));
		editorWindow->addChildWindow(mEditorGuiSheet->getChild((CEGUI::utf8*)"Run3Menu/"));
		editorWindow->getChild((CEGUI::utf8*)"Run3Win2/MainWindow")->hide();
		
		
		setupNewHandler();
		
		}
		else
		{
			editorWindow = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"root");
			editorWindow->getChild((CEGUI::utf8*)"Run3Win2/MainWindow")->hide();
			editorWindow->getChild((CEGUI::utf8*)"Run3Menu/")->show();
		}

		CEGUI::Scrollbar* sldr=  (CEGUI::Scrollbar*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//11");
		sldr->setScrollPosition(player->getMouseSensivity());
		CEGUI::Checkbox* alls = (CEGUI::Checkbox*)CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"Run3Menu//Subt");
		bool allowsubtitles = player->getAllowSubtitles();
		alls->setSelected(allowsubtitles);

        return true;
    }

    bool handleNewGame(const CEGUI::EventArgs& e)
    {
		//soundMgr->setSoundPosition(audioId7,player->get_location());
        //soundMgr->playAudio( audioId7, true );
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
		/*soundMgr->setSoundPosition(audioId2,player->get_location());
        soundMgr->playAudio( audioId2, true );*/
        loadNewGame();
        return true;
    }

	 bool handleLoadGame(const CEGUI::EventArgs& e)
    {
		//soundMgr->setSoundPosition(audioId7,player->get_location());
        //soundMgr->playAudio( audioId7, true );
		Run3SoundRuntime::getSingleton().emitSound(click,2,false);
		/*soundMgr->setSoundPosition(audioId2,player->get_location());
        soundMgr->playAudio( audioId2, true );*/
        loadSaveGame();
        return true;
    }

	bool handleMove(const CEGUI::EventArgs& e)
    {
		//soundMgr->setSoundPosition(audioId3,player->get_location());
        //soundMgr->playAudio( audioId3, true );move
		Run3SoundRuntime::getSingleton().emitSound(move,1,false);
		/*soundMgr->setSoundPosition(audioId3,player->get_location());
        soundMgr->playAudio( audioId3, true );*/
        return true;
    }

	bool handleMoveExit(const CEGUI::EventArgs& e)
    {
		//soundMgr->setSoundPosition(audioId4,player->get_location());
        //soundMgr->playAudio( audioId4, true );
		Run3SoundRuntime::getSingleton().emitSound(move,1,false);
		/*soundMgr->setSoundPosition(audioId3,player->get_location());
        soundMgr->playAudio( audioId3, true );*/
        return true;
    }

	bool handleMoveNewG(const CEGUI::EventArgs& e)
    {
		//soundMgr->setSoundPosition(audioId6,player->get_location());
        //soundMgr->playAudio( audioId6, true );
		Run3SoundRuntime::getSingleton().emitSound(move,1,false);
		/*soundMgr->setSoundPosition(audioId3,player->get_location());
        soundMgr->playAudio( audioId3, true );*/
        return true;
    }
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"


INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
int main(int argc, char **argv)
#endif
{
	//Bootstrap.
	pL = lua_open();

    OpenLuaLibraries(pL);
    // Create application object
    Run3App app;

    try {
        app.go();
    } catch(Ogre::Exception& e) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBoxA(NULL, e.getFullDescription().c_str(), "An exception has occurred!",
            MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        fprintf(stderr, "An exception has occurred: %s\n",
            e.getFullDescription().c_str());
#endif
    }
	lua_close(pL);

    return 0;
}

