/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Sequence.h"
#include "LoadMap.h"
#include "tinyxml.h"
#include "POs.h"

template<> Sequence *Singleton<Sequence>::ms_Singleton=0;

Sequence::Sequence()
{
	rest=true;
	sec=0;
}

Sequence::~Sequence()
{
mRoot->removeFrameListener(this);
}

void Sequence::init(Ogre::Root* root,SceneManager* scene,SoundManager* sound)
{
	grpgen=new ElementGroupGenerator();
	pLuaState=global::getSingleton().getLuaState();
	mRoot=root;
	mSceneMgr=scene;
	mSoundManager=sound;
	new EventEntC;
	//new EventChangeLevel;
	//EventChangeLevel::getSingleton().init(mSceneMgr);
	EventEntC::getSingleton().init(mSceneMgr);
	mRoot->addFrameListener(this);

	lua_register(pLuaState, "openDoor",openDoor);
	lua_register(pLuaState, "closeDoor",closeDoor);
	lua_register(pLuaState, "lockDoor",lockDoor);
	lua_register(pLuaState, "unlockDoor",unlockDoor);
	lua_register(pLuaState, "toggleDoor",toggleDoor);
	lua_register(pLuaState, "setRotSpeed",setRotSpeed);
	lua_register(pLuaState, "toggleLight",toggleLight);
	lua_register(pLuaState, "enableLight",enableLight);
	lua_register(pLuaState, "getPixelCount1",getPixelCount1);
	lua_register(pLuaState, "getPixelCount2",getPixelCount2);
	lua_register(pLuaState, "disableLight",disableLight);
	lua_register(pLuaState, "HUDflashText",flashText);
	lua_register(pLuaState, "emitSound",emitSound);
	lua_register(pLuaState, "disableAmbientSound",disableAmbientSound);
	lua_register(pLuaState, "enableAmbientSound",enableAmbientSound);
	lua_register(pLuaState, "emitSound2",emitSound2);
	lua_register(pLuaState, "setCompositorEnabled",setCompositorEnab);
	lua_register(pLuaState, "fragmentGPUProgramParams",fragmentGPUProgramParams);
	lua_register(pLuaState, "fragmentGPUProgramTime",fragmentGPUProgramTime);
	lua_register(pLuaState, "gameText",gameText);
	lua_register(pLuaState, "gameText2",gameText2);
	lua_register(pLuaState, "gameTextColor",gameTextColor);
	lua_register(pLuaState, "gameTextSize",gameTextSize);
	lua_register(pLuaState, "playMusic",playMusic);
	lua_register(pLuaState, "toggleMusic",playMusic);
	lua_register(pLuaState, "setMusicVolume",setMusicVolume);
	lua_register(pLuaState, "stopMusic",stopMusic);
	lua_register(pLuaState, "fadeoutMusic",fadeoutMusic);
	lua_register(pLuaState, "activateMPReg",activateMPReg);
	lua_register(pLuaState, "deactivateMPReg",deactivateMPReg);
	lua_register(pLuaState, "setMPRegVolume",setMPRegVolume);
	lua_register(pLuaState, "Fade",Fade);
	lua_register(pLuaState, "FadeOut",FadeOut);
	lua_register(pLuaState, "setSkyBox",setSkyBox);
	lua_register(pLuaState, "toggleEntity",toggleEntity);
	lua_register(pLuaState, "showEntity",showEntity);
	lua_register(pLuaState, "materialEntity",materialEntity);
	lua_register(pLuaState, "hideEntity",hideEntity);
	lua_register(pLuaState, "playerShake",playerShake);
	lua_register(pLuaState, "startCutScene",startCutScene);
	lua_register(pLuaState, "startSeqScript",startSeqScript);
	lua_register(pLuaState, "changeLevel",changeLevel);
	lua_register(pLuaState, "HUDHide",hideHUD);
	lua_register(pLuaState, "HUDEnable",enableHUD);
	lua_register(pLuaState, "HUDDisable",disableHUD);
	lua_register(pLuaState, "WeaponStrip",stripWeapons);
	lua_register(pLuaState, "addWeapon",addWeapon);
	lua_register(pLuaState, "reverseTrain",reverseTrain);
	lua_register(pLuaState, "startTrain",startTrain);
	lua_register(pLuaState, "setSpeedTrain",setSpeedTrain);
	lua_register(pLuaState, "setAccTrain",setAccTrain);
	lua_register(pLuaState, "setAccDoor",setAccDoor);
	lua_register(pLuaState, "stopTrain",stopTrain);
	lua_register(pLuaState, "startEvent",startEvent);
	lua_register(pLuaState, "powerComputer",powerComputer);
	lua_register(pLuaState, "enableTrainSaveMode",enableTrainSaveMode);
	lua_register(pLuaState, "disableTrainSaveMode",disableTrainSaveMode);
	lua_register(pLuaState, "createParticleSystem",createParticleSystem);
	lua_register(pLuaState, "deleteParticleSystem",deleteParticleSystem);
	lua_register(pLuaState, "setTimeK",setTimeK);
	lua_register(pLuaState, "timeShift",timeShift);
	lua_register(pLuaState, "killEverybody",killEverybody);
	lua_register(pLuaState, "transformToPhys",transformToPhys);
	lua_register(pLuaState, "freezeBod",freezeBod);
	lua_register(pLuaState, "unfreezeBod",unfreezeBod);
	lua_register(pLuaState, "forceDelete",forceDelete);
	lua_register(pLuaState, "toggleParticleSystem",toggleParticleSystem);
	lua_register(pLuaState, "toggleEmitter",toggleEmitter);
	lua_register(pLuaState, "enableEmitter",enableEmitter);
	lua_register(pLuaState, "disableEmitter",disableEmitter);
	lua_register(pLuaState, "enableTrigger",enableTrigger);
	lua_register(pLuaState, "disableTrigger",disableTrigger);
	lua_register(pLuaState, "enableInventory",enableInventory);
	lua_register(pLuaState, "disableInventory",disableInventory);
	lua_register(pLuaState, "godmode",godmode);
	lua_register(pLuaState, "COMSendData",COMSendData);
	lua_register(pLuaState, "runScript",runScript);
	lua_register(pLuaState, "enableTimer",enableTimer);
	lua_register(pLuaState, "disableTimer",disableTimer);
	lua_register(pLuaState, "toggleTimer",toggleTimer);
	lua_register(pLuaState, "player__getposx",player__getposx);
	lua_register(pLuaState,"player__getposy",player__getposy);
	lua_register(pLuaState,"player__getposz",player__getposz);
	lua_register(pLuaState,"player__allowFlash",player__allowFlash);
	lua_register(pLuaState,"player__destroyFlash",player__destroyFlash);
	lua_register(pLuaState,"player__toggleFlash",player__toggleFlash);
	lua_register(pLuaState,"player__setRegeneration",player__setRegeneration);
	lua_register(pLuaState,"cursor__getposx",cursor__getposx);
	lua_register(pLuaState,"cursor__getposy",cursor__getposy);
	lua_register(pLuaState,"bod__getpos",bod__getpos);
	lua_register(pLuaState,"bod__setpos",bod__setpos);
	lua_register(pLuaState,"bod__getpos2",bod__getpos2);
	lua_register(pLuaState,"bod__setpos2",bod__setpos2);
	lua_register(pLuaState,"nod__getpos2",nod__getpos2);
	lua_register(pLuaState,"nod__getpos",nod__getpos);
	lua_register(pLuaState,"nod__setpos2",nod__setpos2);
	lua_register(pLuaState,"bod__getor",bod__getor);
	lua_register(pLuaState,"bod__setor",bod__setor);
	lua_register(pLuaState,"logMessage",logMessage);
	lua_register(pLuaState,"player__getdirx",player__getdirx);
	lua_register(pLuaState,"player__getdiry",player__getdiry);
	lua_register(pLuaState,"player__getdirz",player__getdirz);
	lua_register(pLuaState,"player__getdistf",player__getdistf);
	lua_register(pLuaState,"set__register",set__register);
	lua_register(pLuaState,"get__register",get__register);
	lua_register(pLuaState,"add__register",add__register);
	lua_register(pLuaState,"inc__register",inc__register);
	lua_register(pLuaState,"dec__register",dec__register);
	lua_register(pLuaState,"__all_npcEvent",__all_npcEvent);
	lua_register(pLuaState,"npcEvent",npcEvent);
	lua_register(pLuaState,"npcEvent2",npcEvent2);
	lua_register(pLuaState,"toString",tostringl);
	lua_register(pLuaState,"random_number",random_number);
	lua_register(pLuaState,"VectorToString",tostringl2);
	lua_register(pLuaState,"dssao",dssao);
	lua_register(pLuaState,"setCameraParent",setCameraParent);
	lua_register(pLuaState,"setFullCameraParent",setFullCameraParent);
	lua_register(pLuaState,"resetCameraParent",resetCameraParent);
	lua_register(pLuaState,"teleport",teleport);
	lua_register(pLuaState,"teleport_rel",teleport_rel);
	lua_register(pLuaState,"fireExtinguish",fireExtinguish);
	lua_register(pLuaState,"fireFire",fireFire);
	lua_register(pLuaState,"fireToggle",fireToggle);
	lua_register(pLuaState,"addAmbientSound",addAmbientSound);
	lua_register(pLuaState,"commandLeaderGroup",commandLeaderGroup);
	lua_register(pLuaState,"setNearClipDist",setNearClipDist);
	lua_register(pLuaState,"resetNearClipDist",resetNearClipDist);
	lua_register(pLuaState,"setFarClipDist",setFarClipDist);
	lua_register(pLuaState,"resetFarClipDist",resetFarClipDist);
	lua_register(pLuaState,"shockCutScene",shockCutScene);
	lua_register(pLuaState,"setFov",setFov);
	lua_register(pLuaState,"getFov",getFov);
	lua_register(pLuaState,"removeCutScene",removeCutScene2);
	lua_register(pLuaState,"rotateFuzzyObject",rotateFuzzyObject);
	lua_register(pLuaState,"exitAllComputers",exitAllComputers);
	lua_register(pLuaState,"resetFov",resetFov);
	lua_register(pLuaState,"destroyNPC",destroyNPC);
	lua_register(pLuaState,"SkyX_setTimeMultiplier",SkyX_setTimeMultiplier);
	lua_register(pLuaState,"SkyX_setHeightPosition",SkyX_setHeightPosition);
	lua_register(pLuaState,"SkyX_setOuterRadius",SkyX_setOuterRadius);
	lua_register(pLuaState,"SkyX_setInnerRadius",SkyX_setInnerRadius);
	lua_register(pLuaState,"SkyX_setExposure",SkyX_setExposure);
	lua_register(pLuaState,"SkyX_setMie",SkyX_setMie);
	lua_register(pLuaState,"SkyX_setRayleighMultiplier",SkyX_setRayleighMultiplier);
	lua_register(pLuaState,"SkyX_setSampleNumber",SkyX_setSampleNumber);
	lua_register(pLuaState,"SkyX_updateParameters",SkyX_updateParameters);
	lua_register(pLuaState,"turnoff__Module",turnoff__Module);
	lua_register(pLuaState,"turnoff__Sequence",turnoff__Sequence);
	lua_register(pLuaState,"turnoff__NPCManager",turnoff__NPCManager);
	lua_register(pLuaState,"setNPCManagerStep",setNPCManagerStep);
	lua_register(pLuaState,"deleteDoor",deleteDoor);

	//Load-on-the-go
	lua_register(pLuaState,"processSequence",processSequence);
	lua_register(pLuaState,"processDotScene",processDotScene);
	
	pobjs=POs::getSingleton().getObjects();
	OgreNewt::World* w = global::getSingleton().getWorld();
	mPhysCallback = new TrainMat;
mMatDefault = global::getSingleton().getPlayer()->getPlayerMat();
  trainMat= new OgreNewt::MaterialID( w );
  mMatPair = new OgreNewt::MaterialPair( w, mMatDefault, trainMat );
  mMatPair->setContactCallback( mPhysCallback );
   mMatPair->setContinuousCollisionMode(10); 
  mMatPair = new OgreNewt::MaterialPair( w, trainMat, trainMat );
  mMatPair->setContactCallback( mPhysCallback );
   mMatPair->setContinuousCollisionMode(10); 
   mMatPair->setDefaultSoftness(100);
mMatPair->setDefaultElasticity(100);
   mPhysCallback->mWorld=w;
    mMatPair = new OgreNewt::MaterialPair( w, w->getDefaultMaterialID(), trainMat );
  mMatPair->setContactCallback( mPhysCallback );

}

