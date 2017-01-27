#include "DotSceneLoader.h"
#include "tinyxml.h"

#include <Ogre.h>

using namespace std;
using namespace Ogre;

void DotSceneLoader::parseDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode, const String &sPrependNode,OgreNewt::World *World,Camera* camera,SoundManager* soundMgr,Player* ply)
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
	staticObjects.clear();
	dynamicObjects.clear();

	TiXmlDocument   *XMLDoc = 0;
	TiXmlElement   *XMLRoot;

	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename(SceneName, basename, path);

		DataStreamPtr pStream = ResourceGroupManager::getSingleton().
			openResource( basename, groupName );

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

	// figure out where to attach any nodes we create
	mAttachNode = pAttachNode;
	if(!mAttachNode)
		mAttachNode = mSceneMgr->getRootSceneNode();

	// Process the scene
	processScene(XMLRoot);

	// Close the XML File
	delete XMLDoc;
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

	// Process externals (?)
	pElement = XMLRoot->FirstChildElement("externals");
	if(pElement)
		processExternals(pElement);

	// Process environment (?)
	pElement = XMLRoot->FirstChildElement("environment");
	if(pElement)
		processEnvironment(pElement);

	// Process sounds (?)
	pElement = XMLRoot->FirstChildElement("sounds");
	if(pElement)
		processSounds(pElement);

	// Process terrain (?)
	pElement = XMLRoot->FirstChildElement("terrain");
	if(pElement)
		processTerrain(pElement);

	// Process userDataReference (?)
	pElement = XMLRoot->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement);

	// Process octree (?)
	pElement = XMLRoot->FirstChildElement("octree");
	if(pElement)
		processOctree(pElement);

	// Process light (?)
	pElement = XMLRoot->FirstChildElement("light");
	if(pElement)
		processLight(pElement);

	// Process camera (?)
	pElement = XMLRoot->FirstChildElement("camera");
	if(pElement)
		processCamera(pElement);
}

void DotSceneLoader::processNodes(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;

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

	// Process colourAmbient (?)
	pElement = XMLNode->FirstChildElement("colourAmbient");
	if(pElement)
		mSceneMgr->setAmbientLight(parseColour(pElement));

	// Process colourBackground (?)
	//! @todo Set the background colour of all viewports (RenderWindow has to be provided then)
	pElement = XMLNode->FirstChildElement("colourBackground");
	if(pElement)
		;//mSceneMgr->set(parseColour(pElement));

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		processUserDataReference(pElement);
}

