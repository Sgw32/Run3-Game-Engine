#include "DotSceneLoader.h"
#include "tinyxml.h"
#include "Sequence.h"
#include "Run3Benchmark.h"

#include <Ogre.h>

using namespace std;
using namespace Ogre;

void DotSceneLoader::parseDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode, const String &sPrependNode,OgreNewt::World *World,Camera* camera,SoundManager* soundMgr,Player* ply)
{
	Ogre::MaterialManager::getSingleton().unloadUnreferencedResources();
	Ogre::TextureManager::getSingleton().unloadUnreferencedResources();
	Ogre::MeshManager::getSingleton().unloadUnreferencedResources();
	Ogre::GpuProgramManager::getSingleton().unloadUnreferencedResources();
	// set up shared object values
	m_sGroupName = groupName;
	mSceneMgr = yourSceneMgr;
	mWorld = World;
	mCamera=camera;
	sound = soundMgr;
	player=ply;
	if (!mWorld)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Physics world doesnt exists!");
	}
	m_sPrependNode = sPrependNode;
	staticObjects.clear();
	dynamicObjects.clear();

	TiXmlDocument   *XMLDoc = 0;
	TiXmlElement   *XMLRoot;

	levelofDetailM=global::getSingleton().getMLod();
	levelofDetailT=global::getSingleton().getTLod();

	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename(SceneName, basename, path);
		DataStreamPtr pStream;
		if (ResourceGroupManager::getSingleton().resourceExists(groupName,basename))
		{
			pStream = ResourceGroupManager::getSingleton().
				openResource( basename, groupName );
			
		}
		else
		{
			path = "[DotSceneLoader] File "+basename;
			LogManager::getSingleton().logMessage(path+" doesn't exist" );
			return;
		}

		//DataStreamPtr pStream = ResourceGroupManager::getSingleton().
		//	openResource( SceneName, groupName );
		
		String data = pStream->getAsString();
		// Open the .scene File
		XMLDoc = new TiXmlDocument();
		XMLDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();

		if( XMLDoc->Error() )
		{
			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[DotSceneLoader] The TiXmlDocument reported an error");
			LogManager::getSingleton().logMessage(String(XMLDoc->ErrorDesc()));
			LogManager::getSingleton().logMessage(StringConverter::toString(XMLDoc->ErrorRow()));
			LogManager::getSingleton().logMessage(StringConverter::toString(XMLDoc->ErrorCol()));
			delete XMLDoc;
			return;
		}
	}
	catch(...)
	{
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error creating TiXmlDocument");
		delete XMLDoc;
		return;
	}

	// Validate the File
	XMLRoot = XMLDoc->RootElement();
	if( String( XMLRoot->Value()) != "scene"  ) {
		LogManager::getSingleton().logMessage( "[DotSceneLoader] Error: Invalid .scene File. Missing <scene>" );
		delete XMLDoc;      
		return;
	}
	BENCHMARK1(0)
	if (XMLRoot->FirstChildElement("integratedSequence"))
		Sequence::getSingleton().SetSceneSeq(XMLRoot->FirstChildElement("integratedSequence"));
	if (StringConverter::parseInt(getAttrib(XMLRoot,"tLod","-1"))!=-1)
		levelofDetailT=StringConverter::parseInt(getAttrib(XMLRoot,"tLod","-1"));
	if (StringConverter::parseInt(getAttrib(XMLRoot,"mLod","-1"))!=-1)
		levelofDetailM=StringConverter::parseInt(getAttrib(XMLRoot,"mLod","-1"));
	if (StringConverter::parseBool(getAttrib(XMLRoot,"glass","false"))==false)
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Run3Glass", false);
	}
	else
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Run3Glass", true);
	}
	if (StringConverter::parseBool(getAttrib(XMLRoot,"glow","false"))==false)
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", false);
		Inventory::getSingleton().setGlowWasActive(false);
	}
	else
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", true);
		Inventory::getSingleton().setGlowWasActive(true);
	}
	BENCHMARK1(1)
	sceneMultiplier=getAttribReal(XMLRoot,"multiplier",1);
	// figure out where to attach any nodes we create
	mAttachNode = pAttachNode;
	if(!mAttachNode)
		mAttachNode = mSceneMgr->getRootSceneNode();

	// Process the scene
	processScene(XMLRoot);

	// Close the XML File
	delete XMLDoc;
}

void DotSceneLoader::mergeDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode, const String &sPrependNode,OgreNewt::World *World,Camera* camera,SoundManager* soundMgr,Player* ply)
{
	// set up shared object values
	m_sGroupName = groupName;
	mSceneMgr = yourSceneMgr;
	mWorld = World;
	mCamera=camera;
	sound = soundMgr;
	player=ply;
	if (!mWorld)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Physics world doesnt exists!");
	}
	m_sPrependNode = sPrependNode;
	

	TiXmlDocument   *XMLDoc = 0;
	TiXmlElement   *XMLRoot;

	levelofDetailM=global::getSingleton().getMLod();
	levelofDetailT=global::getSingleton().getTLod();

	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename(SceneName, basename, path);
		DataStreamPtr pStream;
		if (ResourceGroupManager::getSingleton().resourceExists(groupName,basename))
		{
			pStream = ResourceGroupManager::getSingleton().
				openResource( basename, groupName );
			
		}
		else
		{
			path = "[DotSceneLoader] File "+basename;
			LogManager::getSingleton().logMessage(path+" doesn't exist" );
			return;
		}

		//DataStreamPtr pStream = ResourceGroupManager::getSingleton().
		//	openResource( SceneName, groupName );
		
		String data = pStream->getAsString();
		// Open the .scene File
		XMLDoc = new TiXmlDocument();
		XMLDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();

		if( XMLDoc->Error() )
		{
			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[DotSceneLoader] The TiXmlDocument reported an error");
			LogManager::getSingleton().logMessage(String(XMLDoc->ErrorDesc()));
			LogManager::getSingleton().logMessage(StringConverter::toString(XMLDoc->ErrorRow()));
			LogManager::getSingleton().logMessage(StringConverter::toString(XMLDoc->ErrorCol()));
			delete XMLDoc;
			return;
		}
	}
	catch(...)
	{
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error creating TiXmlDocument");
		delete XMLDoc;
		return;
	}

	// Validate the File
	XMLRoot = XMLDoc->RootElement();
	if( String( XMLRoot->Value()) != "scene"  ) {
		LogManager::getSingleton().logMessage( "[DotSceneLoader] Error: Invalid .scene File. Missing <scene>" );
		delete XMLDoc;      
		return;
	}
	if (XMLRoot->FirstChildElement("integratedSequence"))
		Sequence::getSingleton().SetSceneSeq(XMLRoot->FirstChildElement("integratedSequence"));

	if (StringConverter::parseBool(getAttrib(XMLRoot,"glass","false"))==false)
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Run3Glass", false);
	}
	else
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Run3Glass", true);
	}
	if (StringConverter::parseBool(getAttrib(XMLRoot,"glow","false"))==false)
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", false);
	}
	else
	{
		CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "Glow", true);
	}
	sceneMultiplier=getAttribReal(XMLRoot,"multiplier",1);
	// figure out where to attach any nodes we create
	mAttachNode = pAttachNode;
	if(!mAttachNode)
		mAttachNode = mSceneMgr->getRootSceneNode();

	// Process the scene
	processScene(XMLRoot);

	// Close the XML File
	delete XMLDoc;
}

void DotSceneLoader::processWater(TiXmlElement *XMLNode)
{
	String fileName=getAttrib(XMLNode,"fileName");
	TiXmlElement* pElement;
	Vector3 sunPos,sunColor,pos;
		// Process nodes (?)
	pElement = XMLNode->FirstChildElement("sunPos");
	if(pElement)
		sunPos=parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("pos");
	if(pElement)
		pos=parseVector3(pElement);
	pElement = XMLNode->FirstChildElement("sunColor");
	if(pElement)
		sunColor=parseVector3(pElement);
	if ((sunPos==Vector3::ZERO)&&(sunColor==Vector3::ZERO)&& fileName.empty())
	{
		WaterManager::getSingleton().create();
	}
	else
	{
	if ( fileName.empty())
	{
		WaterManager::getSingleton().create(sunPos,sunColor);
	}
	else
		WaterManager::getSingleton().create(sunPos,sunColor,fileName,pos);
	}
}