void Sequence::SetSceneSeq(String seq)
{
	if (seq!="none")
	{
		rest=false;
	sequence = seq;
	SequenceDoc = 0;

	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename(seq, basename, path);

		DataStreamPtr pStream = ResourceGroupManager::getSingleton().
			openResource( basename, "General" );

		//DataStreamPtr pStream = ResourceGroupManager::getSingleton().
		//	openResource( SceneName, groupName );

		String data = pStream->getAsString();
		// Open the .xml File
		SequenceDoc = new TiXmlDocument();
		SequenceDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();

		if( SequenceDoc->Error() )
		{
			rest=true;

			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[Sequence] This scene has no sequence,or TiXml reporated an error.");
			LogManager::getSingleton().logMessage(String(SequenceDoc->ErrorDesc()));
			LogManager::getSingleton().logMessage(StringConverter::toString(SequenceDoc->ErrorRow()));
			LogManager::getSingleton().logMessage(StringConverter::toString(SequenceDoc->ErrorCol()));
			return;
		}
	}
	catch(...)
	{
		rest=true;
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[Sequence] This scene has no sequence,or TiXml reporated an error.");
		return;
	}

	SequenceRoot = SequenceDoc->RootElement();
	if( String( SequenceRoot->Value()) != "sequence"  ) {
		LogManager::getSingleton().logMessage( "[Sequence] Error: Invalid .xml File. Missing <sequence>" );     
		return;
	}
	
	/// Okay,we are under the <sequence> tag!

	TiXmlElement *pElement;

		// Process nodes (?)
	pElement = SequenceRoot->FirstChildElement("adents");
	if(pElement)
		processAdents(pElement);

	// Process externals (?)
	pElement = SequenceRoot->FirstChildElement("events");
	if(pElement)
		processEvents(pElement);
	}
	else
	{
rest=true;
	}

}


void Sequence::SetSceneSeq(TiXmlElement *SequenceRoot)
{

	
	/// Okay,we are under the <sequence> tag!
rest=false;
	TiXmlElement *pElement;

		// Process nodes (?)
	pElement = SequenceRoot->FirstChildElement("adents");
	if(pElement)
		processAdents(pElement);

	// Process externals (?)
	pElement = SequenceRoot->FirstChildElement("events");
	if(pElement)
		processEvents(pElement);
	

}


void Sequence::processAdents(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("trigger");
	while(pElement)
	{
				processTrigger(pElement);
				pElement = pElement->NextSiblingElement("trigger");
	}
	

	ADD_AENT("pickup",processPickup);
	LogManager::getSingleton().logMessage("Sequence: processing computers");
	ADD_AENT("computer",processComputer);
	LogManager::getSingleton().logMessage("Sequence: processing events");
	ADD_AENT("event",processEvent);
	LogManager::getSingleton().logMessage("Sequence: processing lensflare");
	ADD_AENT("flare",processLensFlare);
	LogManager::getSingleton().logMessage("Sequence: processing timers");
	ADD_AENT("timer",processTimer);
	LogManager::getSingleton().logMessage("Sequence: processing dark zones");
	ADD_AENT("darkzone",processDarkZone);
	
	LogManager::getSingleton().logMessage("Sequence: processing ladders");
	ADD_AENT("ladder",processLadder);
	LogManager::getSingleton().logMessage("Sequence: processing fires");
	ADD_AENT("fire",processFire);
	/*pElement = XMLNode->FirstChildElement("computer");
	while(pElement)
	{
		processComputer(pElement);
		pElement = pElement->NextSiblingElement("computer");
	}
	pElement = XMLNode->FirstChildElement("event");
	while(pElement)
	{
		processEvent(pElement);
		pElement = pElement->NextSiblingElement("event");
	}*/
	LogManager::getSingleton().logMessage("Sequence: processing npc");
	pElement = XMLNode->FirstChildElement("npc");
	while(pElement)
	{
		processNPC(pElement);
		pElement = pElement->NextSiblingElement("npc");
	}
	LogManager::getSingleton().logMessage("Sequence: processing npc groups");
	ADD_AENT("npcgroup",processNPCGroup);
	LogManager::getSingleton().logMessage("Sequence: processing buttons");
	pElement = XMLNode->FirstChildElement("button");
	while(pElement)
	{
		processButton(pElement);
		pElement = pElement->NextSiblingElement("button");
	}
	LogManager::getSingleton().logMessage("Sequence: processing cutscenes");
	pElement = XMLNode->FirstChildElement("cutscene");
	while(pElement)
	{
		processCutScene(pElement);
		pElement = pElement->NextSiblingElement("cutscene");
	}
	LogManager::getSingleton().logMessage("Sequence: processing seqscripts");
	pElement = XMLNode->FirstChildElement("seqscript");
	while(pElement)
	{
		processSeqScript(pElement);
		pElement = pElement->NextSiblingElement("seqscript");
	}
	LogManager::getSingleton().logMessage("Sequence: processing trains");
	ADD_AENT("train",processTrain);
	LogManager::getSingleton().logMessage("Sequence: processing pendulums");
	ADD_AENT("pendulum",processPendulum);
	LogManager::getSingleton().logMessage("Sequence: processing fuzzy objects");
	ADD_AENT("fuzzy",processFuzzyObject);
	LogManager::getSingleton().logMessage("Sequence: processing doors");
	pElement = XMLNode->FirstChildElement("door");
	while(pElement)
	{
				processDoor(pElement);
				pElement = pElement->NextSiblingElement("door");
	}	
	LogManager::getSingleton().logMessage("Sequence: processing rots");
	pElement = XMLNode->FirstChildElement("rot");
	while(pElement)
	{
				processRot(pElement);
				pElement = pElement->NextSiblingElement("rot");
	}	

	SequenceLuaCallback::getSingleton().setDoors(doors);
	LogManager::getSingleton().logMessage("Sequence: processing lua");
	pElement = XMLNode->FirstChildElement("lua");
	while(pElement)
	{
		processLuaScript(pElement);
		pElement = pElement->NextSiblingElement("lua");
	}
	pElement = XMLNode->FirstChildElement("onexit");
	while(pElement)
	{
		processLuaScriptOnExit(pElement);
		pElement = pElement->NextSiblingElement("onexit");
	}
	Vector3 post;
}

void Sequence::processLuaScript(TiXmlElement *XMLNode)
{
	LogManager::getSingleton().logMessage("Lua run:"+getAttrib(XMLNode,"script",""));
	RunLuaScript(pLuaState,getAttrib(XMLNode,"script","").c_str());
}

void Sequence::processLuaScriptOnExit(TiXmlElement *XMLNode)
{
	LogManager::getSingleton().logMessage("Lua deferred run:"+getAttrib(XMLNode,"script",""));
	LoadMap::getSingleton().setScriptOnExit(getAttrib(XMLNode,"script",""));
	//RunLuaScript(pLuaState,getAttrib(XMLNode,"script","").c_str());
}


void Sequence::processEvents(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("trigger");
	while(pElement)
	{
		processTriggerEvent(pElement);
		pElement = pElement->NextSiblingElement("trigger");
	}
	pElement = XMLNode->FirstChildElement("cutscene");
	while(pElement)
	{
		processCutSceneEvent(pElement);
		pElement = pElement->NextSiblingElement("cutscene");
	}
	pElement = XMLNode->FirstChildElement("seqscript");
	while(pElement)
	{
		processSeqScriptEvent(pElement);
		pElement = pElement->NextSiblingElement("seqscript");
	}
}

