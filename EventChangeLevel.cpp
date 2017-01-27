/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "EventChangeLevel.h"

template<> EventChangeLevel *Singleton<EventChangeLevel>::ms_Singleton=0;

EventChangeLevel::EventChangeLevel()
{

}

EventChangeLevel::~EventChangeLevel()
{

}

void EventChangeLevel::init(SceneManager* scene)
{
	mSceneMgr = scene;
}

void EventChangeLevel::fire(String map)
{
	LoadMap::getSingleton().LoadM("/" + map,"MAP","/scripts","run3/maps","General",mSceneMgr);
}