void DotSceneLoader::processScene(TiXmlElement *XMLRoot)
{
	// Process the scene parameters
	String version = getAttrib(XMLRoot, "formatVersion", "unknown");

	String message = "[DotSceneLoader] Parsing dotScene file with version " + version;
	if(XMLRoot->Attribute("ID"))
		message += ", id " + String(XMLRoot->Attribute("ID"));
	if(XMLRoot->Attribute("sceneManager"))
		message += ", scene manager " + String(XMLRoot->Attribute("sceneManager"));
	if(XMLRoot->Attribute("minOgreVersion"))
		message += ", min. Ogre version " + String(XMLRoot->Attribute("minOgreVersion"));
	if(XMLRoot->Attribute("author"))
		message += ", author " + String(XMLRoot->Attribute("author"));

	LogManager::getSingleton().logMessage(message);

	TiXmlElement *pElement;
	// Process nodes (?)
	pElement = XMLRoot->FirstChildElement("nodes");
	if(pElement)
		processNodes(pElement);
	BENCHMARK2(2)
	// Process AI
	pElement = XMLRoot->FirstChildElement("aiNodes");
	if(pElement)
		processNPCNodes(pElement);
	BENCHMARK2(3)
	// Process externals (?)
	pElement = XMLRoot->FirstChildElement("externals");
	if(pElement)
		processExternals(pElement);
	BENCHMARK2(4)
	// Process environment (?)
	pElement = XMLRoot->FirstChildElement("environment");
	if(pElement)
		processEnvironment(pElement);
	BENCHMARK2(5)
	// Process sounds (?)
	pElement = XMLRoot->FirstChildElement("sounds");
	if(pElement)
		processSounds(pElement);
	BENCHMARK2(6)
	// Process terrain (?)
	pElement = XMLRoot->FirstChildElement("terrain");
	if(pElement)
		processTerrain(pElement);
	BENCHMARK2(7)
	// Process portals (?)
	pElement = XMLRoot->FirstChildElement("portal");
	while (pElement)
	{
		processPortals(pElement);
		pElement = pElement->NextSiblingElement("portal");
	}
	BENCHMARK2(8)
	// Process userDataReference (?)
	pElement = XMLRoot->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement);
	BENCHMARK2(9)
	// Process octree (?)
	pElement = XMLRoot->FirstChildElement("octree");
	if(pElement)
		processOctree(pElement);
	BENCHMARK2(10)
	// Process light (?)
	pElement = XMLRoot->FirstChildElement("light");
	while (pElement)
	{
		processLight(pElement);
		pElement = pElement->NextSiblingElement("light");
	}
	BENCHMARK2(11)
// Process light (?)
	/*pElement = XMLRoot->FirstChildElement("light");
	while (pElement)
	{
		processLight(pElement);
		pElement = pElement->NextSiblingElement("light");
	}*/

// Process portal (?)
	/*pElement = XMLRoot->FirstChildElement("portal");
	while (pElement)
	{
		processPortal(pElement);
		pElement = pElement->NextSiblingElement("portal");
	}

// Process portal (?)
	pElement = XMLRoot->FirstChildElement("zone");
	while (pElement)
	{
		processPortal(pElement);
		pElement = pElement->NextSiblingElement("zone");
	}*/

	// Process camera (?)
	pElement = XMLRoot->FirstChildElement("camera");
	if(pElement)
		processCamera(pElement);
}

void DotSceneLoader::processNPCNodes(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("npcnode");
	while(pElement)
	{
		processNPCNode(pElement);
		pElement = pElement->NextSiblingElement("npcnode");
	}
}

void DotSceneLoader::processNPCNode(TiXmlElement *XMLNode)
{
	//TiXmlElement *pElement;
	bool drawNPCNode = getAttribBool(XMLNode,"drawNPCNode",false);
	Vector3 pos = parseVector3(XMLNode)*sceneMultiplier;
	NPCNode* n = new NPCNode(mSceneMgr,pos);
	AIManager::getSingleton().addNPCNode(n);

	
	#ifdef _DEBUG
	LogManager::getSingleton().logMessage("DotSceneLoader: TEST NODES MODE 1!");
	SceneNode* sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(n->getPosition());
	Entity* e = mSceneMgr->createEntity(sn->getName(),"box.mesh");
	sn->setScale(50*sceneMultiplier,50*sceneMultiplier,50*sceneMultiplier);
	sn->attachObject(e);
	return;
	#endif
	if (drawNPCNode)
	{
	LogManager::getSingleton().logMessage("DotSceneLoader: TEST NODES MODE 2!");
	SceneNode* sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	Entity* e = mSceneMgr->createEntity(sn->getName(),"box.mesh");
	sn->setScale(50*sceneMultiplier,50*sceneMultiplier,50*sceneMultiplier);
	sn->attachObject(e);
	}
}

void DotSceneLoader::processPortals(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","");
	LogManager::getSingleton().logMessage("Parsing portal:"+name);
	TiXmlElement *pElement;

	Vector3 pos,scale;
	
	// Process node (*)
	pElement = XMLNode->FirstChildElement("node");
	while(pElement)
	{
		processNode(pElement);
		pElement = pElement->NextSiblingElement("node");
	}

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
	{
		pos = sceneMultiplier*parseVector3(pElement);
	}

	// Process scale (?)
	pElement = XMLNode->FirstChildElement("scale");
	if(pElement)
	{
		scale = sceneMultiplier*parseVector3(pElement);
	}


	ZonePortalManager::getSingleton().passZone(pos,scale);
	
	Real farClip = getAttribReal(XMLNode,"farClip");
	if (farClip)
		ZonePortalManager::getSingleton().setZoneCameraParams(mCamera->getFarClipDistance(),farClip);

	pElement = XMLNode->FirstChildElement("node");
	while(pElement)
	{
		LogManager::getSingleton().logMessage("Adding node to queue.");
		TiXmlElement *pElement2;
		pElement2 = pElement->FirstChildElement("entity");
		while (pElement2)
		{
		ZonePortalManager::getSingleton().passEntity(getAttrib(pElement,"name"));
		pElement2 =pElement2->NextSiblingElement("entity");
		}
		pElement2 = pElement->FirstChildElement("phys");
		while (pElement2)
		{
		ZonePortalManager::getSingleton().passEntity(getAttrib(pElement,"name"));
		pElement2 =pElement2->NextSiblingElement("phys");
		}
		pElement2 = pElement->FirstChildElement("nocollide");
		while (pElement2)
		{
		ZonePortalManager::getSingleton().passEntity(getAttrib(pElement,"name"));
		pElement2 =pElement2->NextSiblingElement("nocollide");
		}
		pElement = pElement->NextSiblingElement("node");
	}
	LogManager::getSingleton().logMessage("Portal parsed.");
}

void DotSceneLoader::processNodes(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	float nodeCnt=0;
	// Process node (*)
	pElement = XMLNode->FirstChildElement("node");
	while(pElement)
	{
		nodeCnt+=0.02;
		BENCHMARK2((int)(11.0f+nodeCnt));
		if (Run3Benchmark::getSingleton().benchMarkEnabled())
		{
			Run3Benchmark::getSingleton().setIterationCommentary(getAttrib(pElement, "name"));
		}
		processNode(pElement);
		pElement = pElement->NextSiblingElement("node");
	}

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
	{
		mAttachNode->setPosition(parseVector3(pElement));
		mAttachNode->setInitialState();
	}

	// Process rotation (?)
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
	{
		mAttachNode->setOrientation(parseQuaternion(pElement));
		mAttachNode->setInitialState();
	}

	// Process scale (?)
	pElement = XMLNode->FirstChildElement("scale");
	if(pElement)
	{
		mAttachNode->setScale(parseVector3(pElement));
		mAttachNode->setInitialState();
	}
}

void DotSceneLoader::processExternals(TiXmlElement *XMLNode)
{
	//! @todo Implement this
}