void Sequence::processTrigger(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	Real x1 = getAttribReal(XMLNode,"x1",0);
	Real x2 = getAttribReal(XMLNode,"x2",0);
	Real y1 = getAttribReal(XMLNode,"y1",0);
	Real y2 = getAttribReal(XMLNode,"y2",10);
	Real z1 = getAttribReal(XMLNode,"z1",10);
	Real z2 = getAttribReal(XMLNode,"z2",10);
	String name = getAttrib(XMLNode,"name","undefined"); 
	bool show = getAttribBool(XMLNode,"show",false);
	bool sw = getAttribBool(XMLNode,"sw",false);
	bool en = getAttribBool(XMLNode,"enabled",true);
	bool multiple = getAttribBool(XMLNode,"multiple",false);
	Trigger* trig = new Trigger(mRoot);
	Vector3 pos,scale;
	
	if (sw)
	{
		TiXmlElement* pElement;
		pElement=XMLNode->FirstChildElement("position");
		pos=parseVector3(pElement);
		pElement=XMLNode->FirstChildElement("scale");
		scale=parseVector3(pElement);
		trig->init(pos,scale,global::getSingleton().getPlayer());
	}
	else
	trig->init(AxisAlignedBox(x1,y1,z1,x2,y2,z2),global::getSingleton().getPlayer());
	trig->setEnabled(en);
	trig->setFootstepSound(getAttrib(XMLNode,"footstep",""));
	trig->show_box(show);
	trig->setname(name);

	if (multiple)
	{
		trig->setMultiple(getAttrib(XMLNode,"luaOnEnter",""),getAttrib(XMLNode,"luaOnLeave",""));

		pElement = XMLNode->FirstChildElement("lighton");
		while(pElement)
		{
					if (mSceneMgr->hasLight(getAttrib(pElement,"name")))
					{
						Light* l = mSceneMgr->getLight(getAttrib(pElement,"name"));	
						trig->addMultipleLightOn(l);
					}
					pElement = pElement->NextSiblingElement("lighton");
		}
		pElement = XMLNode->FirstChildElement("lightoff");
		while(pElement)
		{
					if (mSceneMgr->hasLight(getAttrib(pElement,"name")))
					{
						Light* l = mSceneMgr->getLight(getAttrib(pElement,"name"));	
						trig->addMultipleLightOff(l);
					}
					pElement = pElement->NextSiblingElement("lightoff");
		}
	}

	triggers.push_back(trig);
	LogManager::getSingleton().logMessage("!Trigger found");
	//triggers
}

void Sequence::processTrain(TiXmlElement *XMLNode)
{
	Train* train = new Train();
	String name = getAttrib(XMLNode,"name","");
	Real speed = getAttribReal(XMLNode,"speed",30.0f);
	Real dist = getAttribReal(XMLNode,"dist",-1.0f);
	bool setor = getAttribBool(XMLNode,"setor",true);
	bool derail_capable = getAttribBool(XMLNode,"derail",false);
	bool setor2 = getAttribBool(XMLNode,"setor2",true);
	bool infint = getAttribBool(XMLNode,"inf",false);
	Real yawAngle = getAttribReal(XMLNode,"yaw",180.0f);
TiXmlElement *pElement;
pElement = XMLNode->FirstChildElement("entity");
Entity* ent = mSceneMgr->createEntity(name,getAttrib(pElement,"meshFile","box.mesh"));
if (dist!=-1.0f)
	ent->setRenderingDistance(dist);
String materialFile = getAttrib(XMLNode, "materialFile");
if (!materialFile.empty())
ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ
pElement = XMLNode->FirstChildElement("scale");
Vector3 scale = parseVector3(pElement);
pElement = XMLNode->FirstChildElement("rotation");
Quaternion rot = parseQuaternion(pElement);
pElement = XMLNode->FirstChildElement("position");
SceneNode* sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,parseVector3(pElement),rot);
sn->setScale(scale);
sn->attachObject(ent);
bool sim = getAttribBool(XMLNode,"simple",true);
LogManager::getSingleton().logMessage("Initializing trains:");
if (sim)
	train->init(name,sn,speed);
else
	train->init_nosetup(name,sn,speed);



train->stopSound=getAttrib(XMLNode,"stopSound","run3/sounds/tram_squeak.wav");
train->startSound=getAttrib(XMLNode,"startSound","run3/sounds/train_horn2.wav");
train->movingSound=getAttrib(XMLNode,"movingSound","run3/sounds/train2.wav");
train->soundDuration=getAttribReal(XMLNode,"soundDuration",9);
if(derail_capable)
	train->setMaterialGroupID(trainMat);
train->setFixOrient(setor);
train->setYawAngle(yawAngle);
LogManager::getSingleton().logMessage("Processing keypoints:");
	pElement = XMLNode->FirstChildElement("keyPoint");
	while(pElement)
	{
		train->addKeyPoint(parseVector3(pElement),getAttrib(pElement,"script","none"),getAttribBool(pElement,"startup",false),false);
		pElement = pElement->NextSiblingElement("keyPoint");
	}
	LogManager::getSingleton().logMessage("Processing details of train:");
if (!sim)
{
	pElement = XMLNode->FirstChildElement("psys");
	if (pElement)
	{
		TiXmlElement *pElement1;
		String partName = getAttrib(pElement,"psysName","");
		//Quaternion rot;
		pElement1 = pElement->FirstChildElement("position");
		Vector3 pos = parseVector3(pElement1);
		//Entity* ent = mSceneMgr->createEntity(getAttrib(pElement1,"name",""),getAttrib(pElement1,"meshFile","box.mesh"));
		LogManager::getSingleton().logMessage("1");
		pElement1 = pElement->FirstChildElement("scale");
		Vector3 scale = parseVector3(pElement1);
		train->addParticleSystem(partName,pos,scale);
	}

	pElement = XMLNode->FirstChildElement("object");
	while(pElement)
	{
		TiXmlElement *pElement1;
		Quaternion rot;
		pElement1 = pElement->FirstChildElement("entity");
		Entity* ent = mSceneMgr->createEntity(getAttrib(pElement1,"name",""),getAttrib(pElement1,"meshFile","box.mesh"));
		if (dist!=-1.0f)
			ent->setRenderingDistance(dist);
		LogManager::getSingleton().logMessage("1");
		pElement1 = pElement->FirstChildElement("scale");
		Vector3 scale = parseVector3(pElement1);
		
LogManager::getSingleton().logMessage("122");

		
		pElement1 = pElement->FirstChildElement("rotation");
		if (pElement1)
		rot = parseQuaternion(pElement1);
		pElement1 = pElement->FirstChildElement("position");
				//ent = mSceneMgr->createEntity(getAttrib(XMLNode,"name",""),getAttrib(pElement,"meshFile","box.mesh"));
		if (ent)
		train->addDetail(ent, parseVector3(pElement1),scale,rot);
		else
			LogManager::getSingleton().logMessage("Train: error adding a detail object");
		LogManager::getSingleton().logMessage("Train: finished objct");
		pElement = pElement->NextSiblingElement("object");
	}

	pElement = XMLNode->FirstChildElement("nocollide");
	while(pElement)
	{
		TiXmlElement *pElement1;
		Quaternion rot;
		pElement1 = pElement->FirstChildElement("entity");
		Entity* ent = mSceneMgr->createEntity(getAttrib(pElement1,"name",""),getAttrib(pElement1,"meshFile","box.mesh"));
		if (dist!=-1.0f)
			ent->setRenderingDistance(dist);
		LogManager::getSingleton().logMessage("1");
		pElement1 = pElement->FirstChildElement("scale");
		Vector3 scale = parseVector3(pElement1);
		
LogManager::getSingleton().logMessage("122");

		
		pElement1 = pElement->FirstChildElement("rotation");
		if (pElement1)
		rot = parseQuaternion(pElement1);
		pElement1 = pElement->FirstChildElement("position");
				//ent = mSceneMgr->createEntity(getAttrib(XMLNode,"name",""),getAttrib(pElement,"meshFile","box.mesh"));
		if (ent)
		train->addNoCollide(ent, parseVector3(pElement1),scale,rot);
		else
			LogManager::getSingleton().logMessage("Train: error adding a detail object");
		LogManager::getSingleton().logMessage("Train: finished objct");
		pElement = pElement->NextSiblingElement("nocollide");
	}

	LogManager::getSingleton().logMessage("Train: finishing");
	train->finish_build();
	train->setInfinite(infint);
	LogManager::getSingleton().logMessage("Train: ok");
}

if(setor2)
	train->addUpVectors();

	if (getAttribBool(XMLNode,"start",false))
		train->start();
	trains.push_back(train);
	LogManager::getSingleton().logMessage("!Train found");
}



void Sequence::processTimer(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","");
	if (hasTimer(name))
		return;
	Run3Timer* timer = new Run3Timer();
	
	String lua = getAttrib(XMLNode,"lua","");
	Real period = getAttribReal(XMLNode,"period",1.0f);
	Real nam = getAttribReal(XMLNode,"noiseAmount",0.0f);
	timer->init(name,lua,period);
	timer->setNoiseAmount(nam);
	if (getAttribBool(XMLNode,"start",false))
		timer->enable();
	timers.push_back(timer);
	LogManager::getSingleton().logMessage("!Timer found");
}


void Sequence::processFire(TiXmlElement *XMLNode)
{

	TiXmlElement* pElement = XMLNode->FirstChildElement("position");
	Vector3 pos;
	Quaternion rot;
	Vector3 scale=Vector3(1,1,1); 
	if (pElement)
		pos = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("rotation");
	if (pElement)
		rot = parseQuaternion(pElement);
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)	
		scale = parseVector3(pElement);
	String name2 = getAttrib(XMLNode,"name");
	SceneNode* bNod;
	bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(name2,pos,rot);

	String name = getAttrib(XMLNode, "pSys");
	
	Real rd = getAttribReal(XMLNode,"renderDist",5000);
	Fire* f = new Fire(mSceneMgr,bNod,name,rd);
	f->fire();
	f->setName(name2);
	fires.push_back(f);
}

