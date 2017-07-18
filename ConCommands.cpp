/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "ConCommands.h"
#include "Loader.h"
#include "NPCManager.h"
#include "Run3Benchmark.h"

template<> ConCommands *Singleton<ConCommands>::ms_Singleton=0;

ConCommands::ConCommands()
{

}

ConCommands::~ConCommands()
{

}


void ConCommands::init(Player* ply,Root* mRoot)
{
	//play=ply;
	/*Trigger* trig = new Trigger(mRoot);
	AxisAlignedBox aab = AxisAlignedBox(Vector3(-7905,-100,-4250),Vector3(-7305,700,-3350));
	trig->init(aab,ply);
	trig->setcallback(Gluk);
	trig->show_box(true);*/
}

void ConCommands::Compile()
{
	//luaactivated=false;
	OgreConsole::getSingleton().addCommand("noclip",this->Noclip);
	OgreConsole::getSingleton().addCommand("fade",this->Fade);
	OgreConsole::getSingleton().addCommand("select",this->Give);
	//weapons
	//HUD
	OgreConsole::getSingleton().addCommand("setred",this->SetRed);
	OgreConsole::getSingleton().addCommand("setgreen",this->SetGreen);
	OgreConsole::getSingleton().addCommand("setblue",this->SetBlue);
	OgreConsole::getSingleton().addCommand("a1gfdo4abfs",this->a1gfdo4abfs);
	OgreConsole::getSingleton().addCommand("toggleoldtv",this->toggleOldTv);
	OgreConsole::getSingleton().addCommand("togglenightvision",this->toggleNightvision);
	OgreConsole::getSingleton().addCommand("changefov",this->changeFOV);
	OgreConsole::getSingleton().addCommand("stats",this->printStats);
	OgreConsole::getSingleton().addCommand("c64",this->Commodore);
	OgreConsole::getSingleton().addCommand("testcompos",this->testCompos);
	OgreConsole::getSingleton().addCommand("testcompos2",this->testCompos2);
	OgreConsole::getSingleton().addCommand("drug1",this->drug1);
	OgreConsole::getSingleton().addCommand("drug2",this->drug2);
	OgreConsole::getSingleton().addCommand("drug3",this->drug3);
	OgreConsole::getSingleton().addCommand("buyweapon",this->buyWeapon);
	OgreConsole::getSingleton().addCommand("god",this->god);
	OgreConsole::getSingleton().addCommand("timesh",this->timesh);
	OgreConsole::getSingleton().addCommand("ts",this->ts);
	OgreConsole::getSingleton().addCommand("dssao",this->disssao);
	OgreConsole::getSingleton().addCommand("essao",this->enablessao);
	OgreConsole::getSingleton().addCommand("spawnzom",this->spawnzom);
	OgreConsole::getSingleton().addCommand("lua",this->lua);
	OgreConsole::getSingleton().addCommand("benchmark",this->benchmark);
}

void ConCommands::buyWeapon(vector<String>& param)
{
	CWeapon::getSingleton().addWeapon(param[1]);
}

void ConCommands::ts(vector<String>& param)
{
	Timeshift::getSingleton().toggleTStop();
}

void ConCommands::benchmark(vector<String>& param)
{
	Run3Benchmark::getSingleton().benchMarkMap(param[1]);
}

void ConCommands::spawnzom(vector<String>& param)
{

	NPCSpawnProps prop;
	prop.farFind=10000;
	prop.health=30;
	prop.mesh="marazm02.mesh";
	prop.yShift=0;
	prop.mName="";
	prop.animated=true;
	prop.rot=Quaternion::IDENTITY;
	prop.spCPos=global::getSingleton().getPlayer()->getFObject()+Vector3(0,150,0);
	prop.scale=Vector3(80,80,80);
	prop.spCName="npc_enemy";
	prop.velocity=98;
	prop.stopAtDist=false;
	prop.stopDist=1;
	prop.physPosit=Vector3(0,-1.5f,0);
	prop.physSize=Vector3(0.3f,0.4f,0.3f);
	prop.angle=180;
	prop.ax=Vector3(0,1,0);
	prop.stupidsounds=false;
	prop.attackAnimDist=105;
	prop.headshot=true;
	prop.headshotDist=20.0f;
	prop.headMesh="marazm02_head.mesh";
	prop.headBone="BMan0003-Head";
	LogManager::getSingleton().logMessage("CC: npc passed, all data was got;ready for spawn");
	NPCManager::getSingleton().npc_spawn(prop);
}

void ConCommands::Noclip(vector<String>& param)
{
	/*Player* ply = play;
		if (param[1] == "0")
		{
			ply->noclip=false;
		}
		else
		{
			ply->noclip=true;
		}*/
}



void ConCommands::god(vector<String>& param)
{
	global::getSingleton().getPlayer()->god=!global::getSingleton().getPlayer()->god;
	if (global::getSingleton().getPlayer()->god)
	{
	OgreConsole::getSingleton().print("GOD MODE ON");
	}
	else
	{
OgreConsole::getSingleton().print("GOD MODE OFF");
	}
}

void ConCommands::timesh(vector<String>& param)
{
	Timeshift::getSingleton().setTimeK(StringConverter::parseReal(param[1]));
}