void DotSceneLoader::processEnvironment(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;

	// Process fog (?)
	pElement = XMLNode->FirstChildElement("fog");
	if(pElement)
		processFog(pElement);

	// Process skyBox (?)
	pElement = XMLNode->FirstChildElement("skyBox");
	if(pElement)
		processSkyBox(pElement);

	// Process newtonWorld (?)
	pElement = XMLNode->FirstChildElement("newtonWorld");
	if(pElement)
		processNewton(pElement);

	// Process newtonWorld (?)
	pElement = XMLNode->FirstChildElement("player");
	if(pElement)
		processPlayer(pElement);

	// Process newtonWorld (?)
	pElement = XMLNode->FirstChildElement("hud");
	if(pElement)
		processShowHUD(pElement);
	
	// Process terrain (?)
	pElement = XMLNode->FirstChildElement("water");
	if(pElement)
		processWater(pElement);

	// Process terrain (?)
	pElement = XMLNode->FirstChildElement("skyx");
	if(pElement)
		processSkyX(pElement);

	// Process skyDome (?)
	pElement = XMLNode->FirstChildElement("skyDome");
	if(pElement)
		processSkyDome(pElement);

	// Process skyPlane (?)
	pElement = XMLNode->FirstChildElement("skyPlane");
	if(pElement)
		processSkyPlane(pElement);

	// Process clipping (?)
	pElement = XMLNode->FirstChildElement("clipping");
	if(pElement)
		processClipping(pElement);

	// Process fade (?)
	pElement = XMLNode->FirstChildElement("fade");
	if(pElement)
		processFade(pElement);

	// Process colourAmbient (?)
	pElement = XMLNode->FirstChildElement("colourAmbient");
	if(pElement)
	{
		global::getSingleton().ambiLight = parseColour(pElement);
		mSceneMgr->setAmbientLight(global::getSingleton().ambiLight);
	}
	// Process colourBackground (?)
	//! @todo Set the background colour of all viewports (RenderWindow has to be provided then)
	pElement = XMLNode->FirstChildElement("colourBackground");
	if(pElement)
		;//mSceneMgr->set(parseColour(pElement));

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement);

	//processFarClip
	pElement = XMLNode->FirstChildElement("farClip");
	if(pElement)
	{
		mCamera->setFarClipDistance(getAttribReal(pElement,"dist",100000.0f));
	}
}

void DotSceneLoader::processSounds(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;

	// Process skyBox (?)
	pElement = XMLNode->FirstChildElement("sound");
	while(pElement)
	{
		processSound(pElement);
		pElement =pElement->NextSiblingElement("sound");
	}

	pElement = XMLNode->FirstChildElement("ambient");
	while(pElement)
	{
		processAmbient(pElement);
		pElement =pElement->NextSiblingElement("ambient");
	}

	pElement = XMLNode->FirstChildElement("music");
	if(pElement)
		processMusic(pElement);
}

void DotSceneLoader::processTerrain(TiXmlElement *XMLNode)
{
	//! @todo Implement this
}

void DotSceneLoader::processUserDataReference(TiXmlElement *XMLNode, SceneNode *pParent)
{
	//! @todo Implement this
}

void DotSceneLoader::processOctree(TiXmlElement *XMLNode)
{
	//! @todo Implement this
}

void DotSceneLoader::processZone(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX"),getAttribReal(XMLNode,"sY"),getAttribReal(XMLNode,"sZ"));
	ZonePortalManager::getSingleton().passZone(pos,scale);
	pElement= XMLNode->FirstChildElement("entity");
	while (pElement)
	{
		ZonePortalManager::getSingleton().passEntity(getAttrib(pElement,"name"));
		pElement =pElement->NextSiblingElement("entity");
	}
}

void DotSceneLoader::processPortal(TiXmlElement *XMLNode)
{
		TiXmlElement *pElement;
	Vector3 pos = parseVector3(XMLNode);
	Vector3 scale = Vector3(getAttribReal(XMLNode,"sX"),getAttribReal(XMLNode,"sY"),getAttribReal(XMLNode,"sZ"));
	ZonePortalManager::getSingleton().passPortal(pos,scale);
	pElement= XMLNode->FirstChildElement("entity");
	while (pElement)
	{
		ZonePortalManager::getSingleton().passEntity(getAttrib(pElement,"name"));
		pElement =pElement->NextSiblingElement("entity");
	}
}

void DotSceneLoader::processLight(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	Real dist = getAttribReal(XMLNode, "dist",-1);
	Real shdist = getAttribReal(XMLNode, "shadowdist",-1);
	// Create the light
	if (!Loader::getSingleton().deferred)
	{
	Light *pLight = mSceneMgr->createLight(name);
	if(pParent)
		pParent->attachObject(pLight);

	String sValue = getAttrib(XMLNode, "type");

	if(sValue == "point")
		pLight->setType(Light::LT_POINT);
	else if(sValue == "directional")
		pLight->setType(Light::LT_DIRECTIONAL);
	else if(sValue == "spot")
		pLight->setType(Light::LT_SPOTLIGHT);
	else if(sValue == "radPoint")
		pLight->setType(Light::LT_POINT);
	

	pLight->setVisible(getAttribBool(XMLNode, "visible", true));
	pLight->setCastShadows(getAttribBool(XMLNode, "castShadows", true));
	pLight->setPowerScale(getAttribReal(XMLNode,"power",1));
	TiXmlElement *pElement;

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
		pLight->setPosition(parseVector3(pElement));
	// Process normal (?)
	pElement = XMLNode->FirstChildElement("normal");
	if(pElement)
	{
		pLight->setDirection(parseVector3(pElement));
	}
	// Process colourDiffuse (?)
	pElement = XMLNode->FirstChildElement("colourDiffuse");
	if(pElement)
		pLight->setDiffuseColour(parseColour(pElement));

	// Process colourSpecular (?)
	pElement = XMLNode->FirstChildElement("colourSpecular");
	if(pElement)
		pLight->setSpecularColour(parseColour(pElement));

	// Process lightRange (?)
	pElement = XMLNode->FirstChildElement("lightRange");
	if(pElement)
		processLightRange(pElement, pLight);

	// Process lightAttenuation (?)
	pElement = XMLNode->FirstChildElement("lightAttenuation");
	if(pElement)
		processLightAttenuation(pElement, pLight);
	// Process dynamics (?)
	pElement = XMLNode->FirstChildElement("dynamic");
	if(pElement)
		processLightDynamics(pElement, pLight);
	Loader::getSingleton().pssmAddLight(pLight);
	if (dist!=-1.0f)
	{
		Vector3 pos = pParent->getPosition();
		//pLight->setRenderingDistance(dist);
		LightPerfomanceManager::getSingleton().addLight(AxisAlignedBox(pos-Vector3(dist,dist,dist),pos+Vector3(dist,dist,dist)),name);
	}
	if (shdist!=-1.0f)
	{
		//Vector3 pos = pParent->getPosition();
		//pLight->setRenderingDistance(dist);
		//LightPerfomanceManager::getSingleton().addLight(AxisAlignedBox(pos-Vector3(dist,dist,dist),pos+Vector3(dist,dist,dist)),name);
		pLight->setShadowFarDistance(shdist);
	}

	LogManager::getSingleton().logMessage("Derived direction of the light:"+StringConverter::toString(pLight->getDerivedDirection()));

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		;//processUserDataReference(pElement, pLight);
	}
	else
	{
		MLight* m = global::getSingleton().iSystem->createMLight();
		if(pParent)
		pParent->attachObject(m);
		TiXmlElement *pElement;
		pElement = XMLNode->FirstChildElement("colourDiffuse");
	if(pElement)
		m->setDiffuseColour(parseColour(pElement));

	// Process colourSpecular (?)
	pElement = XMLNode->FirstChildElement("colourSpecular");
	if(pElement)
		m->setSpecularColour(parseColour(pElement));
	// Process lightAttenuation (?)
	pElement = XMLNode->FirstChildElement("lightAttenuation");
	if(pElement)
	{
		Real range = getAttribReal(XMLNode, "range");
	Real constant = getAttribReal(XMLNode, "constant");
	Real linear = getAttribReal(XMLNode, "linear");
	Real quadratic = getAttribReal(XMLNode, "quadratic");

	// Setup the light attenuation
	m->setAttenuation(getAttribReal(pElement, "a"),getAttribReal(pElement, "b"), getAttribReal(pElement, "c"));
	}
	}
}