void Sequence::processButton(TiXmlElement *XMLNode)
{
	Run3::Button* butt = new Run3::Button();
	TiXmlElement* pElement = XMLNode->FirstChildElement("position");
	Vector3 pos;
	Quaternion rot;
	Vector3 scale=Vector3(1,1,1); 
	if (pElement)
		pos = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("rotation");
	if (pElement)
		rot = parseQuaternion(pElement);
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)	
		scale = parseVector3(pElement);
	String fileName = getAttrib(XMLNode,"meshName","box.mesh");
	String buttonName = getAttrib(XMLNode,"buttonName");
	bool movableButton = getAttribBool(XMLNode,"movable",false);
	SceneNode* bNod;
	if (buttonName.empty())
	{
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,rot);
	}
	else
	{
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(buttonName,pos,rot);
	}
	bNod->setScale(scale);
	Entity* ent = mSceneMgr->createEntity(bNod->getName(),fileName);

	String materialFile = getAttrib(XMLNode, "materialFile");
	if (!materialFile.empty())
	ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ
	bNod->attachObject(ent);
	if (!movableButton)
	{
		butt->init(bNod,0);
	}
	else
	{
		butt->initMovable(bNod,0,getAttribReal(XMLNode,"mass",40));
		if (getAttribBool(XMLNode,"oncont",false))
		{
			butt->setAlarmOnContact();
		}
	}
	butt->setCallback(getAttrib(XMLNode,"luaScript"));
	buttons.push_back(butt);
	LogManager::getSingleton().logMessage("!Button found");
}


void Sequence::processFuzzyObject(TiXmlElement *XMLNode)
{

	TiXmlElement* pElement = XMLNode->FirstChildElement("position");
	Vector3 pos;
	Quaternion rot;
	Vector3 scale=Vector3(1,1,1); 
	if (pElement)
		pos = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("rotation");
	if (pElement)
		rot = parseQuaternion(pElement);
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)	
		scale = parseVector3(pElement);
	String fileName = getAttrib(XMLNode,"meshName","box.mesh");
	String buttonName = getAttrib(XMLNode,"fuzzyName");
	
	SceneNode* bNod;
	if (buttonName.empty())
	{
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,rot);
	}
	else
	{
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(buttonName,pos,rot);
	}
	bNod->setScale(scale);
	
	String type = getAttrib(XMLNode,"type","test");
	FuzzyObject* fuzzyObj=NULL;
	if (type=="test")
	{
		fuzzyObj = new FuzzyTest();
	}
	if (type=="test2")
	{
		fuzzyObj = new FuzzyTest2();
	}

	if (fuzzyObj)
	{
	fuzzyObj->init(mSceneMgr);
	fuzzyObj->setup(bNod);
	fuzzyObj->setname(buttonName);
	fobj.push_back(fuzzyObj);
	LogManager::getSingleton().logMessage("!FuzzyObject found");
	}

	if (type=="test2")
	{
		FuzzyTest2* tst = (FuzzyTest2*)fuzzyObj;
		pElement = XMLNode->FirstChildElement("object");
		while(pElement)
		{
			TiXmlElement *pElement1;
			Quaternion rot;
			pElement1 = pElement->FirstChildElement("entity");
			Entity* ent = mSceneMgr->createEntity(getAttrib(pElement1,"name",""),getAttrib(pElement1,"meshFile","box.mesh"));
			LogManager::getSingleton().logMessage("1");
			pElement1 = pElement->FirstChildElement("scale");
			Vector3 scale = parseVector3(pElement1);
			
			LogManager::getSingleton().logMessage("122");
			Real pscale=1;

			pElement1 = pElement->FirstChildElement("pposition");
			if (pElement1)
			pscale = getAttribReal(pElement1,"scale",1.0f);
			
			pElement1 = pElement->FirstChildElement("rotation");
			if (pElement1)
			rot = parseQuaternion(pElement1);
			pElement1 = pElement->FirstChildElement("position");
			if (ent)
				tst->parseMotor(ent, parseVector3(pElement1), pos, scale, pscale, rot);
			else
				LogManager::getSingleton().logMessage("FuzzyObj: error adding a detail object");

			LogManager::getSingleton().logMessage("FuzzyObj: finished objct");
			pElement = pElement->NextSiblingElement("object");
		}
		tst->rotateBody(Quaternion(Radian(0.0),Vector3::UNIT_X));
		tst->rotateBody(Quaternion(Radian(0.6),Vector3::UNIT_Z));

		tst->setRegType(StringConverter::parseInt(getAttrib(XMLNode,"reg_type","0")));
		tst->setupNamedPipeControl();
	}
}

void Sequence::processLadder(TiXmlElement *XMLNode)
{
	Ladder* butt = new Ladder();
	TiXmlElement* pElement = XMLNode->FirstChildElement("position");
	Vector3 pos;
	Quaternion rot;
	Vector3 scale=Vector3(1,1,1); 
	if (pElement)
		pos = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("rotation");
	if (pElement)
		rot = parseQuaternion(pElement);
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)	
		scale = parseVector3(pElement);
	String fileName = getAttrib(XMLNode,"meshName","box.mesh");
	String objName = getAttrib(XMLNode,"name","");
	SceneNode* bNod;
	if (objName.empty())
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,rot);
	else
		bNod = mSceneMgr->getRootSceneNode()->createChildSceneNode(objName,pos,rot);
	bNod->setScale(scale);
	Entity* ent = mSceneMgr->createEntity(bNod->getName(),fileName);
	String materialFile = getAttrib(XMLNode, "materialFile");

	if (!materialFile.empty())
	ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ

	bNod->attachObject(ent);
	butt->init(bNod,0);
	
	ladders.push_back(butt);
	LogManager::getSingleton().logMessage("!Ladder found");
}

void Sequence::processNPC(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	Real farFindDist;
	Vector3 pos;
	Quaternion rot;
	Vector3 scale;
	pElement = XMLNode->FirstChildElement("position");
	if (pElement)
	pos = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("rotate");
	if (pElement)
	rot = parseQuaternion(pElement);
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)
	scale = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("farFind");
	if (pElement)
	farFindDist = getAttribReal(pElement,"dist",1000);
	String meshFile = getAttrib(XMLNode,"meshFile","ninja.mesh");
	String name  = getAttrib(XMLNode,"name","unnamed");
	String className  = getAttrib(XMLNode,"className","npc_enemy");
	String cNearScript = getAttrib(XMLNode,"cNearScript","");
	String scriptOnDeath  = getAttrib(XMLNode,"scriptOnDeath","");
	String materialFile = getAttrib(XMLNode,"materialFile","");
	Real health = getAttribReal(XMLNode,"health",30);
	Real velocity = getAttribReal(XMLNode,"velocity",1);
	bool stopAtDist = getAttribBool(XMLNode,"stopAtDist");
	bool animated = getAttribBool(XMLNode,"animated",true);
	bool applyGravity = getAttribBool(XMLNode,"applyGravity",true);
	Real stopDist = getAttribReal(XMLNode,"stopDist",1);
	Real yShift = getAttribReal(XMLNode,"yShift",0);
	Real ascendFromGround = getAttribReal(XMLNode,"ascendFromGround",0);
	Real rotateSpeed = getAttribReal(XMLNode,"rotateSpeed",50.0f);
	bool landed = getAttribBool(XMLNode,"landed",true);
	Vector3 physposit;
	Vector3 physsize=Vector3(1,1,1);
	Vector3 axis;
	Real angle;
	pElement = XMLNode->FirstChildElement("physPosit");
	if (pElement)
		physposit=parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("physSize");
	if (pElement)
		physsize=parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("axis");
	if (pElement)
		axis=parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("angle");
	if (pElement)
		angle=getAttribReal(pElement,"f",0.0f);

	String soundAttack="run3/sounds/attack_growl3.wav";
	String soundHit="run3/sounds/iceaxe_swing1.wav";
	String soundFind="run3/sounds/attack_growl3.wav";
	String soundRandom1="run3/sounds/breathing3.wav";
	String soundSubRandom21="run3/sounds/cough1.wav";
	String soundSubRandom22="run3/sounds/cough1.wav";
	String soundSubRandom23="run3/sounds/cough1.wav";
	String soundFootstepPrefix="foot";
	
	pElement = XMLNode->FirstChildElement("sounds");
	if (pElement)
	{
		soundAttack=getAttrib(pElement,"soundAttack","run3/sounds/attack_growl3.wav");
		soundHit=getAttrib(pElement,"soundHit","run3/sounds/iceaxe_swing1.wav");
		soundFind=getAttrib(pElement,"soundFind","run3/sounds/attack_growl3.wav");
		soundRandom1=getAttrib(pElement,"soundRandom1","run3/sounds/breathing3.wav");
		soundSubRandom21=getAttrib(pElement,"soundSubRandom21","run3/sounds/cough1.wav");
		soundSubRandom22=getAttrib(pElement,"soundSubRandom22","run3/sounds/cough1.wav");
		soundSubRandom23=getAttrib(pElement,"soundSubRandom23","run3/sounds/cough1.wav");
		soundFootstepPrefix=getAttrib(pElement,"soundFootstepPrefix","foot");
	}


	NPCSpawnProps prop;
	prop.cNearScript=cNearScript;
	prop.soundAttack=soundAttack;
	prop.soundHit=soundHit;
	prop.soundFind=soundFind;
	prop.soundRandom1=soundRandom1;
	prop.rotateSpeed=rotateSpeed;
	prop.materialName=materialFile;
	prop.soundSubRandom21=soundSubRandom21;
	prop.soundSubRandom22=soundSubRandom22;
	prop.soundSubRandom23=soundSubRandom23;
	prop.soundFootstepPrefix=soundFootstepPrefix;
	prop.renderDist=getAttribReal(XMLNode,"renderDist",10000.0f);
	prop.farFind=farFindDist;
	prop.health=health;
	prop.mesh=meshFile;
	prop.yShift=yShift;
	prop.luaOnDeath=scriptOnDeath;
	prop.mName=name;
	prop.animated=animated;
	prop.rot=rot;
	prop.spCPos=pos;
	prop.scale=scale;
	prop.spCName=className;
	prop.velocity=velocity;
	prop.applyGravity=applyGravity;
	prop.stopAtDist=stopAtDist;
	prop.stopDist=stopDist;
	prop.physPosit=physposit;
	prop.physSize=physsize;
	prop.angle=angle;
	prop.ax=axis;
	prop.landed=landed;
	prop.ascendFromGround=ascendFromGround;
	prop.ragdoll=getAttribBool(XMLNode,"ragdoll",false);
	prop.facial_animation=getAttribBool(XMLNode,"fac_anim",false);
	prop.stupidsounds=getAttribBool(XMLNode,"sounds",true);
	prop.sounds=getAttribBool(XMLNode,"sounds2",true);
	prop.attackAnimDist=getAttribReal(XMLNode,"attackAnimDist",130.0f);
	prop.headshot=getAttribBool(XMLNode,"headshot",false);
	prop.headshotDist=getAttribReal(XMLNode,"headshotDist",20.0f);
	prop.takeOffBypass=getAttribReal(XMLNode,"takeOffBypass",2.0f);
	prop.headMesh=getAttrib(XMLNode,"headMesh","marazm01_head.mesh");
	prop.headBone=getAttrib(XMLNode,"headBone","BMan0002-Head");
	prop.handBone=getAttrib(XMLNode,"handBone","Hand");
	prop.headAxis=StringConverter::parseVector3(getAttrib(XMLNode,"headAxis","0 1 0"));
	prop.luaOnReach=getAttrib(XMLNode,"scriptOnReach","");
	prop.exploding =getAttribBool(XMLNode,"exploding",false);
	prop.strange_look =getAttribBool(XMLNode,"strange_look",false);
	LogManager::getSingleton().logMessage("Sequence: npc passed, all data was got;ready for spawn");
	NPCManager::getSingleton().npc_spawn(prop);
}

