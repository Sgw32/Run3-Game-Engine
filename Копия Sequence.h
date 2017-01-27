///////////////////////////////////////////////////////////////////////
//		Sequnce class for Run3										 //
//		Started 25.02.2010											 //
//		by Sgw32													 //
//		use for implementing buttons,npcs,etc						 //
///////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "Player.h"
#include "PhysObject.h"
#include "SoundManager.h"
#include "tinyxml.h"
#include "Trigger.h"
#include "global.h"


class Sequence:public FrameListener, Singleton<Sequence>
{
public:
   Sequence();
   ~Sequence();
   void init(Ogre::Root *root,SceneManager* sceneMgr,SoundManager* sound);
   void SetSceneSeq(String seq);
   void interpretate(String obj,String command);
	//2 main categories
   void processAdents(TiXmlElement *XMLNode);
   void processEvents(TiXmlElement *XMLNode);
	//in <adents>
   void processTrigger(TiXmlElement *XMLNode);
	//in <events>
	 void processTriggerEvent(TiXmlElement *XMLNode);
	 static void Gluk();
	static void processEventEntc();

   //utilites
   	String getAttrib(TiXmlElement *XMLNode, const String &parameter, const String &defaultValue = "");
	Real getAttribReal(TiXmlElement *XMLNode, const String &parameter, Real defaultValue = 0);
	bool getAttribBool(TiXmlElement *XMLNode, const String &parameter, bool defaultValue = false);

	Vector3 parseVector3(TiXmlElement *XMLNode);
	Quaternion parseQuaternion(TiXmlElement *XMLNode);
	ColourValue parseColour(TiXmlElement *XMLNode);

   virtual bool frameStarted(const Ogre::FrameEvent &evt);
   virtual bool frameEnded(const Ogre::FrameEvent &evt);

   vector<Trigger*> triggers;
   vector<String> events;
private:
	Ogre::Root* mRoot;
	Real sec;
	String sequence;
	SceneManager* mSceneMgr;
	SoundManager* mSoundManager;
	TiXmlDocument   *SequenceDoc;
	TiXmlElement   *SequenceRoot;
};