void DotSceneLoader::processLightDynamics(TiXmlElement *XMLNode, Light *pLight)
{
	ColourValue c = parseColour(XMLNode);
	 ControllerValueRealPtr mLightFlasher = ControllerValueRealPtr(
            new LightFlasher(pLight, c));
	ControllerFunctionRealPtr mLightControllerFunc = ControllerFunctionRealPtr(
            new LightFlasherControllerFunction(Ogre::WFT_SINE, 0.5, 0.0));
	ControllerManager* mControllerManager = &ControllerManager::getSingleton();
	Controller<Real>* mLightController = mControllerManager->createController(mControllerManager->getFrameTimeSource(), mLightFlasher, mLightControllerFunc);
	lctrl.push_back(mLightController);
	lctrlf.push_back(mLightControllerFunc);
	lctrlv.push_back(mLightFlasher);
}


void DotSceneLoader::processCamera(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	Real fov = getAttribReal(XMLNode, "fov", 45);
	Real aspectRatio = getAttribReal(XMLNode, "aspectRatio", 1.3333);
	String projectionType = getAttrib(XMLNode, "projectionType", "perspective");

	// Create the camera
	Camera *pCamera = mSceneMgr->createCamera(name);
	if(pParent)
		pParent->attachObject(pCamera);
	
	// Set the field-of-view
	//! @todo Is this always in degrees?
	pCamera->setFOVy(Ogre::Degree(fov));

	// Set the aspect ratio
	pCamera->setAspectRatio(aspectRatio);
	
	// Set the projection type
	if(projectionType == "perspective")
		pCamera->setProjectionType(PT_PERSPECTIVE);
	else if(projectionType == "orthographic")
		pCamera->setProjectionType(PT_ORTHOGRAPHIC);


	TiXmlElement *pElement;

	// Process clipping (?)
	pElement = XMLNode->FirstChildElement("clipping");
	if(pElement)
	{
		Real nearDist = getAttribReal(pElement, "near");
		pCamera->setNearClipDistance(nearDist);

		Real farDist =  getAttribReal(pElement, "far");
		pCamera->setFarClipDistance(farDist);
	}

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
		pCamera->setPosition(parseVector3(pElement));

	// Process rotation (?)
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
		pCamera->setOrientation(parseQuaternion(pElement));

	// Process normal (?)
	pElement = XMLNode->FirstChildElement("normal");
	if(pElement)
		;//!< @todo What to do with this element?

	// Process lookTarget (?)
	pElement = XMLNode->FirstChildElement("lookTarget");
	if(pElement)
		;//!< @todo Implement the camera look target

	// Process trackTarget (?)
	pElement = XMLNode->FirstChildElement("trackTarget");
	if(pElement)
		;//!< @todo Implement the camera track target

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		;//!< @todo Implement the camera user data reference
}

void DotSceneLoader::processNode(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Construct the node's name
	String name = m_sPrependNode + getAttrib(XMLNode, "name");

	// Create the scene node
	SceneNode *pNode;
	if((name.empty())||(mSceneMgr->hasSceneNode(name)))
	{
		// Let Ogre choose the name
		if(pParent)
			pNode = pParent->createChildSceneNode();
		else
			pNode = mAttachNode->createChildSceneNode();
	}
	else
	{
		// Provide the name
		if(pParent)
			pNode = pParent->createChildSceneNode(name);
		else
			pNode = mAttachNode->createChildSceneNode(name);
	}

	// Process other attributes
	String id = getAttrib(XMLNode, "id");
	bool isTarget = getAttribBool(XMLNode, "isTarget");

	TiXmlElement *pElement;

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
	{
		pNode->setPosition(parseVector3(pElement)*sceneMultiplier);
		pNode->setInitialState();
	}

	// Process rotation (?)
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
	{
		pNode->setOrientation(parseQuaternion(pElement));
		pNode->setInitialState();
	}
	
	pNode->setScale(sceneMultiplier*Vector3(1,1,1));
	pNode->setInitialState();
	// Process scale (?)
	pElement = XMLNode->FirstChildElement("scale");
	if(pElement)
	{
		pNode->setScale(parseVector3(pElement)*sceneMultiplier);
		pNode->setInitialState();
	}

	// Process lookTarget (?)
	pElement = XMLNode->FirstChildElement("lookTarget");
	if(pElement)
		processLookTarget(pElement, pNode);

	// Process trackTarget (?)
	pElement = XMLNode->FirstChildElement("trackTarget");
	if(pElement)
		processTrackTarget(pElement, pNode);

	// Process node (*)
	pElement = XMLNode->FirstChildElement("node");
	while(pElement)
	{
		processNode(pElement, pNode);
		pElement = pElement->NextSiblingElement("node");
	}

	// Process entity (*)
	pElement = XMLNode->FirstChildElement("entity");
	while(pElement)
	{
		processEntity(pElement, pNode);
		pElement = pElement->NextSiblingElement("entity");
	}

	// Process phys-entity (*)
	pElement = XMLNode->FirstChildElement("phys");
	while(pElement)
	{
		processPhysObj(pElement, pNode);
		pElement = pElement->NextSiblingElement("phys");
	}
	// Process blockbox-entity (*)
	pElement = XMLNode->FirstChildElement("blockbox");
	while(pElement)
	{
		processPhysBox(pElement, pNode);
		pElement = pElement->NextSiblingElement("blockbox");
	}
	// Process blocking box (*)
	pElement = XMLNode->FirstChildElement("pblock");
	while(pElement)
	{
		processPhysBlock(pElement, pNode);
		pElement = pElement->NextSiblingElement("pblock");
	}

// Process mirror (*)
	pElement = XMLNode->FirstChildElement("mirror");
	while(pElement)
	{
		processMirror(pElement, pNode);
		pElement = pElement->NextSiblingElement("mirror");
	}

	pElement = XMLNode->FirstChildElement("breakable");
	while(pElement)
	{
		processBreakable(pElement, pNode);
		pElement = pElement->NextSiblingElement("breakable");
	}
	pElement = XMLNode->FirstChildElement("ragdoll");
	while(pElement)
	{
		processRagDoll(pElement, pNode);
		pElement = pElement->NextSiblingElement("ragdoll");
	}
	//Process tree
	pElement = XMLNode->FirstChildElement("tree");
	while(pElement)
	{
		processTree(pElement, pNode);
		pElement = pElement->NextSiblingElement("tree");
	}
	// Process light (*)
	pElement = XMLNode->FirstChildElement("light");
	while(pElement)
	{
		processLight(pElement, pNode);
		pElement = pElement->NextSiblingElement("light");
	}

	// Process camera (*)
	pElement = XMLNode->FirstChildElement("camera");
	while(pElement)
	{
		processCamera(pElement, pNode);
		pElement = pElement->NextSiblingElement("camera");
	}

	// Process particleSystem (*)
	pElement = XMLNode->FirstChildElement("particleSystem");
	while(pElement)
	{
		processParticleSystem(pElement, pNode);
		pElement = pElement->NextSiblingElement("particleSystem");
	}
// Process particleSystem (*)
	pElement = XMLNode->FirstChildElement("fire");
	while(pElement)
	{
		processFire(pElement, pNode);
		pElement = pElement->NextSiblingElement("fire");
	}
	// Process billboardSet (*)
	pElement = XMLNode->FirstChildElement("billboardSet");
	while(pElement)
	{
		processBillboardSet(pElement, pNode);
		pElement = pElement->NextSiblingElement("billboardSet");
	}

	// Process plane (*)
	pElement = XMLNode->FirstChildElement("plane");
	while(pElement)
	{
		processPlane(pElement, pNode);
		pElement = pElement->NextSiblingElement("plane");
	}
	
	
	// Process NoCollide (*)
	pElement = XMLNode->FirstChildElement("nocollide");
	while(pElement)
	{
		processNoCollide(pElement, pNode);
		pElement = pElement->NextSiblingElement("nocollide");
	}
	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, pNode);
}

