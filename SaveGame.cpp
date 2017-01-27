#include "SaveGame.h"
#include "global.h"

template<> SaveGame *Singleton<SaveGame>::ms_Singleton=0;

SaveGame::SaveGame()
{
	first=true;
}

SaveGame::~SaveGame()
{
}

void SaveGame::init()
{

}

void SaveGame::saveLast(String filename)
{
	if (first)
	{
		first=false;
		return;
	}
	try
	{
	TiXmlDocument doc;

	TiXmlElement * scene = new TiXmlElement( "savefile" );
	//scene attrib setup
	loadRegisters();

	//Store Run3 registers with useful data to the savefile.
	scene->SetAttribute("rx1",StringConverter::toString(rx1).c_str());
	scene->SetAttribute("rx2",StringConverter::toString(rx2).c_str());
	scene->SetAttribute("ry1",StringConverter::toString(ry1).c_str());
	scene->SetAttribute("ry2",StringConverter::toString(ry2).c_str());
	scene->SetAttribute("rz1",StringConverter::toString(rz1).c_str());
	scene->SetAttribute("rz2",StringConverter::toString(rz2).c_str());
	scene->SetAttribute("a",StringConverter::toString(a).c_str());
	scene->SetAttribute("ra",StringConverter::toString(ra).c_str());
	scene->SetAttribute("rb",StringConverter::toString(rb).c_str());
	scene->SetAttribute("rc",StringConverter::toString(rc).c_str());
	
	scene->SetAttribute("mulh",StringConverter::toString(
		global::getSingleton().getPlayer()->getRegenerationMultiplier()).c_str());
	scene->SetAttribute("maxh",StringConverter::toString(
		global::getSingleton().getPlayer()->getRegenerationHealth()).c_str());
	scene->SetAttribute("map",filename.c_str());
	doc.LinkEndChild( scene );
	doc.SaveFile( "run3/game/savefile.xml" );
	}
	catch(...)
	{
		LogManager::getSingleton().logMessage("[SaveFile] Save Error. TiXml reporated an error.");
	}
//	delete doc;
}

void SaveGame::loadRegisters()
{
	this->rx1=global::getSingleton().rx1;
	this->rx2=global::getSingleton().rx2;
	this->ry1=global::getSingleton().ry1;
	this->ry2=global::getSingleton().ry2;
	this->rz1=global::getSingleton().rz1;
	this->rz2=global::getSingleton().rz2;
	this->a=global::getSingleton().a;
	this->ra=global::getSingleton().ra;
	this->rb=global::getSingleton().rb;
	this->rc=global::getSingleton().rc;
}

void SaveGame::restoreRegisters()
{
	global::getSingleton().rx1=this->rx1;
	global::getSingleton().rx2=this->rx2;
	global::getSingleton().ry1=this->ry1;
	global::getSingleton().ry2=this->ry2;
	global::getSingleton().rz1=this->rz1;
	global::getSingleton().rz2=this->rz2;
	global::getSingleton().a=this->a;
	global::getSingleton().ra=this->ra;
	global::getSingleton().rb=this->rb;
	global::getSingleton().rc=this->rc;
}

String SaveGame::openLast()
{
	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename("run3/game/savefile.xml", basename, path);

		DataStreamPtr pStream = ResourceGroupManager::getSingleton().
			openResource( basename, "General" );

		String data = pStream->getAsString();
		// Open the .xml File
		SequenceDoc = new TiXmlDocument();
		SequenceDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();
		
		if( SequenceDoc->Error() )
		{
			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[SaveFile] TiXml reporated an error.");
			return "";
		}
	}
	catch(...)
	{
		//rest=true;
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[SaveFile] SaveFile does not present,or TiXml reporated an error.");
		return "";
	}

	SequenceRoot = SequenceDoc->RootElement();
	if( String( SequenceRoot->Value()) != "savefile"  ) {
		LogManager::getSingleton().logMessage( "[SaveFile] Error: Invalid .xml File. Missing <savefile>" );     
		return "";
	}
	String out = SequenceRoot->Attribute("map");
	LogManager::getSingleton().logMessage("[SaveFile] "+out);

	this->rx1=StringConverter::parseReal(SequenceRoot->Attribute("rx1"));
	this->rx2=StringConverter::parseReal(SequenceRoot->Attribute("rx2"));
	this->ry1=StringConverter::parseReal(SequenceRoot->Attribute("ry1"));
	this->ry2=StringConverter::parseReal(SequenceRoot->Attribute("ry2"));
	this->rz1=StringConverter::parseReal(SequenceRoot->Attribute("rz1"));
	this->rz2=StringConverter::parseReal(SequenceRoot->Attribute("rz2"));
	this->a=StringConverter::parseReal(SequenceRoot->Attribute("a"));
	this->ra=StringConverter::parseReal(SequenceRoot->Attribute("ra"));
	this->rb=StringConverter::parseReal(SequenceRoot->Attribute("rb"));
	this->rc=StringConverter::parseReal(SequenceRoot->Attribute("rc"));

	global::getSingleton().getPlayer()->setHealthRegeneration(StringConverter::parseReal(SequenceRoot->Attribute("mulh")),
		StringConverter::parseReal(SequenceRoot->Attribute("maxh")));

	restoreRegisters();

	delete SequenceDoc;
	return out;
}

void SaveGame::restoreHealthParameters(Real mul, Real maxHealth)
{
	global::getSingleton().getPlayer()->setHealthRegeneration(mul,maxHealth);
}


/*void SaveGame::cleanup()
{
	
}*/