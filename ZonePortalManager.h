/*A PART OF RUN3 GAME ENGINE
Zone Portal Manager by Sgw32 2011
I love Katya!!!
*/
#pragma once

#include "Ogre.h"
#include "global.h"
#include "Manager_Template.h"
#include "ZonePortal.h"

using namespace Ogre;
using namespace std;
 


class ZonePortalManager : public Singleton<ZonePortalManager>, public managerTemplate
{
public:
	ZonePortalManager(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
	ZonePortalManager();
	virtual ~ZonePortalManager();
	virtual void init();
	void passPortal(Vector3 pos,Vector3 size);
	void passZone(Vector3 pos,Vector3 size);
	void setZoneCameraParams(Real of,Real nf);
	void passEntity(String ent)
	{
		if (curZP)
		{
		LogManager::getSingleton().logMessage("Passing entity to zone:"+ent);
		curZP->addObject(ent);
		}
	}
	void triggerAll()
	{
		vector<ZonePortal*>::iterator i;
		for (i=zps.begin();i!=zps.end();i++)
		{
			ZonePortal* a = (*i);
			Zone* b = (Zone*)a;
			b->trigger();
		}
	}
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup();
private:
	vector<ZonePortal*> zps;
	ZonePortal* curZP;
};