void Sequence::processNPCGroup(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	TiXmlElement *pElement1;
//	Real farFindDist;
	Vector3 pos;
	Quaternion rot;
	Vector3 scale;
	pElement1=XMLNode->FirstChildElement("npc");
	//AIElementGroupLeaderShip* lship = new AIElementGroupLeaderShip();
	
	String name  = getAttrib(XMLNode,"name","unnamed");
	int type = StringConverter::parseInt(getAttrib(XMLNode,"type","0"));
	AIElementGroup* lship = grpgen->createGroup(type);
	lship->setName(name);
	while (pElement1)
	{
	pElement = pElement1->FirstChildElement("position");
	if (pElement)
	pos = parseVector3(pElement);
	
	pElement = pElement1->FirstChildElement("rotate");
	if (pElement)
	rot = parseQuaternion(pElement);
	pElement = pElement1->FirstChildElement("scale");
	if (pElement)
	scale = parseVector3(pElement);

	name  = getAttrib(pElement1,"name","unnamed");
	AIElement* node=new AIElement(mSceneMgr,pos,name);
	int type = StringConverter::parseInt(getAttrib(pElement1,"type","0"));
	String className  = getAttrib(pElement1,"className","npc_enemy");
	pElement1 = pElement1->NextSiblingElement("npc");
	lship->addAIElement(node);
	}
	AIManager::getSingleton().addElementGroup(lship);
	LogManager::getSingleton().logMessage("Sequence: npc group passed, all data was got;ready for spawn");
}

void Sequence::processLensFlare(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	Real scale = getAttribReal(XMLNode,"scale",200);
	LensFlare* lensflare = new LensFlare(pos,global::getSingleton().getCamera(), mSceneMgr,scale);
	flares.push_back(lensflare);
}


void Sequence::processCutScene(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	
	Real length = getAttribReal(XMLNode,"length",20.0f);
	Real scalerAnim = getAttribReal(XMLNode,"skipAnimSpeed",10.0f);
	bool freeze = getAttribBool(XMLNode,"freezeb",true);
	bool unfreeze = getAttribBool(XMLNode,"unfreezea",false);
	bool splineanim = getAttribBool(XMLNode,"splineAnims",false);
	bool hidehud = getAttribBool(XMLNode,"hideHUD",true);
	bool infin = getAttribBool(XMLNode,"inf",false);
	bool music = getAttribBool(XMLNode,"music",false);
	bool mNoDestroy = !getAttribBool(XMLNode,"destroy",true);
	bool mFromCam = getAttribBool(XMLNode,"fromcam",false);
	String name = getAttrib(XMLNode,"name","undefined"); 
	CutScene* cscene = new CutScene();
	cscene->assign(pos,length,name,freeze,unfreeze,splineanim,hidehud);
	cscene->setInf(infin);
	cscene->setNoDestroy(mNoDestroy);
	if(music)
	{
		cscene->setMusicSync(getAttrib(XMLNode,"musicFile",""),getAttribReal(XMLNode,"musicLength",0.0f));
	}
	cscene->setFasterCoefficient(scalerAnim);
	//cscene->setFUnF(unfreeze);
	Vector3 deltaPos = Vector3::ZERO;
	Quaternion deltaQuat = Quaternion::IDENTITY;

	if (mFromCam)
	{
		Ogre::Camera* cam = global::getSingleton().getCamera();
		deltaPos = cam->getDerivedPosition();
		deltaQuat = cam->getDerivedOrientation();
	}


	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("frame");
	while(pElement)
	{
		
		bool or = getAttribBool(pElement,"or",false);
		bool p = getAttribBool(pElement,"p",false);
		Quaternion quat;
		Ogre::Vector3 p2;
		if (!or)
		{
		quat = parseQuaternion(pElement);
		}
		else
			quat = StringConverter::parseQuaternion(getAttrib(pElement,"orient","1 0 0 0"));

		if (!p)
		{
		p2 = parseVector3(pElement);
		}
		else
			p2 = StringConverter::parseVector3(getAttrib(pElement,"pos","1 0 0 0"));
		Real l = getAttribReal(pElement ,"second",1.0f);
		Real mul = getAttribReal(pElement ,"mul2",1.0f);


		cscene->addFrame(deltaQuat*p2*mul+deltaPos,l,deltaQuat*quat);
		pElement = pElement->NextSiblingElement("frame");
	}
	cutscenes.push_back(cscene);
	LogManager::getSingleton().logMessage("!Cutscene found");
	//triggers	
}

void Sequence::processSeqScript(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	
	Real length = getAttribReal(XMLNode,"length",20.0f);
	bool infin = getAttribBool(XMLNode,"inf",false);
	String name = getAttrib(XMLNode,"name","undefined"); 

	SeqScript* cscene = new SeqScript();
	cscene->assign(Vector3(0,0,0),length,name,false,false,false,false);
	cscene->setInf(infin);
	seqscripts.push_back(cscene);
	LogManager::getSingleton().logMessage("!SeqScript found");
	//triggers	
}

void Sequence::processDarkZone(TiXmlElement *XMLNode)
{
	TiXmlElement* pElement = XMLNode->FirstChildElement("position");
	Vector3 pos;
	Quaternion rot;
	Vector3 scale=Vector3(1,1,1); 
	if (pElement)
		pos = parseVector3(pElement);

	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)	
		scale = parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("light");
	
	String name = getAttrib(XMLNode,"name","undefined"); 
	Real darken=getAttribReal(XMLNode,"darken",0.5f);
	Real exp=getAttribReal(XMLNode,"exp",0.1f);
	DarkZone* dzone = new DarkZone(mSceneMgr,pos,scale,darken,exp);
	while (pElement)
	{
		String name1 = getAttrib(pElement,"name","undefined"); 
		if (mSceneMgr->hasLight(name1))
			dzone->addLight(mSceneMgr->getLight(name1));
		pElement->NextSiblingElement("light");
	}
	dzone->mName=name;
	darkzones.push_back(dzone);
	LogManager::getSingleton().logMessage("!Dark Zone found");
	//triggers	
}

void Sequence::processEvent(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","undefined");
	TiXmlElement *pElement;
	Event* event = new Event(mRoot,true,name);
	// <entc>
	pElement = XMLNode->FirstChildElement("entc");
	while (pElement)
	{
				String ent_name = getAttrib(pElement,"name","undefined");
				String meshFile = getAttrib(pElement,"meshFile","box.mesh");
				Real secs = getAttribReal(pElement,"secs",0);
				Vector3 pos;
				TiXmlElement *pPosition;
				pPosition = pElement->FirstChildElement("pos");	
				if (pPosition)
				{
					pos = parseVector3(pPosition);
				}
				event->setSpawnPhys(ent_name,meshFile,pos,secs);

				pElement = pElement->NextSiblingElement("entc");	
	}
	// <player>
	pElement = XMLNode->FirstChildElement("player");
	if (pElement)
	{
				String ent_event = getAttrib(pElement,"event","teleport");
				Real secs = getAttribReal(pElement,"secs",0);
				Vector3 pos;
							TiXmlElement *pPosition;
							pPosition = pElement->FirstChildElement("pos");	
							if (pPosition)
							{
								 pos = parseVector3(pPosition);
								//global::getSingleton().entc_name = ent_name;
							}
				event->setPlayer(pos,secs,ent_event);
	}
	// <changelevel>
	pElement = XMLNode->FirstChildElement("changelevel");
	if (pElement)
	{
				String ent_event = getAttrib(pElement,"map","techdemo_01");
				event->setChangelevel(ent_event);
	}
	// <door>
	pElement = XMLNode->FirstChildElement("door");
	while(pElement)
	{
				String door_name = getAttrib(pElement,"name","undefined");
				String ent_event = getAttrib(pElement,"event","open");
				Real secs = getAttribReal(pElement,"secs",0);
				int i;
				func_door* door;
				for (i=0;i!=doors.size();i++)
				{
					if (doors[i]->getname()==door_name)
					{
						door = doors[i];
					}
				}
				event->setOpenDoor(door,ent_event,secs);
				pElement = pElement->NextSiblingElement("door");
	}
	pElement = XMLNode->FirstChildElement("lua");
	while(pElement)
	{
		String luascr = getAttrib(pElement,"script","run3/lua/empty.lua");
		Real secs = getAttribReal(pElement,"secs",0);
		event->addLuaScript2(luascr,secs);
		pElement = pElement->NextSiblingElement("lua");
	}
	// <cutscene>
	pElement = XMLNode->FirstChildElement("cutscene");
	while(pElement)
	{
				String cscene_name = getAttrib(pElement,"name","undefined");
				//String ent_event = getAttrib(pElement,"event","open");
				//Real secs = getAttribReal(pElement,"secs",0);
				
				int i;
				CutScene* cscene;
				for (i=0;i!=cutscenes.size();i++)
				{
					if (cutscenes[i]->getname()==cscene_name)
					{
						cscene = cutscenes[i];
					}
				}
				event->setStartCutScene(cscene);
				pElement = pElement->NextSiblingElement("cutscene");
	}

	s_events.push_back(event);
}

