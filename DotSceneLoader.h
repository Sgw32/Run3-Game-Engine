/////////////////////////////////////////////////////////////////////
///////////////Modified by:Fyodor Zagumennov aka Sgw32     //////////
///////////////DotScene xTended by Sgw32, including physobjs,////////
///////////////Player and Sounds/////////////////////////////////////
/////////////////////////////////////////////////////////////////////
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
#include "FadeListener.h"
#include "global.h"
#include "AIManager.h"
#include "HUD.h"
#include "Ragdoll.h"
#include "Loader.h"
#include "MirrorManager.h"
#include "CaduneTree.h"
#include "MusicPlayer.h"
#include "WaterManager.h"
#include "MLight.h"
#include "LightPerfomanceManager.h"
#include "LightFlasher.h"
#include "Run3Batcher.h"
#include "Fire.h"
#include "ZonePortalManager.h"
#include "SkyManager.h"
#include "SoftwareOcclusionCulling.h"
// Forward declarations

class TiXmlElement;
using namespace CaduneTree;

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
		DotSceneLoader() : mSceneMgr(0) {levelofDetailM=0;levelofDetailT=0;}
		virtual ~DotSceneLoader() {}

		void parseDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode = NULL, const String &sPrependNode = "",OgreNewt::World *World = NULL,Camera *mCamera = NULL,SoundManager* soundMgr = NULL,Player* ply = NULL);
		void mergeDotScene(const String &SceneName, const String &groupName, SceneManager *yourSceneMgr, SceneNode *pAttachNode = NULL, const String &sPrependNode = "",OgreNewt::World *World = NULL,Camera *mCamera = NULL,SoundManager* soundMgr = NULL,Player* ply = NULL);
		String getProperty(const String &ndNm, const String &prop);
		void refreshObjs()
		{
			std::vector<RagDoll*>::iterator i;
			for (i=ragdolls.begin();i!=ragdolls.end();i++)
			{
				RagDoll* rd = (*i);
				SceneNode* n = rd->getSceneNode();
				//mSceneMgr->destroySceneNode(n);
				delete rd;
			}
			ragdolls.clear();
			LogManager::getSingleton().logMessage("DESTROYING MOS");
			for (unsigned int m=0;m!=trunks2.size();m++){
			LogManager::getSingleton().logMessage("DESTROYING MOS2");
			if (mSceneMgr->hasManualObject(trunks2[m]->getName()))
			{
				LogManager::getSingleton().logMessage(trunks2[m]->getName());
				mSceneMgr->destroyManualObject(trunks2[m]);
			}
			}

			LogManager::getSingleton().logMessage("DESTROYING BS");
			for (unsigned int m=0;m!=trunks3.size();m++){
			if (mSceneMgr->hasBillboardSet(trunks3[m]->getName()))
				mSceneMgr->destroyBillboardSet(trunks3[m]);
			}
trunks2.clear();
trunks3.clear();
			LogManager::getSingleton().logMessage("DESTROYING TRUNKS");
			std::vector<Stem*>::iterator j;
			for (j=trunks.begin();j!=trunks.end();j++)
			{
				Stem* rd = (*j);

				delete rd;
			}
			trunks.clear();
			

			std::vector<Controller<Real>*>::iterator i1;
			std::vector<ControllerFunctionRealPtr>::iterator i2;
			std::vector<ControllerValueRealPtr>::iterator i3;

			for (i1=lctrl.begin();i1!=lctrl.end();i1++)
			{
				ControllerManager::getSingleton().destroyController(*i1);
			}
			lctrl.clear();
			LogManager::getSingleton().logMessage("LightFlashers: destroyed!");
			for (i2=lctrlf.begin();i2!=lctrlf.end();i2++)
			{
				(*i2).setNull();
			}
			lctrlf.clear();

			for (i3=lctrlv.begin();i3!=lctrlv.end();i3++)
			{
				(*i3).setNull();
			}
			lctrlv.clear();

			std::vector<Fire*>::iterator i4;
			for (i4=fires.begin();i4!=fires.end();i4++)
			{
				(*i4)->destroy();
				delete (*i4);
			}		
			fires.clear();
			/*std::vector<OgreNewt:
			:Body*>::iterator m;
			for (m=trunks4.begin();m!=trunks4.end();m++)
			{
				OgreNewt::Body* rd = (*m);

				delete rd;
			}
			trunks4.clear();*/
		}
	
		void util_processRagDoll(String name,String script,String meshFile, SceneNode *pParent);
		void util_deleteRagDoll(String name);
		std::string _tostr(float a);

		std::vector<nodeProperty> nodeProperties;
		std::vector<String> staticObjects;
		std::vector<String> dynamicObjects;
		//std::vector<OgreNewt::Body*> trunks4;
		std::vector<unsigned int> sounds;
		std::vector<RagDoll*> ragdolls;
		std::vector<Fire*> fires;
		int texture_copy_counter;
		Real sceneMultiplier;
	protected:
		void processScene(TiXmlElement *XMLRoot);

		void processNodes(TiXmlElement *XMLNode);
		void processExternals(TiXmlElement *XMLNode);
		void processEnvironment(TiXmlElement *XMLNode);
		void processNPCNodes(TiXmlElement* XMLNode); //added by Sgw32
		void processSounds(TiXmlElement *XMLNode);	//added by Sgw32
		void processTerrain(TiXmlElement *XMLNode);
		void processUserDataReference(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processUserDataReference(TiXmlElement *XMLNode, Entity *pEntity);
		void processOctree(TiXmlElement *XMLNode);
		void processLight(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processCamera(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		
		void processNPCNode(TiXmlElement *XMLNode); //added by Sgw32
		void processNode(TiXmlElement *XMLNode, SceneNode *pParent = 0);
		void processLookTarget(TiXmlElement *XMLNode, SceneNode *pParent);
		void processTrackTarget(TiXmlElement *XMLNode, SceneNode *pParent);
		void processEntity(TiXmlElement *XMLNode, SceneNode *pParent);	//modified by Sgw32
		void processPhysObj(TiXmlElement *XMLNode, SceneNode *pParent);	//added by Sgw32
		void processNoCollide(TiXmlElement *XMLNode, SceneNode *pParent);	//added by Sgw32
		void processBreakable(TiXmlElement *XMLNode, SceneNode *pParent);	//added by Sgw32
		//void processWood(TiXmlElement *XMLNode, SceneNode *pParent);	//added by Sgw32
		void processPhysBlock(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		void processPhysBox(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		void processPhysCyl(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		
		void processRagDoll(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		void processTree(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		void processMusic(TiXmlElement *XMLNode); //added by Sgw32
		void processWater(TiXmlElement *XMLNode); //added by Sgw32
		void processSkyX(TiXmlElement *XMLNode); //added by Sgw32
		void processFire(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32
		
		void processZone(TiXmlElement *XMLNode); //added by Sgw32
		void processPortal(TiXmlElement *XMLNode); //added by Sgw32
		void processPortals(TiXmlElement *XMLNode); //added by Sgw32

		void processNewton(TiXmlElement *XMLNode);	//added by Sgw32
		void processParticleSystem(TiXmlElement *XMLNode, SceneNode *pParent);
		void processBillboardSet(TiXmlElement *XMLNode, SceneNode *pParent);
		void processPlane(TiXmlElement *XMLNode, SceneNode *pParent);
		void processPlayer(TiXmlElement *XMLNode);	//added by Sgw32
		void processSound(TiXmlElement *XMLNode);	//added by Sgw32
		void processAmbient(TiXmlElement *XMLNode);	//added by Sgw32
		void processMirror(TiXmlElement *XMLNode, SceneNode *pParent); //added by Sgw32

		void processFog(TiXmlElement *XMLNode);
		void processSkyBox(TiXmlElement *XMLNode);
		void processSkyDome(TiXmlElement *XMLNode);
		void processSkyPlane(TiXmlElement *XMLNode);
		void processClipping(TiXmlElement *XMLNode);
		void processFade(TiXmlElement *XMLNode);	//added by Sgw32
		void processShowHUD(TiXmlElement *XMLNode); //added by Sgw32

		void processLightRange(TiXmlElement *XMLNode, Light *pLight);
		void processLightAttenuation(TiXmlElement *XMLNode, Light *pLight);
		void processLightDynamics(TiXmlElement *XMLNode, Light *pLight); //added by Sgw32

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
		int levelofDetailM;
		int levelofDetailT;
		Ogre::Root* root;
		std::vector<Stem*> trunks;
		std::vector<ManualObject*> trunks2;
		std::vector<BillboardSet*> trunks3;

		std::vector<Controller<Real>*> lctrl;
		std::vector<ControllerFunctionRealPtr> lctrlf;
		std::vector<ControllerValueRealPtr> lctrlv;
	};
}

#endif // DOT_SCENELOADER_H