void DotSceneLoader::processLookTarget(TiXmlElement *XMLNode, SceneNode *pParent)
{
	//! @todo Is this correct? Cause I don't have a clue actually

	// Process attributes
	String nodeName = getAttrib(XMLNode, "nodeName");

	Node::TransformSpace relativeTo = Node::TS_PARENT;
	String sValue = getAttrib(XMLNode, "relativeTo");
	if(sValue == "local")
		relativeTo = Node::TS_LOCAL;
	else if(sValue == "parent")
		relativeTo = Node::TS_PARENT;
	else if(sValue == "world")
		relativeTo = Node::TS_WORLD;

	TiXmlElement *pElement;

	// Process position (?)
	Vector3 position;
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
		position = parseVector3(pElement);

	// Process localDirection (?)
	Vector3 localDirection = Vector3::NEGATIVE_UNIT_Z;
	pElement = XMLNode->FirstChildElement("localDirection");
	if(pElement)
		localDirection = parseVector3(pElement);

	// Setup the look target
	try
	{
		if(!nodeName.empty())
		{
			SceneNode *pLookNode = mSceneMgr->getSceneNode(nodeName);
			position = pLookNode->_getDerivedPosition();
		}

		pParent->lookAt(position, relativeTo, localDirection);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error processing a look target!");
	}
}

void DotSceneLoader::processTrackTarget(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String nodeName = getAttrib(XMLNode, "nodeName");

	TiXmlElement *pElement;

	// Process localDirection (?)
	Vector3 localDirection = Vector3::NEGATIVE_UNIT_Z;
	pElement = XMLNode->FirstChildElement("localDirection");
	if(pElement)
		localDirection = parseVector3(pElement);

	// Process offset (?)
	Vector3 offset = Vector3::ZERO;
	pElement = XMLNode->FirstChildElement("offset");
	if(pElement)
		offset = parseVector3(pElement);

	// Setup the track target
	try
	{
		SceneNode *pTrackNode = mSceneMgr->getSceneNode(nodeName);
		pParent->setAutoTracking(true, pTrackNode, localDirection, offset);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error processing a track target!");
	}
}

std::string DotSceneLoader::_tostr(float a)
{
    std::ostringstream stream;
    stream.precision(6);
    stream.width(0);
    stream.fill(((char)" "));
    stream << a;
    return stream.str();
}

void DotSceneLoader::processNoCollide(TiXmlElement *XMLNode, SceneNode *pParent)
{
// Process attributes
	String name = getAttrib(XMLNode, "name",pParent->getName());
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String materialFile = getAttrib(XMLNode, "materialFile");
	Real scalex = getAttribReal(XMLNode,"scaleU",1);
	Real scaley = getAttribReal(XMLNode,"scaleV",1);
	Real maxDist = getAttribReal(XMLNode,"maxDist",-1);
	bool isStatic = getAttribBool(XMLNode, "run3batcher", false);
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);
	bool buildTangents = getAttribBool(XMLNode,"buildTangents",true);

	// TEMP: Maintain a list of static and dynamic objects
	if(isStatic)
		staticObjects.push_back(name);
	else
		dynamicObjects.push_back(name);

	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)

	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	Entity* ent;
	if (!mSceneMgr->hasEntity(name))
		ent = mSceneMgr->createEntity(name,meshFile);
	else
	{
		//LogManager::getSingleton().logMessage("assigning a new name to entity"+pParent->getName());
		name=pParent->getName();
		while (mSceneMgr->hasEntity(name))
			name=name+"1";
		ent = mSceneMgr->createEntity(name,meshFile);
	}
	try
	{
		MeshPtr pMesh =  MeshManager::getSingleton().load(meshFile, m_sGroupName);
		pParent->attachObject(ent);

		if (buildTangents)
		{
		unsigned short src, dest;
		if (!pMesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
		{
		pMesh->buildTangentVectors(VES_TANGENT, src, dest);
		}
		}
		
		ent->setCastShadows(castShadows);
		
		if (maxDist!=-1.0f)
		ent->setRenderingDistance(maxDist);
		
		if(!materialFile.empty() && !Loader::getSingleton().material_disable)
		{
			MaterialPtr mat2;
			texture_copy_counter++;
			Real scalex = getAttribReal(XMLNode,"scaleU",1);
			Real scaley = getAttribReal(XMLNode,"scaleV",1);
			Real scrollx = getAttribReal(XMLNode,"scrollU",0);
			Real scrolly = getAttribReal(XMLNode,"scrollV",0);
			MaterialPtr mat = (MaterialPtr)MaterialManager::getSingleton().getByName(materialFile);
			if ((scalex==1)&&(scaley==1)&&(scrollx==0)&&(scrolly==0))
			{
				mat2=mat;
			}
			else
			{
			mat->clone(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			mat2 = (MaterialPtr)MaterialManager::getSingleton().getByName(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScale(scalex,scaley);
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScroll(scrollx,scrolly);
			mat2->getTechnique(0)->setLodIndex(100);
			}
			ent->setMaterial(mat2);
			

		}
		if (Loader::getSingleton().material_disable)
			ent->setMaterialName("run3TESTu");
		if (Loader::getSingleton().depthshadowmap)
				ent->setMaterialName("metal");
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading an entity!");
	}
	if (isStatic)
	{
		Run3Batcher::getSingleton().addObject(ent,pParent->getPosition(),pParent->getScale(),pParent->getOrientation());
	}
	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, ent);

}

void DotSceneLoader::processEntity(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name",pParent->getName());
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String materialFile = getAttrib(XMLNode, "materialFile");
	Real scalex = getAttribReal(XMLNode,"scaleU",1);
	Real scaley = getAttribReal(XMLNode,"scaleV",1);
	Real maxDist = getAttribReal(XMLNode,"maxDist",-1);
	bool isStatic = getAttribBool(XMLNode, "run3batcher", false);;
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);
	bool buildTangents = getAttribBool(XMLNode,"buildTangents",true);

	// TEMP: Maintain a list of static and dynamic objects
	if(!isStatic)
		dynamicObjects.push_back(name);

	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)

	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		MeshPtr pMesh =  MeshManager::getSingleton().load(meshFile, m_sGroupName);
		pMesh->buildEdgeList();
		pMesh->prepareForShadowVolume();
		/*if (maxDist!=-1.0f)
			pMesh->createManualLodLevel(maxDist,"empty_model01.mesh");*/

		if (buildTangents)
		{
		unsigned short src, dest;
		if (!pMesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
		{
		pMesh->buildTangentVectors(VES_TANGENT, src, dest);
		}
		}
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticObject(name,meshFile,pParent);
		phys->ent->setCastShadows(castShadows);
		//if (isStatic)
		//	phys->addToBatch();
		/*phys->ent->setMaterialLodBias(1,0,10);
		phys->ent->setMeshLodBias(1,0,10);*/
		/*phys->ent->setMaterialLodBias(levelofDetailT);
		phys->ent->setMeshLodBias(levelofDetailM);/*/
		if (maxDist!=-1.0f)
		phys->ent->setRenderingDistance(maxDist);
		/*if (levelofDetailT!=-1)
		{
			for (unsigned int i=0;i!=phys->ent->getNumSubEntities();i++)
			{
				MaterialPtr mat = phys->ent->getSubEntity(i)->getMaterial();
				if (!mat.isNull())
				{
					LogManager::getSingleton().logMessage(StringConverter::toString(levelofDetailT));
					if (mat->getTechnique(0)->getPass(0)->getTextureUnitState(0))
					{
					mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureMipmapBias(levelofDetailT);
					}
				}
			}
		}*/
		//bodies.push_back(phys->bod);
		if(!materialFile.empty() && !Loader::getSingleton().material_disable)
		{
			/*texture_copy_counter++;
			Real scalex = getAttribReal(XMLNode,"scaleU",1);
			Real scaley = getAttribReal(XMLNode,"scaleV",1);
			Real scrollx = getAttribReal(XMLNode,"scrollU",0);
			Real scrolly = getAttribReal(XMLNode,"scrollV",0);
			MaterialPtr mat = (MaterialPtr)MaterialManager::getSingleton().getByName(materialFile);*/

			//mat->
			//mat->getTechnique(0)->setLodIndex(65534);
			/*std::vector<Real> lods;
			lods.assign(65535,1);
			mat->setLodLevels(lods);*/
				/*Ogre::Real s;
			s=(Ogre::Real)mat->getTechnique(0)->getPass(0)->getPointSize();
			String str1=_tostr(s/phys->get_z());
			String str2=_tostr(s/phys->get_x());
			mat->setParameter("scale",str1+" "+str2);*/
			/*mat->clone(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			MaterialPtr mat2 = (MaterialPtr)MaterialManager::getSingleton().getByName(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScale(scalex,scaley);
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScroll(scrollx,scrolly);
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureMipmapBias(levelofDetailT);*/
			//mat2->getTechnique(0)->setLodIndex(100);
			//phys->ent->setMaterial(mat2);
			
			/*phys->ent->setMaterialLodBias(90,0,10);
			phys->ent->setMeshLodBias(90,0,10);*/

			MaterialPtr mat2;
			texture_copy_counter++;
			Real scalex = getAttribReal(XMLNode,"scaleU",1);
			Real scaley = getAttribReal(XMLNode,"scaleV",1);
			Real scrollx = getAttribReal(XMLNode,"scrollU",0);
			Real scrolly = getAttribReal(XMLNode,"scrollV",0);
			MaterialPtr mat = (MaterialPtr)MaterialManager::getSingleton().getByName(materialFile);
			if ((scalex==1)&&(scaley==1)&&(scrollx==0)&&(scrolly==0))
			{
				mat2=mat;
			}
			else
			{
			mat->clone(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			mat2 = (MaterialPtr)MaterialManager::getSingleton().getByName(_tostr((float)texture_copy_counter)+"_"+mat->getName());
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScale(scalex,scaley);
			mat2->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScroll(scrollx,scrolly);
			mat2->getTechnique(0)->setLodIndex(100);
			}
			phys->ent->setMaterial(mat2);
		}
		
		if (Loader::getSingleton().material_disable)
			phys->ent->setMaterialName("run3TESTu");
		if (Loader::getSingleton().depthshadowmap)
				phys->ent->setMaterialName("metal");
		if (isStatic)
		{
			phys->addToBatch();
		}
		else
		{
			bool noCull = getAttribBool(XMLNode,"noCull",false);
			if (!noCull)
			{
				SoftwareOcclusionCulling::getSingleton().passEntity(phys->ent);
			}
		}
		//else
		//{
		global::getSingleton().addPhysObject(phys);
		//}
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading an entity!");
	}

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);

	
}


