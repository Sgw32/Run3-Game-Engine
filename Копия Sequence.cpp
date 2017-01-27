#include "Sequence.h"
#include "tinyxml.h"

template<> Sequence *Singleton<Sequence>::ms_Singleton=0;

Sequence::Sequence()
{
}

Sequence::~Sequence()
{
mRoot->removeFrameListener(this);
}

void Sequence::init(Ogre::Root* root,SceneManager* scene,SoundManager* sound)
{

}

void Sequence::SetSceneSeq(String seq)
{
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
		// Open the .scene File
		SequenceDoc = new TiXmlDocument();
		SequenceDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();

		if( SequenceDoc->Error() )
		{
			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[Sequence] The TiXmlDocument reported an error");
			return;
		}
	}
	catch(...)
	{
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[Sequence] Error creating TiXmlDocument");
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

void Sequence::processAdents(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("trigger");
	if(pElement)
		processTrigger(pElement);
}

void Sequence::processEvents(TiXmlElement *XMLNode)
{
	TiXmlElement *pElement;
	pElement = XMLNode->FirstChildElement("trigger");
	if(pElement)
		processTriggerEvent(pElement);
}

void Sequence::processTrigger(TiXmlElement *XMLNode)
{
	Real x1 = getAttribReal(XMLNode,"x1",0);
	Real x2 = getAttribReal(XMLNode,"x2",0);
	Real y1 = getAttribReal(XMLNode,"y1",0);
	Real y2 = getAttribReal(XMLNode,"y2",10);
	Real z1 = getAttribReal(XMLNode,"z1",10);
	Real z2 = getAttribReal(XMLNode,"z2",10);
	String name = getAttrib(XMLNode,"name","undefined"); 
	bool show = getAttribBool(XMLNode,"show",false);
	Trigger* trig = new Trigger(mRoot);
	trig->init(AxisAlignedBox(x1,y1,z1,x2,y2,z2),global::getSingleton().getPlayer());
	trig->show_box(show);
	triggers.push_back(trig);
	//triggers
}

void Sequence::processTriggerEvent(TiXmlElement *XMLNode)
{
	String name = getAttrib(XMLNode,"name","undefined");
	Real secs = getAttribReal(XMLNode,"sec",0.0f);
	Trigger* tTrig;
	int i;
	for (i=1; i!=triggers.size();i++)
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
			if(pElement)
			{
				String ent_name = getAttrib(pElement,"name","undefined");
				String ent_event = getAttrib(pElement,"event","undefined");
				String meshFile = getAttrib(pElement,"meshFile","undefined");
				Vector3 pos;
				/*global::getSingleton().entc_event = ent_event;
				global::getSingleton().entc_meshFile = meshFile;
				global::getSingleton().entc_name = ent_name;*/
				/*tTrig->entc_event = ent_event;
				tTrig->entc_meshFile = meshFile;
				tTrig->entc_name = ent_name;*/
							TiXmlElement *pPosition;
							pPosition = XMLNode->FirstChildElement("pos");	
							if (pPosition)
							{
								 pos = parseVector3(pPosition);
								//global::getSingleton().entc_name = ent_name;
							}
				tTrig->setCallbackEnt(ent_name,meshFile,"spawn",pos);
				//tTrig->setcallback(processEventEntc);
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
	if (!sequence.empty())
	{
		sec = sec+evt.timeSinceLastFrame;
		

		if (sec>1.0f)
			sec=0;
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
	return Vector3(
		StringConverter::parseReal(XMLNode->Attribute("x")),
		StringConverter::parseReal(XMLNode->Attribute("y")),
		StringConverter::parseReal(XMLNode->Attribute("z"))
	);
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