void Sequence::processCutSceneEvent(TiXmlElement *XMLNode)
{
	LogManager::getSingleton().logMessage("(Cutscene)Event processing. ");
	String name = getAttrib(XMLNode,"name","undefined");
	Real wait = getAttribReal(XMLNode,"wait",0);
	CutScene* cscene;
	int i;
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("run");
	for (i=0; i!=cutscenes.size();i++)
	{
		if (cutscenes[i]->getname() == name)
		{
			cscene = cutscenes[i];
		}
	}
	if (cscene)
	{
	if (wait==0)
	{
		cscene->start();
	}
	while (pElement)
	{
		String script = getAttrib(pElement,"script","");
		Real sec = getAttribReal(pElement,"sec",0);
		
		LogManager::getSingleton().logMessage("(Cutscene)Lua run script processing.");
		if (!script.empty())
		{
			cscene->addLuaRun(sec,script);
		}
		pElement = pElement->NextSiblingElement("run");
	}
	if (wait!=-1)
	cscene->setWait(wait);
	}
}


void Sequence::processSeqScriptEvent(TiXmlElement *XMLNode)
{
	LogManager::getSingleton().logMessage("(Cutscene)Event processing. ");
	String name = getAttrib(XMLNode,"name","undefined");
	Real wait = getAttribReal(XMLNode,"wait",0);
	SeqScript* cscene;
	int i;
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("run");
	for (i=0; i!=seqscripts.size();i++)
	{
		if (seqscripts[i]->getname() == name)
		{
			cscene = seqscripts[i];
		}
	}
	if (cscene)
	{
	if (wait==0)
	{
		cscene->start();
	}
	while (pElement)
	{
		String script = getAttrib(pElement,"script","");
		Real sec = getAttribReal(pElement,"sec",0);
		
		LogManager::getSingleton().logMessage("(SeqScript)Lua run script processing.");
		if (!script.empty())
		{
			cscene->addLuaRun(sec,script);
		}
		pElement = pElement->NextSiblingElement("run");
	}
	if (wait!=-1)
	cscene->setWait(wait);
	}
}

void Sequence::processPickup(TiXmlElement *XMLNode)
{
	#ifdef SEQUENCE_DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("SEQUENCE DEBUG: Pickup data found");
	#endif
	Pickup* p =new Pickup();
	String fileName = getAttrib(XMLNode,"mesh","box.mesh");
	String event = getAttrib(XMLNode,"event");
	String object = getAttrib(XMLNode,"object");
	int value = StringConverter::parseUnsignedInt(getAttrib(XMLNode,"value"));
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX",1),getAttribReal(XMLNode,"sY",1),getAttribReal(XMLNode,"sZ",1));
	Quaternion quat = parseQuaternion(XMLNode);
	p->init(event,object,pos,quat,scale,fileName,value);
	pickups.push_back(p);
		#ifdef SEQUENCE_DEBUG_AT_RELEASE
  LogManager::getSingleton().logMessage("SEQUENCE DEBUG: Sucessfully added!");
	#endif
}

void Sequence::processDoor(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX",1),getAttribReal(XMLNode,"sY",1),getAttribReal(XMLNode,"sZ",1));
	Quaternion quat = parseQuaternion(XMLNode);
	String name = getAttrib(XMLNode,"name","");
	String fileName = getAttrib(XMLNode,"mesh","box.mesh");
	String ParentName = getAttrib(XMLNode,"parent","");
	String openSound = getAttrib(XMLNode,"openSound","nosound");
	String closeSound = getAttrib(XMLNode,"closeSound","nosound");
	Real dist = getAttribReal(XMLNode,"distance",90);
	Real speed = getAttribReal(XMLNode,"speed",10);
	bool rotating = getAttribBool(XMLNode,"rotating",false);
	Real angle = getAttribReal(XMLNode,"angle",0.0f);
	Vector3 dir = Vector3(getAttribReal(XMLNode,"dirX",0),getAttribReal(XMLNode,"dirY",0),getAttribReal(XMLNode,"dirZ",1));
	Vector3 axis = Vector3(getAttribReal(XMLNode,"aX",1),getAttribReal(XMLNode,"aY",0),getAttribReal(XMLNode,"aZ",0));
	LogManager::getSingleton().logMessage("Sequence: processing door "+ name);
	func_door* door = new func_door();
	door->init(mSceneMgr);
	door->allowPRegulator(getAttribBool(XMLNode,"allowPReg",false));
	door->setDistanceDirection(dist,dir,speed);
	door->setSounds(openSound,closeSound);
	door->setUseInteract(getAttribBool(XMLNode,"useInteract",false));
	door->setLuaScripts(getAttrib(XMLNode,"lOnOpen"),getAttrib(XMLNode,"lOnClosed"),getAttrib(XMLNode,"lOnStarted"));
	door->setLockedLua(getAttrib(XMLNode,"lLocked"));
	SceneNode* doorn;

	if (!ParentName.empty())
	{
		if (!(mSceneMgr->hasSceneNode(ParentName)))
		{
			ParentName="";
		}
		else
		{
				doorn = mSceneMgr->getSceneNode(ParentName);
				if (doorn)
				{
					if (!name.empty())
					{
						doorn = doorn->createChildSceneNode(name,pos,quat );
					}
					else
					{
						doorn = doorn->createChildSceneNode(pos,quat );
					}
				}
		}
	}
	if (ParentName.empty())
	{
		if (!name.empty())
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat );
		}
		else
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,quat );
		}
	}


	/*if (!name.empty())
	{
		doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat );
	}
	else
	{
		doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,quat );
	}*/



	doorn->setScale(scale);
	Entity* ent = mSceneMgr->createEntity(name,fileName);
	String materialFile = getAttrib(XMLNode, "materialFile");
	if (!materialFile.empty())
	ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ
	doorn->attachObject(ent);
	//if (getAttribBool(XMLNode,"box",false))
	door->setup(doorn,getAttribBool(XMLNode,"box",false));

	door->setname(name);
	if (rotating)
	{
		door->setRotateDoor(getAttribReal(XMLNode,"pitch"),getAttribReal(XMLNode,"yaw"),getAttribReal(XMLNode,"roll"),getAttribReal(XMLNode,"rotspeed"));
	}
	doors.push_back(door);
	//doorn->
	//door->
}

void Sequence::processRot(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX",1),getAttribReal(XMLNode,"sY",1),getAttribReal(XMLNode,"sZ",1));
	Quaternion quat = parseQuaternion(XMLNode);
	String name = getAttrib(XMLNode,"name","");
	String ParentName = getAttrib(XMLNode,"parent","");
	String fileName = getAttrib(XMLNode,"mesh","box.mesh");
	String openSound = getAttrib(XMLNode,"openSound","nosound");
	String closeSound = getAttrib(XMLNode,"closeSound","nosound");
	Real dist = getAttribReal(XMLNode,"distance",90);
	Real speed = getAttribReal(XMLNode,"speed",10);
	bool rotating = getAttribBool(XMLNode,"rotating",false);
	Real angle = getAttribReal(XMLNode,"angle",0.0f);
	Vector3 dir = Vector3(getAttribReal(XMLNode,"dirX",0),getAttribReal(XMLNode,"dirY",0),getAttribReal(XMLNode,"dirZ",1));
	Vector3 axis = Vector3(getAttribReal(XMLNode,"aX",1),getAttribReal(XMLNode,"aY",0),getAttribReal(XMLNode,"aZ",0));
	func_door* door = new func_door();
	door->init(mSceneMgr);
	door->setDistanceDirection(dist,dir,speed);
	door->setSounds(openSound,closeSound);
	door->setUseInteract(getAttribBool(XMLNode,"useInteract",false));
	door->setLuaScripts(getAttrib(XMLNode,"lOnOpen"),getAttrib(XMLNode,"lOnClosed"),getAttrib(XMLNode,"lOnStarted"));
	door->transformToFuncRot();
	SceneNode* doorn;
	
	if (!ParentName.empty())
	{
		if (!(mSceneMgr->hasSceneNode(ParentName)))
		{
			ParentName="";
		}
		else
		{
				doorn = mSceneMgr->getSceneNode(ParentName);
				if (doorn)
				{
					if (!name.empty())
					{
						doorn = doorn->createChildSceneNode(name,pos,quat );
					}
					else
					{
						doorn = doorn->createChildSceneNode(pos,quat );
					}
				}
		}
	}
	if (ParentName.empty())
	{
		if (!name.empty())
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat );
		}
		else
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,quat );
		}
	}
	doorn->setScale(scale);
	Entity* ent = mSceneMgr->createEntity(name,fileName);
	String materialFile = getAttrib(XMLNode, "materialFile");
	if (!materialFile.empty())
	ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ
	doorn->attachObject(ent);
	door->setup(doorn);
	door->setname(name);
	if (rotating)
	{
		door->setRotateDoor(getAttribReal(XMLNode,"pitch"),getAttribReal(XMLNode,"yaw"),getAttribReal(XMLNode,"roll"),getAttribReal(XMLNode,"rotspeed"));
	}
	doors.push_back(door);
	//doorn->
	//door->
}