void DotSceneLoader::processFire(TiXmlElement *XMLNode, SceneNode *pParent)
{
	String name = getAttrib(XMLNode, "pSys");
	Real rd = getAttribReal(XMLNode,"renderDist",5000);
	Fire* f = new Fire(mSceneMgr,pParent,name,rd);
	f->fire();
	fires.push_back(f);
}

void DotSceneLoader::processSkyX(TiXmlElement *XMLNode)
{
	SkyManager::getSingleton().create();
}

void DotSceneLoader::processMirror(TiXmlElement *XMLNode, SceneNode *pParent)
{
	//String name = getAttrib(XMLNode, "name");
	String mat = getAttrib(XMLNode, "metalTexture");
	MirrorManager::getSingleton().createNewMirror(mat,pParent,pParent->getScale());
}

void DotSceneLoader::util_deleteRagDoll(String name)
{
	std::vector<RagDoll*>::iterator i;
	for (i=ragdolls.begin();i!=ragdolls.end();i++)
	{
		RagDoll* rd = (*i);
		SceneNode* n = rd->getSceneNode();
		if (n->getName()==name)
		{
			//mSceneMgr->destroySceneNode(n);
			delete rd;
		}
	}
}

void DotSceneLoader::util_processRagDoll(String name,String scriptFile,String meshFile, SceneNode *pParent)
{
	dynamicObjects.push_back(name);
	try
	{
		MeshManager::getSingleton().load(meshFile, m_sGroupName);
		Entity* ragDoll = mSceneMgr->createEntity(name,meshFile);
		pParent->attachObject(ragDoll);
		RagDoll* mRagdoll = new RagDoll( scriptFile, mWorld, pParent,8.0f );
		ragdolls.push_back(mRagdoll);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading ragdoll");
	}
}

void DotSceneLoader::processRagDoll(TiXmlElement *XMLNode, SceneNode *pParent)
{
	String name = getAttrib(XMLNode, "name");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String scriptFile = getAttrib(XMLNode, "scriptFile");
	
	dynamicObjects.push_back(name);
	try
	{
		MeshManager::getSingleton().load(meshFile, m_sGroupName);
		Entity* ragDoll = mSceneMgr->createEntity(name,meshFile);
		pParent->attachObject(ragDoll);
		RagDoll* mRagdoll = new RagDoll( scriptFile, mWorld, pParent,30.0f );
		ragdolls.push_back(mRagdoll);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading ragdoll");
	}
}

void DotSceneLoader::processPhysObj(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String materialFile = getAttrib(XMLNode, "materialFile");
	Ogre::Real mass = getAttribReal(XMLNode, "mass", 100.0);
	Ogre::Real poMd = getAttribReal(XMLNode, "poMd", -1.0f);
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);
	bool idleanim = getAttribBool(XMLNode, "idleanim", false);
	String idle = getAttrib(XMLNode, "idle");
	// TEMP: Maintain a list of static and dynamic objects
	dynamicObjects.push_back(name);

	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)
	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		MeshManager::getSingleton().load(meshFile, m_sGroupName);
		phys->init(mSceneMgr,mWorld);
		phys->CreateObject(name,meshFile,pParent,mass,idleanim,idle);
		phys->ent->setCastShadows(castShadows);
		phys->setMaxDist(poMd);
		//bodies.push_back(phys->bod);
		if(!materialFile.empty())
			phys->ent->setMaterialName(materialFile);
		global::getSingleton().addPhysObject(phys);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading a physics object!");
	}

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);

	
}

void DotSceneLoader::processPhysBlock(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");

	// TEMP: Maintain a list of static and dynamic objects
	staticObjects.push_back(name);
	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)
	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticBoxTransparent(name,pParent);
		//bodies.push_back(phys->bod);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error building a physbox!");
	}

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);

	
}

void DotSceneLoader::processPhysBox(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String file = getAttrib(XMLNode, "meshFile");
	String id = getAttrib(XMLNode, "id");
	Real maxDist = getAttribReal(XMLNode,"maxDist",-1);
	bool isStatic = getAttribBool(XMLNode, "run3batcher", false);

	// TEMP: Maintain a list of static and dynamic objects
	staticObjects.push_back(name);
	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)
	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		MeshManager::getSingleton().load(file, m_sGroupName);
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticPhys_Box(name,file,pParent);
		if (maxDist!=-1.0f)
		phys->ent->setRenderingDistance(maxDist);
		//bodies.push_back(phys->bod);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error building a physbox!");
	}

	if (isStatic)
	{
			phys->addToBatch();
	}
	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);

	
}

void DotSceneLoader::processPhysCyl(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	Real radius = getAttribReal(XMLNode, "radius");
	Real height = getAttribReal(XMLNode, "height");
	// TEMP: Maintain a list of static and dynamic objects
	staticObjects.push_back(name);
	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)
	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticCylTransparent(name,pParent,radius,height);
		//bodies.push_back(phys->bod);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error building a physcyl!");
	}

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);

	
}



void DotSceneLoader::processParticleSystem(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String file = getAttrib(XMLNode, "file");
staticObjects.push_back(name);
	// Create the particle system
	try
	{
		ParticleSystem *pParticles = mSceneMgr->createParticleSystem(name, file);
		pParent->attachObject(pParticles);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error creating a particle system!");
	}
}

void DotSceneLoader::processBillboardSet(TiXmlElement *XMLNode, SceneNode *pParent)
{
	//! @todo Implement this
}

void DotSceneLoader::processPlane(TiXmlElement *XMLNode, SceneNode *pParent)
{
	//! @todo Implement this
}

void DotSceneLoader::processSound(TiXmlElement *XMLNode)
{
	//input data
	unsigned int audioId;
	String soundpath = "Run3/sounds/";
	String name = soundpath+"/"+getAttrib(XMLNode, "name");
	Real maxDistance = getAttribReal(XMLNode, "maxDistance",500);
	Real minGain = getAttribReal(XMLNode, "minGain",1000);
	Real rollOff = getAttribReal(XMLNode, "rollOff",10000);
	Real distance = getAttribReal(XMLNode, "distance",10000);
	bool loop = getAttribBool(XMLNode, "loop",false);
	Vector3 pos = parseVector3(XMLNode);
	//set sound
	sound->loadAudio(name,&audioId,loop);
	//sound->setSoundPosition(audioId,pos,Vector3::ZERO,Vector3::ZERO);
	sound->setSound(audioId,pos,Vector3::ZERO,Vector3::ZERO,maxDistance,false,loop,minGain,rollOff,distance);
	//sound->setSoundPosition(audioId,Vector3(10,100,10),Vector3::ZERO,Vector3::ZERO);
	sound->playAudio(audioId,true);
	sounds.push_back(audioId);
}


