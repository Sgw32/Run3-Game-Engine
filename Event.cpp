/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#include "Event.h"

Event::Event(Ogre::Root* root,bool on,String name)
{
	root->addFrameListener(this);
	mRoot = root;
	once=on;
	player_callback = false;
	entc_callback = false;
	changelevel_callback = false;
	door_callback = false;
	unloaded=false;
	once=false;
	cutscene_callback=false;
	script_callback=false;
	started=false;
	mName=name;
	pLuaState=global::getSingleton().getLuaState();
	timeAfterrun=0;
}

Event::~Event()
{
	
}

void Event::setStartCutScene(CutScene* cscene)
{
	cutscene_callback=true;
	mCscene=cscene;
}

void Event::setChangelevel(Ogre::String map)
{
	changelevel_callback = true;
	mMap = map;
}

void Event::setPlayer(Vector3 dest,Real secs,String event)
{
	player_spawn = dest;
	player_event = event;
	player_callback = true;
	playerwait=secs;
	if (playerwait>maxwait)
		maxwait=playerwait;
}

void Event::setSpawnPhys(String name,String fileName,Vector3 dest, Real secs)
{
		entc_name.push_back(name);
		entc_meshFile.push_back(fileName);
		entc_secs.push_back(secs);
		entc_spawn.push_back(dest);
		entc_callback = true;
		if (secs>maxwait)
			maxwait=secs;
}

void Event::setOpenDoor(func_door* door,String event,Real secs)
{
	doors.push_back(door);
	d_events.push_back(event);
	door_callback = true;
	d_secs.push_back(secs);
	if (secs>maxwait)
			maxwait=secs;
}

void Event::trigger()
{
		started=true;
		timeAfterrun=0;
		l_secs=l_secs2;
}

void Event::interpretate_callback()
{

						if (entc_callback)
						{

							for (i=0;i!=entc_name.size();i++)
							{
								if (entc_secs[i]>timeAfterrun)
									break;
								PhysObject* phys = new PhysObject;
								SceneNode* newNode = global::getSingleton().getSceneManager()->getRootSceneNode()->createChildSceneNode(entc_spawn[i]);
								try
								{
									phys->init(global::getSingleton().getSceneManager(),global::getSingleton().getWorld());
									phys->CreateObject(entc_name[i],entc_meshFile[i],newNode,100.0,false,"");
									global::getSingleton().addPhysObject(phys);
								}
								catch(Ogre::Exception &/*e*/)
								{
									LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading a physics object!");
								}
								//ent(entc_name[i],entc_meshFile[i],entc_event[i],entc_spawn[i]);
							}
						}
						if (changelevel_callback)
						{
							if (!unloaded)
							{
								global::getSingleton().changemap_now = true;
								global::getSingleton().mMap = this->mMap;
								unloaded=true;
							}
						}
						if (script_callback)
						{
							for (i=0;i!=luascripts.size();i++)
							{
								//LogManager::getSingleton().logMessage(StringConverter::toString(timeAfterrun));
								//LogManager::getSingleton().logMessage(luascripts[i]);
								if (l_secs[i]<timeAfterrun)
								{
								l_secs[i]=maxwait+1;
								RunLuaScript(pLuaState,luascripts[i].c_str());
								}
							}
						}
						if (door_callback)
						{
							for (i=0;i!=doors.size();i++)
							{
								if (d_secs[i]>timeAfterrun)
									break;
								doors[i]->Fire(d_events[i]);
							}

						}

				if (player_callback)
				{
					if (!(playerwait>timeAfterrun))
					{
										global::getSingleton().getPlayer()->teleport(player_spawn);
					}
				}

				if (cutscene_callback)
				{
					mCscene->start();
				}

}

void Event::dispose()
{
	started=false;
	mRoot->removeFrameListener(this);
}

bool Event::frameStarted(const Ogre::FrameEvent &evt)
{
	if (started)
	{
		timeAfterrun=timeAfterrun+evt.timeSinceLastFrame;
		interpretate_callback();
		if (timeAfterrun>maxwait)
			started=false;
	}
	return true;
}

void Event::addLuaScript2(String script, Real secs)
{
		script_callback=true;
		luascripts.push_back(script);
		l_secs.push_back(secs);
		l_secs2.push_back(secs);
		if (secs>maxwait)
			maxwait=secs;
}