void DotSceneLoader::processSounds(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;

	// Process skyBox (?)
	pElement = XMLNode->FirstChildElement("sound");
	if(pElement)
		processSound(pElement);
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

void DotSceneLoader::processLight(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");

	// Create the light
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

	TiXmlElement *pElement;

	// Process position (?)
	pElement = XMLNode->FirstChildElement("position");
	if(pElement)
		pLight->setPosition(parseVector3(pElement));

	// Process normal (?)
	pElement = XMLNode->FirstChildElement("normal");
	if(pElement)
		pLight->setDirection(parseVector3(pElement));

	// Process colourDiffuse (?)
	pElement = XMLNode->FirstChildElement("colourDiffuse");
	if(pElement)
		pLight->setDiffuseColour(parseColour(pElement));

	// Process colourSpecular (?)
	pElement = XMLNode->FirstChildElement("colourSpecular");
	if(pElement)
		pLight->setSpecularColour(parseColour(pElement));

	// Process lightRange (?)
	/*pElement = XMLNode->FirstChildElement("lightRange");
	if(pElement)
		processLightRange(pElement, pLight);*/

	// Process lightAttenuation (?)
	pElement = XMLNode->FirstChildElement("lightAttenuation");
	if(pElement)
		processLightAttenuation(pElement, pLight);

	// Process userDataReference (?)
	pElement = XMLNode->FirstChildElement("userDataReference");
	if(pElement)
		;//processUserDataReference(pElement, pLight);
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
	if(name.empty())
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
		pNode->setPosition(parseVector3(pElement));
		pNode->setInitialState();
	}

	// Process rotation (?)
	pElement = XMLNode->FirstChildElement("rotation");
	if(pElement)
	{
		pNode->setOrientation(parseQuaternion(pElement));
		pNode->setInitialState();
	}

	// Process scale (?)
	pElement = XMLNode->FirstChildElement("scale");
	if(pElement)
	{
		pNode->setScale(parseVector3(pElement));
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

void DotSceneLoader::processEntity(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String materialFile = getAttrib(XMLNode, "materialFile");
	Real scalex = getAttribReal(XMLNode,"scaleU",1);
	Real scaley = getAttribReal(XMLNode,"scaleV",1);
	bool isStatic = getAttribBool(XMLNode, "static", false);;
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);

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
	PhysObject* phys = new PhysObject;
	try
	{
		MeshManager::getSingleton().load(meshFile, m_sGroupName);
		phys->init(mSceneMgr,mWorld);
		phys->CreateStaticObject(name,meshFile,pParent);
		phys->ent->setCastShadows(castShadows);
		//bodies.push_back(phys->bod);
		if(!materialFile.empty())
		{
			Real scalex = getAttribReal(XMLNode,"scaleU",1);
			Real scaley = getAttribReal(XMLNode,"scaleV",1);
			MaterialPtr mat = (MaterialPtr)MaterialManager::getSingleton().getByName(materialFile);
			/*Ogre::Real s;
			s=(Ogre::Real)mat->getTechnique(0)->getPass(0)->getPointSize();
			String str1=_tostr(s/phys->get_z());
			String str2=_tostr(s/phys->get_x());
			mat->setParameter("scale",str1+" "+str2);*/
			mat->getTechnique(0)-> getPass(0)->getTextureUnitState(0)->setTextureScale(scalex,scaley);
			phys->ent->setMaterial(mat);
		}
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

void DotSceneLoader::processPhysObj(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String meshFile = getAttrib(XMLNode, "meshFile");
	String materialFile = getAttrib(XMLNode, "materialFile");
	Ogre::Real mass = getAttribReal(XMLNode, "mass", 10.0);
	bool castShadows = getAttribBool(XMLNode, "castShadows", true);
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
		phys->CreateObject(name,meshFile,pParent,100.0);
		phys->ent->setCastShadows(castShadows);
		//bodies.push_back(phys->bod);
		if(!materialFile.empty())
			phys->ent->setMaterialName(materialFile);
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


void DotSceneLoader::processParticleSystem(TiXmlElement *XMLNode, SceneNode *pParent)
{
	// Process attributes
	String name = getAttrib(XMLNode, "name");
	String id = getAttrib(XMLNode, "id");
	String file = getAttrib(XMLNode, "file");

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
	unsigned int audioId;
	String soundpath = "Run3/sounds/";
	String name = soundpath+"/"+getAttrib(XMLNode, "name");
	bool loop = getAttribBool(XMLNode, "loop",false);
	Vector3 pos = parseVector3(XMLNode);
	sound->loadAudio(name,&audioId,loop);
	sound->setSoundPosition(audioId,pos,Vector3::ZERO,Vector3::ZERO);
	//sound->setSoundPosition(audioId,Vector3(10,100,10),Vector3::ZERO,Vector3::ZERO);
	sound->playAudio(audioId,true);
	sounds.push_back(audioId);
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

void DotSceneLoader::processNewton(TiXmlElement *XMLNode)
{
	// Process attributes
	Vector3 min,max;
	min=Vector3(getAttribReal(XMLNode, "x1", -1000),getAttribReal(XMLNode, "y1", -1000),getAttribReal(XMLNode, "z1", -1000));
	max=Vector3(getAttribReal(XMLNode, "x2", 1000),getAttribReal(XMLNode, "y2", 1000),getAttribReal(XMLNode, "y2", 1000));
	mWorld->setWorldSize(min,max);
}

void DotSceneLoader::processPlayer(TiXmlElement *XMLNode)
{
	// Process attributes
	Vector3 pos;
	Real fov = getAttribReal(XMLNode, "fov", 45);
	pos=parseVector3(XMLNode);
	mCamera->setFOVy(Ogre::Degree(fov));
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
	Real fNear = getAttribReal(XMLNode, "near", 0);
	Real fFar = getAttribReal(XMLNode, "far", 1);
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
	return Vector3(
		StringConverter::parseReal(XMLNode->Attribute("x")),
		StringConverter::parseReal(XMLNode->Attribute("y")),
		StringConverter::parseReal(XMLNode->Attribute("z"))
	);
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