void DotSceneLoader::processAmbient(TiXmlElement *XMLNode)
{
	//input data
//	unsigned int audioId;
	String soundpath = "Run3/sounds/";
	String name = soundpath+"/"+getAttrib(XMLNode, "name");
	String objName = getAttrib(XMLNode, "objname");
	Real maxDistance = getAttribReal(XMLNode, "maxDistance",500)*sceneMultiplier;
	Real minGain = getAttribReal(XMLNode, "minGain",-1)*sceneMultiplier;
	Real rollOff = getAttribReal(XMLNode, "rollOff",10000)*sceneMultiplier;
	Real distance = getAttribReal(XMLNode, "distance",10000)*sceneMultiplier;
	//bool loop = getAttribBool(XMLNode, "loop",false);
	Vector3 pos = parseVector3(XMLNode)*sceneMultiplier;
	//set sound
	if (minGain>0)
	{
		Run3SoundRuntime::getSingleton().addAmbientSound(objName,name,pos,maxDistance,distance);
	}
	else
	{
		Run3SoundRuntime::getSingleton().addAmbientSound(objName,name,pos,maxDistance,distance,minGain);
	}

}

void DotSceneLoader::processMusic(TiXmlElement *XMLNode)
{
	String soundpath = "run3/sounds";
	String name = soundpath+"/"+getAttrib(XMLNode, "name");
	LogManager::getSingleton().logMessage(getAttrib(XMLNode, "name"));
	bool loop = getAttribBool(XMLNode, "loop",false);
	MusicPlayer::getSingleton().playMusic(name,loop);
}

void DotSceneLoader::processTree(TiXmlElement *XMLNode, SceneNode *pParent)
{
	/*Vector3 pos;
	Quaternion rot;
	Vector3 scale;*/
	String name = getAttrib(XMLNode,"name");
	ManualObject* geom = mSceneMgr->createManualObject(name);
	BillboardSet* bset = mSceneMgr->createBillboardSet(name);
	//TiXmlElement *pElement;

	/*pElement = XMLNode->FirstChildElement("position");
	if(pElement)
		pos=parseVector3(pElement);

	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
		rot=parseQuaternion(pElement);

	pElement = XMLNode->FirstChildElement("scale");
	if(pElement)
		scale=parseVector3(pElement);*/

	String fil = getAttrib(XMLNode,"treeFile","cadune01.mtd");
	Parameters* mParameters = CaduneTree::Serializer::importDefinition(fil);

	Stem* mTrunk = new Stem(mParameters);
	mTrunk->grow(Quaternion::IDENTITY,Vector3::ZERO);
	mTrunk->createGeometry(geom);
	mTrunk->createLeaves(bset);
	geom->convertToMesh(pParent->getName()+"TREEM");
	//Entity* ent = mSceneMgr->createEntity(pParent->getName(),pParent->getName()+"TREEM");
	//pParent->attachObject(ent);
	pParent->createChildSceneNode()->attachObject(bset);

	/*mParent->setPosition(pos);
	mParent->setOrientation(rot);
	mParent->*/
	trunks.push_back(mTrunk);
	trunks2.push_back(geom);
	trunks3.push_back(bset);
dynamicObjects.push_back(name+"TREEME");
PhysObject* phys = new PhysObject;
	try
	{
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticObject(name+"TREEME",pParent->getName()+"TREEM",pParent);
		//phys->ent->setCastShadows(castShadows);
		global::getSingleton().addPhysObject(phys);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading an tree!");
	}
	/*OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(mWorld, pParent,true);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( pParent );
	bod->setName(pParent->getName());
	//bod->setMaterialGroupID( physicalMat );
	bod->setPositionOrientation( pParent->getPosition(), pParent->getOrientation() );
	trunks4.push_back(bod);
	delete col;*/

}

void DotSceneLoader::processFog(TiXmlElement *XMLNode)
{
	// Process attributes
	Real expDensity = getAttribReal(XMLNode, "expDensity", 0.001);
	Real linearStart = getAttribReal(XMLNode, "linearStart", 0.0);
	Real linearEnd = getAttribReal(XMLNode, "linearEnd", 1.0);

	FogMode mode = FOG_NONE;
	String sMode = getAttrib(XMLNode, "mode");
	if(sMode == "none")
		mode = FOG_NONE;
	else if(sMode == "exp")
		mode = FOG_EXP;
	else if(sMode == "exp2")
		mode = FOG_EXP2;
	else if(sMode == "linear")
		mode = FOG_LINEAR;

	TiXmlElement *pElement;

	// Process colourDiffuse (?)
	ColourValue colourDiffuse = ColourValue::White;
	pElement = XMLNode->FirstChildElement("colourDiffuse");
	if(pElement)
		colourDiffuse = parseColour(pElement);

	// Setup the fog
	mSceneMgr->setFog(mode, colourDiffuse, expDensity, linearStart, linearEnd);
}

void DotSceneLoader::processFade(TiXmlElement *XMLNode)
{
	String material = getAttrib(XMLNode, "material");
	String overlay = getAttrib(XMLNode, "overlay");
	Real duration = getAttribReal(XMLNode, "duration",1);
	Real speed_f = getAttribReal(XMLNode, "speed",1);
	bool startFade = getAttribBool(XMLNode, "startFade",false);
	FadeListener::getSingleton().setDuration(duration);
	FadeListener::getSingleton().setDurationF(speed_f);
	if (startFade)
	{
		FadeListener::getSingleton().startIN();
	}
}

void DotSceneLoader::processShowHUD(TiXmlElement *XMLNode)
{
	bool show = getAttribBool(XMLNode,"show",true);
	if (!show)
	{
		HUD::getSingleton().Hide();
		HUD::getSingleton().Hide_crosshair();
	}
	if (show)
	{
		HUD::getSingleton().Show();
		HUD::getSingleton().Show_crosshair();
	}
}

void DotSceneLoader::processSkyBox(TiXmlElement *XMLNode)
{
	// Process attributes
	String material = getAttrib(XMLNode, "material");
	Real distance = getAttribReal(XMLNode, "distance", 5000);
	bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);

	TiXmlElement *pElement;

	// Process rotation (?)
	Quaternion rotation = Quaternion::IDENTITY;
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
		rotation = parseQuaternion(pElement);

	// Setup the sky box
	mSceneMgr->setSkyBox(true, material, distance, drawFirst, rotation, m_sGroupName);
}

void DotSceneLoader::processBreakable(TiXmlElement *XMLNode, SceneNode *pParent)
{
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String gibmeshFile = getAttrib(XMLNode, "gibMesh","box.mesh");
	Vector3 gibScale = StringConverter::parseVector3(getAttrib(XMLNode, "gibScale","1 1 1"));
	String materialFile = getAttrib(XMLNode, "materialFile");
	Ogre::Real mass = getAttribReal(XMLNode, "mass", 10.0);
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);
	bool staticPhys = getAttribBool(XMLNode, "static", false);
	bool box = getAttribBool(XMLNode, "box", false);
	bool explosive = getAttribBool(XMLNode, "explosive", false);
	bool battouch = getAttribBool(XMLNode, "battouch", false);
	String health = getAttrib(XMLNode, "health","30");
	String strength = getAttrib(XMLNode, "strength","10");
	unsigned int c = StringConverter::parseUnsignedInt(getAttrib(XMLNode,"count","10"));
	// TEMP: Maintain a list of static and dynamic objects
	dynamicObjects.push_back(name);

	TiXmlElement *pElement;

	// Process vertexBuffer (?)
	pElement = XMLNode->FirstChildElement("vertexBuffer");
	if(pElement)
		;//processVertexBuffer(pElement);

	// Process indexBuffer (?)
	pElement = XMLNode->FirstChildElement("indexBuffer");
	if(pElement)
		;//processIndexBuffer(pElement);

	// Create the entity
	PhysObject* phys = new PhysObject;
	try
	{
		MeshManager::getSingleton().load(meshFile, m_sGroupName);
		phys->init(mSceneMgr,mWorld);
		if (!staticPhys)
		{
		phys->CreateObject(name,meshFile,pParent,100.0,false,"");
		}
		else
		{
			if (!box)
				phys->CreateStaticObject(name,meshFile,pParent);
			else
				phys->CreateStaticPhys_Box(name,meshFile,pParent);

		}
		phys->set_breakable(StringConverter::parseInt(health),StringConverter::parseInt(strength));
		phys->setGibModel(gibmeshFile);
		phys->setGibScale(gibScale);
		phys->setGibCount(c);
		phys->batt=battouch;
		if (explosive)
			phys->explosive();
		phys->ent->setCastShadows(castShadows);
		//bodies.push_back(phys->bod);
		if(!materialFile.empty())
			phys->ent->setMaterialName(materialFile);
		global::getSingleton().addPhysObject(phys);
	}
	catch(Ogre::Exception &/*e*/)
	{
		LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading a physics object!");
	}

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement, phys->ent);
}

