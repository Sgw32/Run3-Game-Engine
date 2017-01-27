/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Trigger.h"

Trigger::Trigger(Root* mRoot)
{
	root=mRoot;
	scene=root->getSceneManagerIterator().getNext();
	sleep = 0;
	nowtrigger=false;
	callback_type = 0;
	std_callback = false;
	player_callback = false;
	entc_callback = false;
	changelevel_callback = false;
	door_callback = false;
	unloaded=false;
	hurt_callback=false;
	script_callback=false;
	enabled=true;
	state_before=false;
	state_now=false;
	footstep_inside="";
}

Trigger::~Trigger()
{

}

void Trigger::init(AxisAlignedBox aabb)
{
	type=1;
	objaabb=aabb;
	once=false;
	
}

void Trigger::addMultipleLightOn(Light* lt)
{
	mLightsOn[lt]=true;
}

void Trigger::addMultipleLightOff(Light* lt)
{
	mLightsOff[lt]=true;
}

void Trigger::processLightsOn()
{
	map<Light*,bool>::iterator it;
	for (it=mLightsOn.begin();it!=mLightsOn.end();it++)
	{
		(*it).second=(*it).first->getVisible();
		(*it).first->setVisible(true);
	}
}

void Trigger::processLightsOff()
{
	map<Light*,bool>::iterator it;
	for (it=mLightsOff.begin();it!=mLightsOff.end();it++)
	{
		(*it).second=(*it).first->getVisible();
		(*it).first->setVisible(false);
	}
}

void Trigger::restoreLightsOn()
{
	map<Light*,bool>::iterator it;
	for (it=mLightsOn.begin();it!=mLightsOn.end();it++)
	{
		(*it).first->setVisible((*it).second);
	}
}

void Trigger::restoreLightsOff()
{
	map<Light*,bool>::iterator it;
	for (it=mLightsOff.begin();it!=mLightsOff.end();it++)
	{
		(*it).first->setVisible((*it).second);
	}
}

void Trigger::init(AxisAlignedBox aabb,Player* ply)
{
	//root->addFrameListener(this);
	LogManager::getSingleton().logMessage(StringConverter::toString(aabb.getSize()));
	type=2;
	if (ply)
		tPlayer=ply;
	objaabb=aabb;
	trig = scene->createEntity(StringConverter::toString(objaabb.getMinimum())+StringConverter::toString(objaabb.getMaximum()),"box.mesh");
	trigsnap = scene->getRootSceneNode()->createChildSceneNode(objaabb.getCenter());
	trigsnap->attachObject(trig);
	trigsnap->setScale(objaabb.getSize());
	trigsnap->setVisible(false);
	once=false;
}

void Trigger::init(Vector3 pos, Vector3 scale,Player* ply)
{
	
AxisAlignedBox aabb(pos-scale/2,pos+scale/2);
LogManager::getSingleton().logMessage(StringConverter::toString(scale)+" "+StringConverter::toString(pos));
init(aabb,ply);
}


void Trigger::init(AxisAlignedBox aabb,PhysObject* phys)
{
	type=3;
	if (phys)
		obj=phys;
	objaabb=aabb;
	once=false;
}

void Trigger::setcallback(void (*callback)())
{
	func=callback;
	//callback_type = 0;
	std_callback = true;
}

void Trigger::ent(String name,String file, String event,Vector3 pos)
{
/////
	EventEntC::getSingleton().ent(name,file,event,pos);
}

void Trigger::setCallbackEnt(String name,String file, String event,Vector3 pos)
{
		entc_name.push_back(name);
		entc_meshFile.push_back(file);
		entc_event.push_back(event);
		entc_spawn.push_back(pos);
		entc_callback = true;
	/*Entity* ent = scene->createEntity(name,meshFile);
	Ogre::SceneNode* node = scene->getRootSceneNode()->createChildSceneNode(pos);
	node->attachObject(ent);*/
}

void Trigger::setCallbackPlayer(String event,Vector3 pos)
{
		player_spawn = pos;
		player_event = event;
		player_callback = true;
}

void Trigger::setCallbackChangeLevel(String map)
{
	changelevel_callback = true;
	mMap = map;
}
void Trigger::setname(String name)
{
	mName = name;
}

String Trigger::getname()
{
	return mName;
}

void Trigger::setsleep(Real sec)
{
	sleep = sec;
	sleep_i=sec;
}
void Trigger::show_box(bool show)
{
if (show)
	trigsnap->setVisible(true);
if (!show)
	trigsnap->setVisible(false);
}

