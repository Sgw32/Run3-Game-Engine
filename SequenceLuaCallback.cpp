#include "SequenceLuaCallback.h"
#include "Sequence.h"
#include "LoadMap.h"

template<> SequenceLuaCallback *Ogre::Singleton<SequenceLuaCallback>::ms_Singleton=0;

SequenceLuaCallback::SequenceLuaCallback()
{
	pDoor="";
}

SequenceLuaCallback::~SequenceLuaCallback()
{

}

void SequenceLuaCallback::startCutScene(String name)
{
	Sequence::getSingleton().runCutScene(name);
}

void SequenceLuaCallback::processDotScene(String name)
{
	LoadMap::getSingleton().MergeM(name);
}