void ConCommands::disssao(vector<String>& param)
{
	Loader::getSingleton().disableSSAO();
}

void ConCommands::enablessao(vector<String>& param)
{
	Loader::getSingleton().enaSSAO();
}

void ConCommands::a1gfdo4abfs(vector<String>& param)
{
	OgreConsole::getSingleton().print("________________");
	OgreConsole::getSingleton().print("OLOLOLOLO!");
	OgreConsole::getSingleton().print("Helllooo, guys!");
	OgreConsole::getSingleton().print("My name is Fyodor Zagumennov aka Sgw32!");
	OgreConsole::getSingleton().print("This project is named Run3");
	OgreConsole::getSingleton().print("________________");
	OgreConsole::getSingleton().print("Pst!If somebody(not me) is going to say:");
	OgreConsole::getSingleton().print("~Run3 is MY project! Sgw32 is not a programmer!~");
	OgreConsole::getSingleton().print("And is going to sell it and register as a trademark,");
	OgreConsole::getSingleton().print("REMEMBER!!!111!");
	OgreConsole::getSingleton().print("Those people are idiots and they sucks!!!");
	OgreConsole::getSingleton().print("This project is MY!");
	OgreConsole::getSingleton().print("This source code is MY!");
	OgreConsole::getSingleton().print("Luckily, this secret code is MY!");
	OgreConsole::getSingleton().print("AND NOBODY CAN disprove IT!");
	OgreConsole::getSingleton().print("Or they are absolutely idiots and");
	OgreConsole::getSingleton().print("they are stupid fuckers!");
	OgreConsole::getSingleton().print("________________");
}

void ConCommands::Commodore(vector<String>& param)
{
		OgreConsole::getSingleton().print("     **** COMMODORE 64 BASIC V2 ****");
		OgreConsole::getSingleton().print(" 64K RAM SYSTEM 38911 BASIC BYTES FREE");
		OgreConsole::getSingleton().print(" READY ");
		CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"Commodore");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"Commodore",true);
}
void ConCommands::testCompos(vector<String>& param)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"TestRun3");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"TestRun3",true);

}

void ConCommands::testCompos2(vector<String>& param)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),param[1]);
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),param[1],true);

}


void ConCommands::drug1(vector<String>& param)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"RunFX1");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"RunFX1",true);

}
void ConCommands::drug2(vector<String>& param)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"RunLSD");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"RunLSD",true);

}
void ConCommands::drug3(vector<String>& param)
{
	CompositorManager::getSingleton().addCompositor(global::getSingleton().getCamera()->getViewport(),"RunFX3");
		CompositorManager::getSingleton().setCompositorEnabled(global::getSingleton().getCamera()->getViewport(),"RunFX3",true);

}

void ConCommands::lua(vector<String>& param)
{
	//luaactivated=true;
	if (param[1]=="1")
	{
	OgreConsole::getSingleton().activateLua(true);
	LogManager::getSingleton().logMessage("Lua console functions activated!");
	}
	else
	{
	OgreConsole::getSingleton().activateLua(false);
	LogManager::getSingleton().logMessage("Lua console functions deactivated!");
	}
}

void ConCommands::printStats(vector<String>& param)
{
	OgreConsole::getSingleton().print("FPS:" + StringConverter::toString(global::getSingleton().getWindow()->getLastFPS()));
	OgreConsole::getSingleton().print("Average:"+StringConverter::toString(global::getSingleton().getWindow()->getAverageFPS()));
	OgreConsole::getSingleton().print("Best:"+StringConverter::toString(global::getSingleton().getWindow()->getBestFPS()));
	OgreConsole::getSingleton().print("Worst:" + StringConverter::toString(global::getSingleton().getWindow()->getWorstFPS()));
	OgreConsole::getSingleton().print("Batch Count"+StringConverter::toString(global::getSingleton().getWindow()->getBatchCount()));
}

void ConCommands::toggleOldTv(vector<String>& param)
{
	SuperFX::getSingleton().toggleOldTV();
}

void ConCommands::toggleNightvision(vector<String>& param)
{
	SuperFX::getSingleton().toggleNightvision();
}

void ConCommands::Fade(vector<String>& param)
{
	FadeListener::getSingleton().setDuration(3);
	FadeListener::getSingleton().setDurationF(0.5);
	if (param[1]=="in")
	{
		FadeListener::getSingleton().startIN();
	}
	if (param[1]=="out")
	{
		FadeListener::getSingleton().startOUT();
	}
}
void ConCommands::Gluk()
{
	HUD::getSingleton().SetGreenEnergy(0.4f);
}
void ConCommands::Give(vector<String>& param)
{
	CWeapon::getSingleton().select(param[1]);
}

void ConCommands::changeFOV(vector<String>& param)
{
	global::getSingleton().getPlayer()->changeFOV(StringConverter::parseReal(param[1]));
}



void ConCommands::SetRed(vector<String>& param)
{
	HUD::getSingleton().SetRedEnergy(StringConverter::parseReal(param[1]));
}

void ConCommands::SetGreen(vector<String>& param)
{
	HUD::getSingleton().SetGreenEnergy(StringConverter::parseReal(param[1]));
}

void ConCommands::SetBlue(vector<String>& param)
{
	HUD::getSingleton().SetBlueEnergy(StringConverter::parseReal(param[1]));
}