void Sequence::processPendulum(TiXmlElement *XMLNode)
{
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX",1),getAttribReal(XMLNode,"sY",1),getAttribReal(XMLNode,"sZ",1));
	Quaternion quat = parseQuaternion(XMLNode);
	String name = getAttrib(XMLNode,"name","");
	String fileName = getAttrib(XMLNode,"mesh","box.mesh");
	String openSound = getAttrib(XMLNode,"openSound","nosound");
	String closeSound = getAttrib(XMLNode,"closeSound","nosound");
	String ParentName = getAttrib(XMLNode,"parent","");
	Real dist = getAttribReal(XMLNode,"distance",90);
	Real speed = getAttribReal(XMLNode,"speed",10);
	bool rotating = getAttribBool(XMLNode,"rotating",false);
	bool positionPend = getAttribBool(XMLNode,"positionPend",false);
	Real angle = getAttribReal(XMLNode,"angle",0.0f);
	Vector3 dir = Vector3(getAttribReal(XMLNode,"dirX",0),getAttribReal(XMLNode,"dirY",0),getAttribReal(XMLNode,"dirZ",1));
	Vector3 axis = Vector3(getAttribReal(XMLNode,"aX",1),getAttribReal(XMLNode,"aY",0),getAttribReal(XMLNode,"aZ",0));
	Pendulum* door = new Pendulum();
	door->init(mSceneMgr);
	//door->setDistanceDirection(dist,dir,speed);
	//door->setSounds(openSound,closeSound);
	door->setUseInteract(getAttribBool(XMLNode,"useInteract",false));
	//door->setLuaScripts(getAttrib(XMLNode,"lOnOpen"),getAttrib(XMLNode,"lOnClosed"),getAttrib(XMLNode,"lOnStarted"));
	door->transformToFuncRot();
	SceneNode* doorn;
	/*if (!name.empty())
	{
		doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat );
	}
	else
	{
		doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,quat );
	}*/

	if (!ParentName.empty())
	{
		if (!(mSceneMgr->hasSceneNode(ParentName)))
		{
			ParentName="";
		}
		else
		{
				doorn = mSceneMgr->getSceneNode(ParentName);
				if (doorn)
				{
					if (!name.empty())
					{
						doorn = doorn->createChildSceneNode(name,pos,quat );
					}
					else
					{
						doorn = doorn->createChildSceneNode(pos,quat );
					}
				}
		}
	}
	if (ParentName.empty())
	{
		if (!name.empty())
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat );
		}
		else
		{
			doorn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos,quat );
		}
	}


	doorn->setScale(scale);
	Entity* ent = mSceneMgr->createEntity(name,fileName);
	String materialFile = getAttrib(XMLNode, "materialFile");
	if (!materialFile.empty())
	ent->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ

	doorn->attachObject(ent);
	door->setup(doorn);
	door->setname(name);

	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("psys");
	if (pElement)
	{
		TiXmlElement *pElement1;
		String partName = getAttrib(pElement,"psysName","");
		//Quaternion rot;
		pElement1 = pElement->FirstChildElement("position");
		Vector3 pos = parseVector3(pElement1);
		//Entity* ent = mSceneMgr->createEntity(getAttrib(pElement1,"name",""),getAttrib(pElement1,"meshFile","box.mesh"));
		LogManager::getSingleton().logMessage("1");
		pElement1 = pElement->FirstChildElement("scale");
		Vector3 scale = parseVector3(pElement1);
		door->addParticleSystem(partName,pos,scale);
	}
	//door->Fire("toggle");
	if (rotating)
	{
		if (!positionPend)
		{
			door->setRotateDoor(getAttribReal(XMLNode,"pitch"),getAttribReal(XMLNode,"yaw"),getAttribReal(XMLNode,"roll"),getAttribReal(XMLNode,"rotspeed"));
		}
		else
		{
			door->setRotateDoor(getAttribReal(XMLNode,"pitch"),getAttribReal(XMLNode,"yaw"),getAttribReal(XMLNode,"roll"),getAttribReal(XMLNode,"rotspeed"),dir);
		}
	}
	pends.push_back(door);
	door->Fire("toggle");
	//doorn->
	//door->
	
}

void Sequence::processComputer(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","undefined");
	String mesh = getAttrib(XMLNode,"meshFile","pcomputer_01.mesh");
	String script = getAttrib(XMLNode,"script","run3/lua/c64.lua");
	String cShutScript = getAttrib(XMLNode,"cShutScript","");
	String cNearScript = getAttrib(XMLNode,"cNearScript","");
	String dispMat = getAttrib(XMLNode,"dispMat","BLACK");
	bool allowVirtualDisplay = getAttribBool(XMLNode,"allowVirtualDisplay",true);
	Vector3 pos;
	Vector3 scale = Vector3(1,1,1);
	Quaternion quat;
	TiXmlElement *pElement = XMLNode->FirstChildElement("position");
	if (pElement)
	{
		pos = parseVector3(pElement);
	}
	pElement = XMLNode->FirstChildElement("scale");
	if (pElement)
	{
		scale = parseVector3(pElement);
	}
	pElement = XMLNode->FirstChildElement("rotate");
	if (pElement)
	{
		quat =parseQuaternion(pElement);
	}	
	
	SceneNode* cComp = mSceneMgr->getRootSceneNode()->createChildSceneNode(name,pos,quat);
	Entity* eComp = mSceneMgr->createEntity(name,mesh);
	String materialFile = getAttrib(XMLNode, "materialFile");
	if (!materialFile.empty())
	eComp->getSubEntity(0)->setMaterial(MaterialManager::getSingleton().getByName(materialFile)); //SKINZ
	cComp->attachObject(eComp);
	cComp->setScale(scale);
	Computer* comp = new Computer();
	comp->setName(name);
	comp->init(cComp,-1,script,dispMat);
	comp->setShutdownScript(cShutScript);
	comp->setAllowVirtualDisplay(allowVirtualDisplay);
	comp->setNearScript(cNearScript);
	if (getAttribBool(XMLNode,"activateMagic"))
		comp->activateMagic(true);
	computers.push_back(comp);
	LogManager::getSingleton().logMessage("comp"+StringConverter::toString(cComp->getPosition()));
}

void Sequence::init_reload()
{
#define CLEAR_T1(v) for (i=0;i!=v.size();i++) \
	{ \
		v[i]->dispose(); \
	} v.clear();


#define CLEAR_T2(v) for (i=0;i!=v.size();i++) \
	{ \
		v[i]->cleanup(); \
	} v.clear();

#define CLEAR_T3(t,v) vector<t*>::iterator p;for (p=v.begin();p!=v.end();p++) \
	{ \
		(*p)->cleanup(); \
		delete (*p); \
	} v.clear();

#define CLEAR_T4(t,v) ;for (vector<t*>::iterator m=v.begin();m!=v.end();m++) \
	{ \
		(*m)->dispose(); \
		delete (*m); \
	} v.clear();

	int i;
	//triggers
	for (i=0;i!=s_events.size();i++)
	{
		s_events[i]->dispose();
	} 
	s_events.clear();

	std::vector<Fire*>::iterator i4;
	for (i4=fires.begin();i4!=fires.end();i4++)
	{
				(*i4)->destroy();
				delete (*i4);
	}		
	fires.clear();

	for (i=0;i!=triggers.size();i++)
	{
		triggers[i]->unload();
	}
	triggers.clear();

	for (i=0;i!=buttons.size();i++)
	{
		buttons[i]->dispose();
	} 
	buttons.clear();

	for (i=0;i!=ladders.size();i++)
	{
		ladders[i]->dispose();
	} 
	ladders.clear();
	//doors
	for (i=0;i!=doors.size();i++)
	{
		doors[i]->unload();
	}
	doors.clear();

	for (vector<FuzzyObject*>::iterator m=fobj.begin();m!=fobj.end();m++)
	{
		(*m)->unload();
		delete (*m);
	}
	fobj.clear();

	for (i=0;i!=pends.size();i++)
	{
		pends[i]->unload();
	}
	pends.clear();
	//cutscenes
	for (i=0;i!=cutscenes.size();i++)
	{
		cutscenes[i]->dispose();
	} 
	cutscenes.clear();

	for (i=0;i!=computers.size();i++)
	{
		computers[i]->logoff();
		computers[i]->del();
	} 
	computers.clear();

	for (i=0;i!=pickups.size();i++)
	{
		pickups[i]->dispose();
		
	} 
	pickups.clear();

	vector<LensFlare*>::iterator k;
	for (k=flares.begin();k!=flares.end();k++)
	{
		LensFlare* l = *k;
		delete l;
	}

	vector<Run3Timer*>::iterator k1;
	for (k1=timers.begin();k1!=timers.end();k1++)
	{
		Run3Timer* l = *k1;
		delete l;
	}
	timers.clear();

	flares.clear();
	darkzones.clear();
CLEAR_T3(Train,trains)
//CLEAR_T4(SeqScript,seqscripts)
	NPCManager::getSingleton().npcs_flush();
	AIManager::getSingleton().cleanupElementGroups();
}