void Trigger::interpretate_callback()
{
	if( enabled)
	{
				if (std_callback)
					func();
			if (hurt_callback)
				{
					nowtrigger=false;
							global::getSingleton().getPlayer()->bod->setDamage(damage);
				}
				if (!once) //trigger once callbacks
				{
						
						if (entc_callback)
						{

							for (i=0;i!=entc_name.size();i++)
							{
								ent(entc_name[i],entc_meshFile[i],entc_event[i],entc_spawn[i]);
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
						if (door_callback)
						{
							for (i=0;i!=doors.size();i++)
							{
								doors[i]->Fire(d_events[i]);
							}

						}
						if (script_callback)
						{
							for (i=0;i!=scripts.size();i++)
							{
								RunLuaScript(global::getSingleton().getLuaState(),scripts[i].c_str());
							}

						}
						once=true;
				}

				if (player_callback)
				{
					Vector3 pos = tPlayer->get_location();
					Vector3 size = tPlayer->get_cur_size();
					AxisAlignedBox plyaab = AxisAlignedBox (pos-size,pos+size);

					if (plyaab.intersects(objaabb))
					{
										if (player_event == "teleport")
										global::getSingleton().getPlayer()->teleport(player_spawn);
										once=false;
										sleep=sleep_i;
					}

				}

	}

}

bool Trigger::frameStarted(const Ogre::FrameEvent &evt)
{
	if (type==1)
	{
	/*switch (callback_type)
	{
	case 0:
		func();
		break;
	case 1:
		//ent(entc_name,entc_meshFile,entc_event,entc_spawn);
		break;*/
	}
	if (type==2)
	{
		if( enabled)
		{
		Vector3 pos = tPlayer->get_location();
		Vector3 size = tPlayer->get_cur_size();
		AxisAlignedBox plyaab = AxisAlignedBox (pos-size,pos+size);

		/*Vector3 minim = objaabb.getMinimum();
		Vector3 max = objaabb.getMaximum();*/

		/*if ((pos.y+size.y)<minim.y)
			return true;
		if ((pos.y-size.y)>max.y)
			return true;
		if ((pos.x+size.x)<minim.x)
			return true;
		if ((pos.x-size.x)>max.x)
			return true;
		if ((pos.z+size.z)<minim.z)
			return true;
		if ((pos.z-size.z)>max.z)
			return true;*/

		if (plyaab.intersects(objaabb))
		{
		nowtrigger=true;
		}

		if (nowtrigger)
		{
			if (!once)
			{
			sleep = sleep - evt.timeSinceLastFrame*TIME_SHIFT;
			}
			 if (sleep<0)
			{
				sleep=-1;
				interpretate_callback();
			}
		}
		}
	}

	if (type==3)
	{
	func();
	}

	if (type==4) //Trigger multiple
	{
		if( enabled)
		{
			Vector3 pos = tPlayer->get_location();
			Vector3 size = tPlayer->get_cur_size();
			//Hysteresys
			Real hyst = 4.0f*state_now+1.0f*(!state_now);
			AxisAlignedBox plyaab = AxisAlignedBox (pos-size*hyst,pos+size*hyst);
			state_now = plyaab.intersects(objaabb);

			if (!footstep_inside.empty())
			{
				if (state_now)
				{
					tPlayer->setCurrentFootstepPrefix(footstep_inside);
				}
				else
				{
					tPlayer->setCurrentFootstepPrefix("tile");
				}
			}
			
			//Process state change
			if ((state_now)&&(state_now!=state_before))
			{
				// Player entered the trigger
				processLightsOn();
				processLightsOff();
				if (!mLuaOnEnter.empty())
				{
				RunLuaScript(global::getSingleton().getLuaState(),mLuaOnEnter.c_str());
				}
			}
			if ((!state_now)&&(state_now!=state_before))
			{
				// Player exited the trigger
				restoreLightsOn();
				restoreLightsOff();
				if (!mLuaOnLeave.empty())
				{
				RunLuaScript(global::getSingleton().getLuaState(),mLuaOnLeave.c_str());
				}
			}
			state_before=state_now;
		}
	}

	return true;
}

void Trigger::setCallbackDoor(func_door* door,String event)
{
	doors.push_back(door);
	d_events.push_back(event);
	door_callback = true;
}

void Trigger::setCallbackScript(String script)
{
	scripts.push_back(script);
	script_callback = true;
}


void Trigger::unload()
{
//	root->removeFrameListener(this);
	trigsnap->detachAllObjects();
	scene->destroySceneNode(trigsnap);
	scene->destroyEntity(trig);
}
