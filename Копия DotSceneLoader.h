#ifndef DOT_SCENELOADER_H
#define DOT_SCENELOADER_H

// Includes
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <vector>
#include "PhysObject.h"
#include "SoundManager.h"
#include "Player.h"
// Forward declarations
class TiXmlElement;

namespace Ogre
{
	// Forward declarations
	class SceneManager;
	class SceneNode;

	class nodeProperty
	{
	public:
		String nodeName;
		String propertyNm;
		String valueName;
		String typeName;

		nodeProperty(const String &node, const String &propertyName, const String &value, const String &type)
			: nodeName(node), propertyNm(propertyName), valueName(value), typeName(type) {}
	};

	class DotSceneLoader
	{
	public:
		DotSceneLoader() : mSceneMgr(0) {}
		virtual ~DotSceneLoader() {}

		void parseDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode = NULL, const String &sPrependNode = "",OgreNewt::World *World = NULL,Camera *mCamera = NULL,SoundManager* soundMgr = NULL,Player* ply = NULL);
		String getProperty(const String &ndNm, const String &prop);
		std::string _tostr(float a);

		std::vector<nodeProperty> nodeProperties;
		std::vector<String> staticObjects;
		std::vector<String> dynamicObjects;
		std::vector<unsigned int> sounds;
	protected:
		void processScene(TiXmlElement *XMLRoot);

		void processNodes(TiXmlElement *XMLNode);
		void processExternals(TiXmlElement *XMLNode);
		void processEnvironment(TiXmlElement *XMLNode);
		void processSounds(TiXmlElement *XMLNode);	//added by Sgw32
		void processTerrain(TiXmlElement *XMLNode);
		void processUserDataReference(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processUserDataReference(TiXmlElement *XMLNode, Entity *pEntity);
		void processOctree(TiXmlElement *XMLNode);
		void processLight(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processCamera(TiXmlElement *XMLNode, SceneNode *pParent = 0);
			
		void processNode(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processLookTarget(TiXmlElement *XMLNode, SceneNode *pParent);
		void processTrackTarget(TiXmlElement *XMLNode, SceneNode *pParent);
		void processEntity(TiXmlElement *XMLNode, SceneNode *pParent);	//modified by Sgw32
		void processPhysObj(TiXmlElement *XMLNode, SceneNode *pParent);	//added by Sgw32
		void processNewton(TiXmlElement *XMLNode);	//added by Sgw32
		void processParticleSystem(TiXmlElement *XMLNode, SceneNode *pParent);
		void processBillboardSet(TiXmlElement *XMLNode, SceneNode *pParent);
		void processPlane(TiXmlElement *XMLNode, SceneNode *pParent);
		void processPlayer(TiXmlElement *XMLNode);	//added by Sgw32
		void processSound(TiXmlElement *XMLNode);	//added by Sgw32

		void processFog(TiXmlElement *XMLNode);
		void processSkyBox(TiXmlElement *XMLNode);
		void processSkyDome(TiXmlElement *XMLNode);
		void processSkyPlane(TiXmlElement *XMLNode);
		void processClipping(TiXmlElement *XMLNode);

		void processLightRange(TiXmlElement *XMLNode, Light *pLight);
		void processLightAttenuation(TiXmlElement *XMLNode, Light *pLight);

		String getAttrib(TiXmlElement *XMLNode, const String &parameter, const String &defaultValue = "");
		Real getAttribReal(TiXmlElement *XMLNode, const String &parameter, Real defaultValue = 0);
		bool getAttribBool(TiXmlElement *XMLNode, const String &parameter, bool defaultValue = false);

		Vector3 parseVector3(TiXmlElement *XMLNode);
		Quaternion parseQuaternion(TiXmlElement *XMLNode);
		ColourValue parseColour(TiXmlElement *XMLNode);
		

		SceneManager *mSceneMgr;
		Camera* mCamera;
		Player* player;
		SoundManager* sound;
		OgreNewt::World *mWorld;
		SceneNode *mAttachNode;
		String m_sGroupName;
		String m_sPrependNode;
	};
}

#endif // DOT_SCENELOADER_H