void DotSceneLoader::processNewton(TiXmlElement *XMLNode)
{
	// Process attributes
	Vector3 min,max;
	min=Vector3(getAttribReal(XMLNode, "x1", -100000),getAttribReal(XMLNode, "y1", -100000),getAttribReal(XMLNode, "z1", -100000));
	max=Vector3(getAttribReal(XMLNode, "x2", 100000),getAttribReal(XMLNode, "y2", 100000),getAttribReal(XMLNode, "y2", 100000));
	mWorld->setWorldSize(min,max);
}

void DotSceneLoader::processPlayer(TiXmlElement *XMLNode)
{
	// Process attributes
	Vector3 pos;
	Real fov = getAttribReal(XMLNode, "fov", 45);
	Real multiplyer = getAttribReal(XMLNode, "mpr", 1);
	bool startFreeze = getAttribBool(XMLNode,"startFreeze",false);
	pos=parseVector3(XMLNode);
	Vector3 look_at = Vector3(getAttribReal(XMLNode, "lx", 0),getAttribReal(XMLNode, "ly", 0),getAttribReal(XMLNode, "lz", 0));
	//mCamera->setFOVy(Ogre::Degree(fov));
	player->set_position(pos*sceneMultiplier);
	if (startFreeze)
		player->freeze();
	if (!startFreeze)
		player->unfreeze();
	if (look_at != Vector3(0,0,0))
		player->look_at(look_at);

	player->multiply(multiplyer);
}

void DotSceneLoader::processSkyDome(TiXmlElement *XMLNode)
{
	// Process attributes
	String material = XMLNode->Attribute("material");
	Real curvature = getAttribReal(XMLNode, "curvature", 10);
	Real tiling = getAttribReal(XMLNode, "tiling", 8);
	Real distance = getAttribReal(XMLNode, "distance", 4000);
	bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);

	TiXmlElement *pElement;

	// Process rotation (?)
	Quaternion rotation = Quaternion::IDENTITY;
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
		rotation = parseQuaternion(pElement);

	// Setup the sky dome
	mSceneMgr->setSkyDome(true, material, curvature, tiling, distance, drawFirst, rotation, 16, 16, -1, m_sGroupName);
}

void DotSceneLoader::processSkyPlane(TiXmlElement *XMLNode)
{
	// Process attributes
	String material = getAttrib(XMLNode, "material");
	Real planeX = getAttribReal(XMLNode, "planeX", 0);
	Real planeY = getAttribReal(XMLNode, "planeY", -1);
	Real planeZ = getAttribReal(XMLNode, "planeX", 0);
	Real planeD = getAttribReal(XMLNode, "planeD", 5000);
	Real scale = getAttribReal(XMLNode, "scale", 1000);
	Real bow = getAttribReal(XMLNode, "bow", 0);
	Real tiling = getAttribReal(XMLNode, "tiling", 10);
	bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);

	// Setup the sky plane
	Plane plane;
	plane.normal = Vector3(planeX, planeY, planeZ);
	plane.d = planeD;
	mSceneMgr->setSkyPlane(true, plane, material, scale, tiling, drawFirst, bow, 1, 1, m_sGroupName);
}

void DotSceneLoader::processClipping(TiXmlElement *XMLNode)
{
	//! @todo Implement this

	// Process attributes
	Real fNear = getAttribReal(XMLNode, "near", 10);
	Real fFar = getAttribReal(XMLNode, "far", 10000);

	player->get_camera()->setNearClipDistance(fNear);
	player->get_camera()->setFarClipDistance(fFar);
}

void DotSceneLoader::processLightRange(TiXmlElement *XMLNode, Light *pLight)
{
	// Process attributes
	Real inner = getAttribReal(XMLNode, "inner");
	Real outer = getAttribReal(XMLNode, "outer");
	Real falloff = getAttribReal(XMLNode, "falloff", 1.0);

	// Setup the light range
	pLight->setSpotlightRange(Angle(inner), Angle(outer), falloff);
}

void DotSceneLoader::processLightAttenuation(TiXmlElement *XMLNode, Light *pLight)
{
	// Process attributes
	Real range = getAttribReal(XMLNode, "range");
	Real constant = getAttribReal(XMLNode, "constant");
	Real linear = getAttribReal(XMLNode, "linear");
	Real quadratic = getAttribReal(XMLNode, "quadratic");

	// Setup the light attenuation
	pLight->setAttenuation(range, constant, linear, quadratic);
}


String DotSceneLoader::getAttrib(TiXmlElement *XMLNode, const String &attrib, const String &defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return XMLNode->Attribute(attrib.c_str());
	else
		return defaultValue;
}

Real DotSceneLoader::getAttribReal(TiXmlElement *XMLNode, const String &attrib, Real defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return StringConverter::parseReal(XMLNode->Attribute(attrib.c_str()));
	else
		return defaultValue;
}

bool DotSceneLoader::getAttribBool(TiXmlElement *XMLNode, const String &attrib, bool defaultValue)
{
	if(!XMLNode->Attribute(attrib.c_str()))
		return defaultValue;

	if(String(XMLNode->Attribute(attrib.c_str())) == "true")
		return true;

	return false;
}


Vector3 DotSceneLoader::parseVector3(TiXmlElement *XMLNode)
{
	Quaternion orientation;

	if(XMLNode->Attribute("qx"))
	{
		orientation.x = StringConverter::parseReal(XMLNode->Attribute("qx"));
		orientation.y = StringConverter::parseReal(XMLNode->Attribute("qy"));
		orientation.z = StringConverter::parseReal(XMLNode->Attribute("qz"));
		orientation.w = StringConverter::parseReal(XMLNode->Attribute("qw"));
	}

	return orientation*Vector3(
		StringConverter::parseReal(XMLNode->Attribute("x")),
		StringConverter::parseReal(XMLNode->Attribute("y")),
		StringConverter::parseReal(XMLNode->Attribute("z"))
	)*getAttribReal(XMLNode,"m",1.0f);
}

Quaternion DotSceneLoader::parseQuaternion(TiXmlElement *XMLNode)
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

ColourValue DotSceneLoader::parseColour(TiXmlElement *XMLNode)
{
	return ColourValue(
		StringConverter::parseReal(XMLNode->Attribute("r")),
		StringConverter::parseReal(XMLNode->Attribute("g")),
		StringConverter::parseReal(XMLNode->Attribute("b")),
		XMLNode->Attribute("a") != NULL ? StringConverter::parseReal(XMLNode->Attribute("a")) : 1
	);
}

String DotSceneLoader::getProperty(const String &ndNm, const String &prop)
{
	for ( unsigned int i = 0 ; i < nodeProperties.size(); i++ )
	{
		if ( nodeProperties[i].nodeName == ndNm && nodeProperties[i].propertyNm == prop )
		{
			return nodeProperties[i].valueName;
		}
	}

	return "";
}

void DotSceneLoader::processUserDataReference(TiXmlElement *XMLNode, Entity *pEntity)
{
	String str = XMLNode->Attribute("id");
	pEntity->setUserAny(Any(str));
}