void Sequence::processTriggerEvent(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","undefined");
	Real secs = getAttribReal(XMLNode,"sec",0.0f);
	Trigger* tTrig;
	int i;

	for (i=0; i!=triggers.size();i++)
	{
		if (triggers[i]->getname() == name)
		{
			tTrig = triggers[i];
		}
	}

	if (tTrig)
	{
			tTrig->setsleep(secs);
			TiXmlElement *pElement;

			pElement = XMLNode->FirstChildElement("entc");
			while(pElement)
			{
				String ent_name = getAttrib(pElement,"name","undefined");
				String ent_event = getAttrib(pElement,"event","spawn");
				String meshFile = getAttrib(pElement,"meshFile","box.mesh");
				Vector3 pos;
				/*global::getSingleton().entc_event = ent_event;
				global::getSingleton().entc_meshFile = meshFile;
				global::getSingleton().entc_name = ent_name;*/
				/*tTrig->entc_event = ent_event;
				tTrig->entc_meshFile = meshFile;
				tTrig->entc_name = ent_name;*/
							TiXmlElement *pPosition;
							pPosition = pElement->FirstChildElement("pos");	
							if (pPosition)
							{
								 pos = parseVector3(pPosition);
								//global::getSingleton().entc_name = ent_name;
							}

				tTrig->setCallbackEnt(ent_name,meshFile,ent_event,pos);

				pElement = pElement->NextSiblingElement("entc");
			}
	
			pElement = XMLNode->FirstChildElement("player");
			if (pElement)
			{
				String ent_event = getAttrib(pElement,"event","teleport");
			Vector3 pos;
							TiXmlElement *pPosition;
							pPosition = pElement->FirstChildElement("pos");	
							if (pPosition)
							{
								 pos = parseVector3(pPosition);
								//global::getSingleton().entc_name = ent_name;
							}
				tTrig->setCallbackPlayer(ent_event,pos);
			}

			pElement = XMLNode->FirstChildElement("lua");
			if (pElement)
			{
							String ent_event = getAttrib(pElement,"script","teleport");
							tTrig->setCallbackScript(ent_event);
			}
			
			pElement = XMLNode->FirstChildElement("hurt");
			if (pElement)
			{
							Real ent_event = getAttribReal(pElement,"damage",1.0f);
							tTrig->setHurt(ent_event);
			}

			pElement = XMLNode->FirstChildElement("changelevel");
			if (pElement)
			{
				String ent_event = getAttrib(pElement,"map","techdemo_01");
				tTrig->setCallbackChangeLevel(ent_event);
			}

			pElement = XMLNode->FirstChildElement("door");
			while(pElement)
			{
				String door_name = getAttrib(pElement,"name","undefined");
				String ent_event = getAttrib(pElement,"event","open");
				
				
				int i;
				func_door* door;
				for (i=0;i!=doors.size();i++)
				{
					if (doors[i]->getname()==door_name)
					{
						door = doors[i];
					}
				}
				tTrig->setCallbackDoor(door,ent_event);
				pElement = pElement->NextSiblingElement("door");
			}

	}

}

void Sequence::processEventEntc()
{
	
	//SceneManager* mSceneMgr = global::getSingleton().getSceneManager();
}

void Sequence::interpretate(String obj,String command)
{
	
}

bool Sequence::frameStarted(const Ogre::FrameEvent &evt)
{
	if (!rest)
	{
	char a;
	if (global::getSingleton().port)
	{
	global::getSingleton().port->ReadData(&a,1);
		if (a=='a')
		{
			char d[24];
			
			global::getSingleton().port->ReadData(&d,23);
			String str = String(*&d,23);
			LogManager::getSingleton().logMessage("(Sequence) Lua run:"+str);
			if ((str[0]=='r')&&(str[22]=='a'))
			{
				
			RunLuaScript(pLuaState,str.c_str());
			}
		}
		if (a=='r')
		{
			char d ;
			Real strr=0;
			//global::getSingleton().port->ReadData(&a,1);
			//if (a=='1')
			//{
			//char d[24];
			
			//global::getSingleton().port->ReadData(&d,23);
			//String str = String(*&d,23);
			LogManager::getSingleton().logMessage("(Sequence) X1 Register operation from COM:");
			//global::getSingleton().port->ReadData(&a,1);
			//Real strr = StringConverter::parseInt(str)
			global::getSingleton().port->ReadData(&a,1);
			global::getSingleton().port->ReadData(&d,1);
			while ((d!='n')&&(d>47)&&(d<58))
			{
				strr=strr*10+(d-48);
				global::getSingleton().port->ReadData(&d,1);
			}
			switch(a)
			{
			case '1':
				global::getSingleton().rx1=strr;
				break;
			case '2':
				global::getSingleton().rx2=strr;
				break;
			case '3':
				global::getSingleton().ry1=strr;
				break;
			case '4':
				global::getSingleton().ry2=strr;
				break;
			case '5':
				global::getSingleton().rz1=strr;
				break;
			case '6':
				global::getSingleton().rz2=strr;
				break;
			case '7':
				global::getSingleton().a=strr;
				break;
			case '8':
				global::getSingleton().ra=strr;
				break;
			case '9':
				global::getSingleton().rb=strr;
				break;
			case 'A':
				global::getSingleton().rc=strr;
				break;
			case 'B':
				break;
			default:
				break;
			}
			
			//}

		}
	}
	if (!sequence.empty())
	{
		sec = sec+evt.timeSinceLastFrame;
		

		if (sec>1.0f)
			sec=0;
	}
	vector<PhysObject*>::iterator i;

	for (i=pobjs.begin();i!=pobjs.end();i++)
		(*i)->frameStarted(evt);
	vector<Run3::Button*>::iterator j;
	for (j=buttons.begin();j!=buttons.end();j++)
		(*j)->frameStarted(evt);
	for (unsigned int i=0;i!=triggers.size();i++)
		triggers[i]->frameStarted(evt);
	for (unsigned int i=0;i!=doors.size();i++)
		doors[i]->frameStarted(evt);
	for (unsigned int i=0;i!=fobj.size();i++)
		fobj[i]->frameStarted(evt);
	for (unsigned int i=0;i!=pends.size();i++)
		pends[i]->frameStarted(evt);
	for (unsigned int i=0;i!=computers.size();i++)
		computers[i]->frameStarted(evt);
	for (unsigned int i=0;i!=seqscripts.size();i++)
		seqscripts[i]->frameStarted(evt);
	for (unsigned int i=0;i!=timers.size();i++)
		timers[i]->frameStarted(evt);
	for (unsigned int i=0;i!=flares.size();i++)
		flares[i]->update();


	Vector3 mPlPos = global::getSingleton().getPlayer()->get_location();

	ColourValue ambi = global::getSingleton().ambiLight;
	for (unsigned int i=0;i!=darkzones.size();i++)
		darkzones[i]->applyDarken(ambi,mPlPos);
	mSceneMgr->setAmbientLight(ambi);
	}
return true;
}

bool Sequence::frameEnded(const Ogre::FrameEvent &evt)
{
return true;
}

String Sequence::getAttrib(TiXmlElement *XMLNode, const String &attrib, const String &defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return XMLNode->Attribute(attrib.c_str());
	else
		return defaultValue;
}

Real Sequence::getAttribReal(TiXmlElement *XMLNode, const String &attrib, Real defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return StringConverter::parseReal(XMLNode->Attribute(attrib.c_str()));
	else
		return defaultValue;
}

bool Sequence::getAttribBool(TiXmlElement *XMLNode, const String &attrib, bool defaultValue)
{
	if(!XMLNode->Attribute(attrib.c_str()))
		return defaultValue;

	if(String(XMLNode->Attribute(attrib.c_str())) == "true")
		return true;

	return false;
}


Vector3 Sequence::parseVector3(TiXmlElement *XMLNode)
{
	Quaternion orientation;

	if(XMLNode->Attribute("qxx"))
	{
		orientation.x = StringConverter::parseReal(XMLNode->Attribute("qxx"));
		orientation.y = StringConverter::parseReal(XMLNode->Attribute("qyy"));
		orientation.z = StringConverter::parseReal(XMLNode->Attribute("qzz"));
		orientation.w = StringConverter::parseReal(XMLNode->Attribute("qww"));
	}

	return orientation*Vector3(
		StringConverter::parseReal(XMLNode->Attribute("x"))+getAttribReal(XMLNode,"tx",0.0f),//StringConverter::parseReal(XMLNode->Attribute("tx")),
		StringConverter::parseReal(XMLNode->Attribute("y"))+getAttribReal(XMLNode,"ty",0.0f),//StringConverter::parseReal(XMLNode->Attribute("ty")),
		StringConverter::parseReal(XMLNode->Attribute("z"))+getAttribReal(XMLNode,"tz",0.0f)//StringConverter::parseReal(XMLNode->Attribute("tz"))
	)*getAttribReal(XMLNode,"mul",1.0f);
}

Quaternion Sequence::parseQuaternion(TiXmlElement *XMLNode)
{
	//! @todo Fix this crap!

	Quaternion orientation;

	if(XMLNode->Attribute("qx"))
	{
		orientation.x = StringConverter::parseReal(XMLNode->Attribute("qx"));
		orientation.y = StringConverter::parseReal(XMLNode->Attribute("qy"));
		orientation.z = StringConverter::parseReal(XMLNode->Attribute("qz"));
		orientation.w = StringConverter::parseReal(XMLNode->Attribute("qw"));
	}
	else if(XMLNode->Attribute("axisX"))
	{
		Vector3 axis;
		axis.x = StringConverter::parseReal(XMLNode->Attribute("axisX"));
		axis.y = StringConverter::parseReal(XMLNode->Attribute("axisY"));
		axis.z = StringConverter::parseReal(XMLNode->Attribute("axisZ"));
		Real angle = StringConverter::parseReal(XMLNode->Attribute("angle"));;
		orientation.FromAngleAxis(Ogre::Angle(angle), axis);
	}
	else if(XMLNode->Attribute("angleX"))
	{
		Vector3 axis;
		axis.x = StringConverter::parseReal(XMLNode->Attribute("angleX"));
		axis.y = StringConverter::parseReal(XMLNode->Attribute("angleY"));
		axis.z = StringConverter::parseReal(XMLNode->Attribute("angleZ"));
		//orientation.FromAxes(&axis);
		//orientation.F
	}

	return orientation;
}

ColourValue Sequence::parseColour(TiXmlElement *XMLNode)
{
	return ColourValue(
		StringConverter::parseReal(XMLNode->Attribute("r")),
		StringConverter::parseReal(XMLNode->Attribute("g")),
		StringConverter::parseReal(XMLNode->Attribute("b")),
		XMLNode->Attribute("a") != NULL ? StringConverter::parseReal(XMLNode->Attribute("a")) : 1
	);
}

