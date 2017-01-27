#pragma once

#include "Ogre.h"
#include "global.h"

using namespace Ogre;
using namespace std;

/*class ZonePortal
{
public:
	ZonePortal();
	~ZonePortal();
};*/


class ZonePortal
{
public:
	inline ZonePortal(){};
	inline ZonePortal(AxisAlignedBox zonePortal){mZonePortal=zonePortal;}
	virtual ~ZonePortal(){};
	void addObject(String ent){ents.push_back(ent);}
	void transposeAmbientLight(ColourValue amb);
	virtual void check(Vector3 pos){LogManager::getSingleton().logMessage("Undefined object(Zone or Portal?)");}
	void trigger()
	{
		//LogManager::getSingleton().logMessage("Hide");
		vector<String>::iterator i;
		for (i=ents.begin();i!=ents.end();i++)
		{
			//LogManager::getSingleton().logMessage(*i);
			if (global::getSingleton().getSceneManager()->hasEntity((*i)))
			global::getSingleton().getSceneManager()->getEntity(*i)->setVisible(false);
			//if (global::getSingleton().getSceneManager()->hasLight((*i)))
			//global::getSingleton().getSceneManager()->getLight(*i)->setVisible(false);
		}
	}
	void getBack()
	{
		//LogManager::getSingleton().logMessage("Show");
		vector<String>::iterator i;
		for (i=ents.begin();i!=ents.end();i++)
		{
			//LogManager::getSingleton().logMessage(*i);
			if (global::getSingleton().getSceneManager()->hasEntity((*i)))
			global::getSingleton().getSceneManager()->getEntity(*i)->setVisible(true);
			//if (global::getSingleton().getSceneManager()->hasLight((*i)))
			//global::getSingleton().getSceneManager()->getLight(*i)->setVisible(true);
		}
	}
	AxisAlignedBox mZonePortal;
private:
	
	vector<String> ents;
};


class Zone : public ZonePortal
{
public:
	inline Zone(){};
	inline Zone(AxisAlignedBox zonePortal){mZonePortal=zonePortal;inter=false;trigger();}
	virtual void check(Vector3 pos)
	{
		//if (inter!=mZonePortal.intersects(pos))
		//{
			if (!mZonePortal.intersects(pos))
			{
				///LogManager::getSingleton().logMessage("Hide");
				trigger();
			}
			else
			{
				//LogManager::getSingleton().logMessage("Show");
				getBack();
			}
		//}
		//inter=mZonePortal.intersects(pos);
	}
private:
	bool inter;
};

class Portal : public ZonePortal
{
public:
	inline Portal(){};
	inline Portal(AxisAlignedBox zonePortal){mZonePortal=zonePortal;}
	virtual void check(Vector3 pos)
	{
		//LogManager::getSingleton().logMessage("Portal");
		if (!mZonePortal.intersects(pos))
		{
			trigger();
		}
	}
};